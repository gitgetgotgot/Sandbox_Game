#include "Rendering/D3D12/D3D12_Pipeline.h"
#include "Rendering/D3D12/D3D12_Shader.h"
#include "Rendering/D3D12/D3D12_VertexLayout.h"
#include "Rendering/D3D12/D3D12_Texture.h"
#include "Rendering/D3D12/D3D12_TextureArray.h"
#include <system_error>

Engine::Graphics::D3D12_Pipeline::D3D12_Pipeline(ID3D12Device* dev) : device(dev) {}

void Engine::Graphics::D3D12_Pipeline::init(
	const ShaderHandle& shader, const VertexLayout& layout,
	PipelinePrimitives primitive, PipelineScissor, PipelineViewport,
	PipelineBlendMode blend, PipelineDepthMode depth, PipelineCullMode cull,
	std::error_code& ec) {
	const auto& dx_shader = static_cast<const D3D12_Shader&>(shader);
	const auto& dx_layout = static_cast<const D3D12_VertexLayout&>(layout);

	D3D12_ROOT_PARAMETER1 params[3]{};
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[0].Descriptor.ShaderRegister = 0;
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	params[1].Constants.ShaderRegister = 1;
	params[1].Constants.RegisterSpace = 0;
	params[1].Constants.Num32BitValues = 16;
	params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	params[2].Descriptor.ShaderRegister = 0;
	params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rs_desc{};
	rs_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rs_desc.Desc_1_1.NumParameters = 3;
	rs_desc.Desc_1_1.pParameters = params;
	rs_desc.Desc_1_1.NumStaticSamplers = 0;
	rs_desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;
	HRESULT hr = D3D12SerializeVersionedRootSignature(&rs_desc, &signature, &errors);
	if (FAILED(hr)) { ec = std::error_code(static_cast<int>(hr), std::system_category()); return; }
	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(&root_signature));
	if (FAILED(hr)) { ec = std::error_code(static_cast<int>(hr), std::system_category()); return; }

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
	pso.pRootSignature = root_signature.Get();
	pso.VS = { dx_shader.get_vertex_bytecode()->GetBufferPointer(),
		dx_shader.get_vertex_bytecode()->GetBufferSize() };
	pso.PS = { dx_shader.get_pixel_bytecode()->GetBufferPointer(),
		dx_shader.get_pixel_bytecode()->GetBufferSize() };
	pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	pso.BlendState.RenderTarget[0].BlendEnable = blend != PipelineBlendMode::None;
	pso.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	pso.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	pso.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	pso.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	pso.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	pso.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pso.SampleMask = UINT_MAX;
	pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pso.RasterizerState.CullMode = cull == PipelineCullMode::None ? D3D12_CULL_MODE_NONE :
		cull == PipelineCullMode::Front ? D3D12_CULL_MODE_FRONT : D3D12_CULL_MODE_BACK;
	pso.RasterizerState.FrontCounterClockwise = FALSE;
	pso.DepthStencilState.DepthEnable = depth != PipelineDepthMode::None;
	pso.DepthStencilState.DepthWriteMask = depth == PipelineDepthMode::ReadAndWrite ?
		D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	pso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	pso.InputLayout = { dx_layout.get_elements().data(),
		static_cast<UINT>(dx_layout.get_elements().size()) };
	pso.PrimitiveTopologyType = primitive == PipelinePrimitives::Point ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT :
		primitive == PipelinePrimitives::Line ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE :
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pso.NumRenderTargets = 1;
	pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pso.SampleDesc.Count = 1;
	hr = device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipeline_state));
	if (FAILED(hr)) ec = std::error_code(static_cast<int>(hr), std::system_category());
}

void Engine::Graphics::D3D12_Pipeline::uninit() {
	pipeline_state.Reset(); root_signature.Reset(); cbv_heap.Reset(); srv_heap.Reset();
	uniform = nullptr; texture = nullptr; texture_array = nullptr;
}

void Engine::Graphics::D3D12_Pipeline::bind_uniform_buffer(UniformBuffer& ub, uint32_t, uint32_t) const {
	auto& mutable_self = const_cast<D3D12_Pipeline&>(*this);
	mutable_self.uniform = &ub;
}
void Engine::Graphics::D3D12_Pipeline::bind_storage_buffer(StorageBuffer&, uint32_t, uint32_t) const {}
void Engine::Graphics::D3D12_Pipeline::bind_sampler_2D(Texture& tex, uint32_t, uint32_t) const {
	const_cast<D3D12_Pipeline&>(*this).texture = &tex;
}
void Engine::Graphics::D3D12_Pipeline::bind_sampler_2D_array(TextureArray& tex_arr, uint32_t, uint32_t) const {
	const_cast<D3D12_Pipeline&>(*this).texture_array = &tex_arr;
}
void Engine::Graphics::D3D12_Pipeline::add_push_constant(const char*, PushConstantType type) {
	push_constant_type = type;
}

void Engine::Graphics::D3D12_Pipeline::bind(ID3D12GraphicsCommandList& command_list) const {
	command_list.SetPipelineState(pipeline_state.Get());
	command_list.SetGraphicsRootSignature(root_signature.Get());
	command_list.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	if (uniform) {
		const auto& ub = static_cast<const D3D12_UniformBuffer&>(*uniform);
		command_list.SetGraphicsRootConstantBufferView(0, ub.get_resource()->GetGPUVirtualAddress());
	}
	if (texture_array) {
		const auto& ta = static_cast<const D3D12_TextureArray&>(*texture_array);
		command_list.SetGraphicsRootShaderResourceView(2, ta.get_resource()->GetGPUVirtualAddress());
	} else if (texture) {
		const auto& tx = static_cast<const D3D12_Texture&>(*texture);
		command_list.SetGraphicsRootShaderResourceView(2, tx.get_resource()->GetGPUVirtualAddress());
	}
}

void Engine::Graphics::D3D12_Pipeline::set_push_constant(
	ID3D12GraphicsCommandList& command_list, const void* data) const {
	switch (push_constant_type) {
	case PushConstantType::Float:
		command_list.SetGraphicsRoot32BitConstants(1, 1,
			data, 0); break;
	case PushConstantType::Vec2:
		command_list.SetGraphicsRoot32BitConstants(1, 2,
			data, 0); break;
	case PushConstantType::Vec3:
		command_list.SetGraphicsRoot32BitConstants(1, 3,
			data, 0); break;
	case PushConstantType::Vec4:
		command_list.SetGraphicsRoot32BitConstants(1, 4,
			data, 0); break;
	default: break;
	}
}
