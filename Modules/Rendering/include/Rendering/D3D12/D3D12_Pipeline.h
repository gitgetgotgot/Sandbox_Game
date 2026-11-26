#pragma once
#include "Rendering/RHI/Pipeline.h"
#include "Rendering/D3D12/D3D12_Buffer.h"
#include <d3d12.h>
#include <wrl/client.h>

namespace Engine::Graphics {
class D3D12_Pipeline : public Pipeline {
public:
	explicit D3D12_Pipeline(ID3D12Device* device);
	~D3D12_Pipeline() override { uninit(); }
	void init(const ShaderHandle& shader, const VertexLayout& layout,
		PipelinePrimitives p, PipelineScissor s, PipelineViewport v,
		PipelineBlendMode bm, PipelineDepthMode dm, PipelineCullMode cm,
		std::error_code& ec) override;
	void uninit() override;
	void bind_uniform_buffer(UniformBuffer& ub, uint32_t binding = 0, uint32_t set = 0) const override;
	void bind_storage_buffer(StorageBuffer& sb, uint32_t binding = 0, uint32_t set = 0) const override;
	void bind_sampler_2D(Texture& tex, uint32_t binding = 0, uint32_t set = 0) const override;
	void bind_sampler_2D_array(TextureArray& tex_arr, uint32_t binding = 0, uint32_t set = 0) const override;
	void add_push_constant(const char* name, PushConstantType type) override;
	void bind(ID3D12GraphicsCommandList& command_list) const;
	void set_push_constant(ID3D12GraphicsCommandList& command_list, const void* data) const;
private:
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbv_heap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_heap;
	UniformBuffer* uniform = nullptr;
	Texture* texture = nullptr;
	TextureArray* texture_array = nullptr;
	UINT cbv_increment = 0;
	UINT srv_increment = 0;
};
}
