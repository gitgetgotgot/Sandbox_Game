#include "Rendering/D3D12/D3D12_Renderer.h"
#include "Rendering/D3D12/D3D12_Shader.h"
#include "Rendering/D3D12/D3D12_VertexLayout.h"
#include "Rendering/D3D12/D3D12_Buffer.h"
#include "Rendering/D3D12/D3D12_Texture.h"
#include "Rendering/D3D12/D3D12_TextureArray.h"
#include "Rendering/D3D12/D3D12_Pipeline.h"
#include <Utility/Logger.h>
#include <windows.h>
#include <system_error>

namespace {
void set_hresult_error(std::error_code& ec, HRESULT hr) {
	ec = std::error_code(static_cast<int>(hr), std::system_category());
}
}

Engine::Graphics::D3D12_Renderer::D3D12_Renderer(HWND hwnd, std::error_code& ec)
	: hwnd(hwnd) {
	if (!hwnd) {
		ec = std::make_error_code(std::errc::invalid_argument);
		return;
	}

#if defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
		debug->EnableDebugLayer();
	}
#endif

	Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
	HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }

	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
	for (UINT i = 0;
		factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC1 desc{};
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
		if (SUCCEEDED(hr)) break;
		adapter.Reset();
	}
	if (!device) {
		ec = std::make_error_code(std::errc::no_such_device);
		return;
	}

	D3D12_COMMAND_QUEUE_DESC queue_desc{};
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	hr = device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue));
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }

	create_swap_chain(ec);
	if (ec) return;
	create_render_targets(ec);
	if (ec) return;
	create_depth_buffer(ec);
	if (ec) return;

	for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&command_allocators[i]));
		if (FAILED(hr)) { set_hresult_error(ec, hr); return; }
		fence_values[i] = 0;
	}
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		command_allocators[current_frame_index].Get(), nullptr, IID_PPV_ARGS(&command_list));
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }
	command_list->Close();

	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }
	fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!fence_event) {
		ec = std::make_error_code(std::errc::resource_unavailable_try_again);
		return;
	}
}

Engine::Graphics::D3D12_Renderer::~D3D12_Renderer() {
	std::error_code ec;
	wait_for_gpu(ec);
	if (fence_event) { CloseHandle(fence_event); fence_event = nullptr; }
}

void Engine::Graphics::D3D12_Renderer::create_swap_chain(std::error_code& ec) {
	RECT rect{};
	GetClientRect(hwnd, &rect);
	width = std::max<LONG>(1, rect.right - rect.left);
	height = std::max<LONG>(1, rect.bottom - rect.top);

	Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
	HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }

	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.Width = width;
	desc.Height = height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = FRAME_COUNT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> chain;
	hr = factory->CreateSwapChainForHwnd(command_queue.Get(), hwnd, &desc, nullptr, nullptr, &chain);
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }
	factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	hr = chain.As(&swap_chain);
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }
	current_image_index = swap_chain->GetCurrentBackBufferIndex();
}

void Engine::Graphics::D3D12_Renderer::create_render_targets(std::error_code& ec) {
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = FRAME_COUNT;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtv_heap));
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }
	rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();
	for (uint32_t i = 0; i < FRAME_COUNT; ++i) {
		hr = swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i]));
		if (FAILED(hr)) { set_hresult_error(ec, hr); return; }
		device->CreateRenderTargetView(render_targets[i].Get(), nullptr, handle);
		handle.ptr += rtv_descriptor_size;
	}
}

void Engine::Graphics::D3D12_Renderer::create_depth_buffer(std::error_code& ec) {
	D3D12_DESCRIPTOR_HEAP_DESC dsv_desc{};
	dsv_desc.NumDescriptors = 1;
	dsv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	HRESULT hr = device->CreateDescriptorHeap(&dsv_desc, IID_PPV_ARGS(&dsv_heap));
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }

	D3D12_HEAP_PROPERTIES heap{};
	heap.Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_DESC resource{};
	resource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resource.Width = width;
	resource.Height = height;
	resource.DepthOrArraySize = 1;
	resource.MipLevels = 1;
	resource.Format = DXGI_FORMAT_D32_FLOAT;
	resource.SampleDesc.Count = 1;
	resource.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	D3D12_CLEAR_VALUE clear{};
	clear.Format = DXGI_FORMAT_D32_FLOAT;
	clear.DepthStencil.Depth = 1.0f;
	hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &resource,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&depth_buffer));
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
	dsv.Format = DXGI_FORMAT_D32_FLOAT;
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(depth_buffer.Get(), &dsv,
		dsv_heap->GetCPUDescriptorHandleForHeapStart());
}

void Engine::Graphics::D3D12_Renderer::wait_for_frame_slot(std::error_code& ec) {
	current_frame_index = swap_chain->GetCurrentBackBufferIndex();
	const UINT64 fence_value = ++fence_values[current_frame_index];
	if (fence->GetCompletedValue() < fence_values[current_frame_index]) {
		HRESULT hr = fence->SetEventOnCompletion(fence_value, fence_event);
		if (FAILED(hr)) { set_hresult_error(ec, hr); return; }
		WaitForSingleObject(fence_event, INFINITE);
	}
}

void Engine::Graphics::D3D12_Renderer::start_frame(std::error_code& ec) {
	wait_for_frame_slot(ec);
	if (ec) return;
	command_allocators[current_frame_index]->Reset();
	command_list->Reset(command_allocators[current_frame_index].Get(), nullptr);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = render_targets[current_image_index].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	command_list->ResourceBarrier(1, &barrier);
	frame_started = true;
}

void Engine::Graphics::D3D12_Renderer::clear(float r, float g, float b) {
	if (!frame_started) return;
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtv_heap->GetCPUDescriptorHandleForHeapStart();
	rtv.ptr += static_cast<SIZE_T>(current_image_index) * rtv_descriptor_size;
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsv_heap->GetCPUDescriptorHandleForHeapStart();
	const float color[] = { r, g, b, 1.0f };
	command_list->OMSetRenderTargets(1, &rtv,
		FALSE, &dsv);
	command_list->ClearRenderTargetView(rtv, color, 0, nullptr);
	command_list->ClearDepthStencilView(dsv_heap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH, depth_write ? 1.0f : 0.0f, 0, 0, nullptr);
}

void Engine::Graphics::D3D12_Renderer::present_frame(std::error_code& ec) {
	if (!frame_started) return;
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = render_targets[current_image_index].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	command_list->ResourceBarrier(1, &barrier);
	command_list->Close();
	ID3D12CommandList* lists[] = { command_list.Get() };
	command_queue->ExecuteCommandLists(1, lists);

	HRESULT hr = swap_chain->Present(
		static_cast<UINT>(current_present_mode == PresentMode::Immediate ? 0 : 1), 0);
	if (FAILED(hr)) { set_hresult_error(ec, hr); }

	const UINT64 fence_value = ++fence_values[current_frame_index];
	command_queue->Signal(fence.Get(), fence_value);
	frame_started = false;
	current_image_index = swap_chain->GetCurrentBackBufferIndex();
	move_to_next_frame(ec);
}

void Engine::Graphics::D3D12_Renderer::move_to_next_frame(std::error_code& ec) {
	if (!swap_chain) return;
	current_image_index = swap_chain->GetCurrentBackBufferIndex();
	current_frame_index = current_image_index;
}

void Engine::Graphics::D3D12_Renderer::wait_for_gpu(std::error_code& ec) {
	if (!command_queue || !fence) return;
	const UINT64 value = ++fence_values[current_frame_index];
	HRESULT hr = command_queue->Signal(fence.Get(), value);
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }
	hr = fence->SetEventOnCompletion(value, fence_event);
	if (FAILED(hr)) { set_hresult_error(ec, hr); return; }
	WaitForSingleObject(fence_event, INFINITE);
}

void Engine::Graphics::D3D12_Renderer::on_window_resize(uint32_t window_width, uint32_t window_height, uint8_t mode_code) {
	if (!swap_chain || window_width == 0 || window_height == 0) return;
	std::error_code ec;
	wait_for_gpu(ec);
	for (auto& target : render_targets) target.Reset();
	depth_buffer.Reset();
	swap_chain->ResizeBuffers(FRAME_COUNT, window_width, window_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	width = window_width;
	height = window_height;
	create_render_targets(ec);
	create_depth_buffer(ec);
}

void Engine::Graphics::D3D12_Renderer::set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	D3D12_VIEWPORT viewport{};
	viewport.TopLeftX = static_cast<float>(x);
	viewport.TopLeftY = static_cast<float>(y);
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	command_list->RSSetViewports(1, &viewport);
}

void Engine::Graphics::D3D12_Renderer::use_depth_test(bool use) { depth_test = use; }
void Engine::Graphics::D3D12_Renderer::use_depth_write(bool use) { depth_write = use; }
void Engine::Graphics::D3D12_Renderer::use_depth_func(DepthFunc func) {}

void Engine::Graphics::D3D12_Renderer::set_scissor_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	D3D12_RECT rect{ static_cast<LONG>(x), static_cast<LONG>(y),
		static_cast<LONG>(x + width), static_cast<LONG>(y + height) };
	command_list->RSSetScissorRects(1, &rect);
}

void Engine::Graphics::D3D12_Renderer::bind_pipeline(const Pipeline& p) {
	const auto& dx_pipeline = static_cast<const D3D12_Pipeline&>(p);
	dx_pipeline.bind(*command_list);
	current_pipeline = &p;
}

void Engine::Graphics::D3D12_Renderer::bind_pipeline_push_constant(const Pipeline& p, const void* data) {
	if (!data) return;
	const auto& dx_pipeline = static_cast<const D3D12_Pipeline&>(p);
	dx_pipeline.set_push_constant(*command_list, data);
}

std::unique_ptr<Engine::Graphics::ShaderHandle> Engine::Graphics::D3D12_Renderer::create_ShaderHandle(
	const std::filesystem::path& vertex_shader_path,
	const std::filesystem::path& fragment_shader_path,
	std::error_code& ec
) const {
	return std::make_unique<D3D12_Shader>(device.Get(), vertex_shader_path, fragment_shader_path, ec);
}
std::unique_ptr<Engine::Graphics::VertexLayout> Engine::Graphics::D3D12_Renderer::create_VertexLayout(std::error_code& ec) const {
	return std::make_unique<D3D12_VertexLayout>();
}
std::unique_ptr<Engine::Graphics::VertexBuffer> Engine::Graphics::D3D12_Renderer::create_VertexBuffer(std::error_code& ec) const {
	return std::make_unique<D3D12_VertexBuffer>(device.Get(), command_queue.Get());
}
std::unique_ptr<Engine::Graphics::IndexBuffer> Engine::Graphics::D3D12_Renderer::create_IndexBuffer(std::error_code& ec) const {
	return std::make_unique<D3D12_IndexBuffer>(device.Get(), command_queue.Get());
}
std::unique_ptr<Engine::Graphics::UniformBuffer> Engine::Graphics::D3D12_Renderer::create_UniformBuffer(std::error_code& ec) const {
	return std::make_unique<D3D12_UniformBuffer>(device.Get());
}
std::unique_ptr<Engine::Graphics::StorageBuffer> Engine::Graphics::D3D12_Renderer::create_StorageBuffer(std::error_code& ec) const {
	return std::make_unique<D3D12_StorageBuffer>(device.Get());
}
std::unique_ptr<Engine::Graphics::Texture> Engine::Graphics::D3D12_Renderer::create_Texture(std::error_code& ec) const {
	return std::make_unique<D3D12_Texture>(device.Get(), command_queue.Get());
}
std::unique_ptr<Engine::Graphics::TextureArray> Engine::Graphics::D3D12_Renderer::create_TextureArray(std::error_code& ec) const {
	return std::make_unique<D3D12_TextureArray>(device.Get(), command_queue.Get());
}
std::unique_ptr<Engine::Graphics::Pipeline> Engine::Graphics::D3D12_Renderer::create_Pipeline(std::error_code& ec) const {
	return std::make_unique<D3D12_Pipeline>(device.Get());
}

void Engine::Graphics::D3D12_Renderer::renderIndexedData(
	const VertexBuffer& vb, const IndexBuffer& ib, uint32_t vb_offset,
	uint32_t indexCount, uint32_t baseIndex) {
	const auto& dx_vb = static_cast<const D3D12_VertexBuffer&>(vb);
	const auto& dx_ib = static_cast<const D3D12_IndexBuffer&>(ib);
	command_list->IASetVertexBuffers(0, 1, &dx_vb.get_view(vb_offset));
	command_list->IASetIndexBuffer(&dx_ib.get_view());
	command_list->DrawIndexedInstanced(indexCount, 1, baseIndex, 0, 0);
}

void Engine::Graphics::D3D12_Renderer::renderInstancedData(
	const VertexBuffer& vb, const IndexBuffer& ib, uint32_t indexCount,
	uint32_t instanceCount, uint32_t baseInstance) {
	const auto& dx_vb = static_cast<const D3D12_VertexBuffer&>(vb);
	const auto& dx_ib = static_cast<const D3D12_IndexBuffer&>(ib);
	command_list->IASetVertexBuffers(0, 1, &dx_vb.get_view());
	command_list->IASetIndexBuffer(&dx_ib.get_view());
	command_list->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, baseInstance);
}
