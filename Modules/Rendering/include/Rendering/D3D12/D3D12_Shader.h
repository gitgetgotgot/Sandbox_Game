#pragma once
#include "Rendering/RHI/ShaderHandle.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

namespace Engine::Graphics {
class D3D12_Shader : public ShaderHandle {
public:
	D3D12_Shader(ID3D12Device* device,
		const std::filesystem::path& vertex_shader_path,
		const std::filesystem::path& fragment_shader_path,
		std::error_code& ec);
	~D3D12_Shader() override { uninit(); }
	void uninit() override;
	ID3DBlob* get_vertex_bytecode() const { return vertex_bytecode.Get(); }
	ID3DBlob* get_pixel_bytecode() const { return pixel_bytecode.Get(); }
private:
	Microsoft::WRL::ComPtr<ID3DBlob> vertex_bytecode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixel_bytecode;
};
}
