#include "Rendering/D3D12/D3D12_Buffer.h"
#include <cstring>
#include <system_error>

bool Engine::Graphics::D3D12_BufferBase::create_buffer(
	size_t bytes, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES initial_state,
	std::error_code& ec) {
	if (!device || bytes == 0) { ec = std::make_error_code(std::errc::invalid_argument); return false; }
	D3D12_HEAP_PROPERTIES heap{};
	heap.Type = heap_type;
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = bytes;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	HRESULT hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
		initial_state, nullptr, IID_PPV_ARGS(&resource));
	if (FAILED(hr)) { ec = std::error_code(static_cast<int>(hr), std::system_category()); return false; }
	size = bytes;
	if (heap_type == D3D12_HEAP_TYPE_UPLOAD) {
		resource->Map(0, nullptr, &mapped);
	}
	return true;
}

void Engine::Graphics::D3D12_BufferBase::copy_data(const void* data, size_t bytes, size_t offset) {
	if (!data || !mapped) return;
	std::memcpy(static_cast<unsigned char*>(mapped) + offset, data, bytes);
}

Engine::Graphics::D3D12_VertexBuffer::D3D12_VertexBuffer(ID3D12Device* dev, ID3D12CommandQueue*) { device = dev; }
void Engine::Graphics::D3D12_VertexBuffer::set_data(const void* data, size_t bytes, VertexBufUsage usage) {
	std::error_code ec;
	create_buffer(bytes, usage == USAGE_DYNAMIC ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT,
		usage == USAGE_DYNAMIC ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, ec);
	if (!ec) copy_data(data, bytes, 0);
}
void Engine::Graphics::D3D12_VertexBuffer::update_data(const void* data, size_t bytes, size_t offset) const {
	if (!mapped) return;
	std::memcpy(static_cast<unsigned char*>(mapped) + offset, data, bytes);
}
void Engine::Graphics::D3D12_VertexBuffer::uninit() {
	if (mapped) resource->Unmap(0, nullptr);
	mapped = nullptr; resource.Reset(); size = 0;
}
D3D12_VERTEX_BUFFER_VIEW Engine::Graphics::D3D12_VertexBuffer::get_view(uint32_t base_offset) const {
	D3D12_VERTEX_BUFFER_VIEW view{};
	view.BufferLocation = resource->GetGPUVirtualAddress() + base_offset;
	view.SizeInBytes = static_cast<UINT>(size);
	view.StrideInBytes = 0;
	return view;
}

Engine::Graphics::D3D12_IndexBuffer::D3D12_IndexBuffer(ID3D12Device* dev, ID3D12CommandQueue*) { device = dev; }
void Engine::Graphics::D3D12_IndexBuffer::set_data(const void* data, size_t bytes, IndexBufUsage usage) {
	std::error_code ec;
	create_buffer(bytes, usage == USAGE_DYNAMIC ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT,
		usage == USAGE_DYNAMIC ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_INDEX_BUFFER, ec);
	if (!ec) copy_data(data, bytes, 0);
}
void Engine::Graphics::D3D12_IndexBuffer::update_data(const void* data, size_t bytes, size_t offset) const {
	if (!mapped) return;
	std::memcpy(static_cast<unsigned char*>(mapped) + offset, data, bytes);
}
void Engine::Graphics::D3D12_IndexBuffer::uninit() {
	if (mapped) resource->Unmap(0, nullptr);
	mapped = nullptr; resource.Reset(); size = 0;
}
D3D12_INDEX_BUFFER_VIEW Engine::Graphics::D3D12_IndexBuffer::get_view() const {
	D3D12_INDEX_BUFFER_VIEW view{};
	view.BufferLocation = resource->GetGPUVirtualAddress();
	view.SizeInBytes = static_cast<UINT>(size);
	view.Format = DXGI_FORMAT_R32_UINT;
	return view;
}

Engine::Graphics::D3D12_UniformBuffer::D3D12_UniformBuffer(ID3D12Device* dev) { device = dev; }
void Engine::Graphics::D3D12_UniformBuffer::set_data(const void* data, size_t bytes, UniformBufUsage) {
	const size_t aligned = (bytes + 255) & ~size_t(255);
	std::error_code ec;
	create_buffer(aligned, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, ec);
	copy_data(data, bytes, 0);
}
void Engine::Graphics::D3D12_UniformBuffer::update_data(const void* data, size_t bytes, size_t offset) const {
	if (!mapped) return;
	std::memcpy(static_cast<unsigned char*>(mapped) + offset, data, bytes);
}
void Engine::Graphics::D3D12_UniformBuffer::uninit() {
	if (mapped) resource->Unmap(0, nullptr);
	mapped = nullptr; resource.Reset(); size = 0;
}

Engine::Graphics::D3D12_StorageBuffer::D3D12_StorageBuffer(ID3D12Device* dev) { device = dev; }
void Engine::Graphics::D3D12_StorageBuffer::set_data(const void* data, size_t bytes, StorageBufUsage) {
	std::error_code ec;
	create_buffer(bytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, ec);
	copy_data(data, bytes, 0);
}
void Engine::Graphics::D3D12_StorageBuffer::update_data(const void* data, size_t bytes, size_t offset) const {
	if (!mapped) return;
	std::memcpy(static_cast<unsigned char*>(mapped) + offset, data, bytes);
}
void Engine::Graphics::D3D12_StorageBuffer::uninit() {
	if (mapped) resource->Unmap(0, nullptr);
	mapped = nullptr; resource.Reset(); size = 0;
}
