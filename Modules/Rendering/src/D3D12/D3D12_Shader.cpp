#include "Rendering/D3D12/D3D12_Shader.h"
#include <d3dcompiler.h>
#include <fstream>
#include <system_error>
#include <cstring>

namespace {
bool read_blob(const std::filesystem::path& path, const char* target,
	Microsoft::WRL::ComPtr<ID3DBlob>& blob) {
	std::ifstream file(path, std::ios::binary);
	if (!file) return false;
	file.seekg(0, std::ios::end);
	const std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	if (size <= 0) return false;
	std::vector<char> source(static_cast<size_t>(size) + 1);
	file.read(source.data(), size);
	source.back() = '\0';
	if (size >= 4 &&
		static_cast<unsigned char>(source[0]) == 'D' &&
		static_cast<unsigned char>(source[1]) == 'X' &&
		static_cast<unsigned char>(source[2]) == 'B' &&
		static_cast<unsigned char>(source[3]) == 'C') {
		Microsoft::WRL::ComPtr<ID3D10Blob> raw;
		if (FAILED(D3DCreateBlob(static_cast<SIZE_T>(size), &raw))) return false;
		std::memcpy(raw->GetBufferPointer(), source.data(), static_cast<size_t>(size));
		blob = raw;
		return true;
	}
	Microsoft::WRL::ComPtr<ID3D10Blob> raw;
	Microsoft::WRL::ComPtr<ID3D10Blob> errors;
	HRESULT hr = D3DCompile(source.data(), static_cast<SIZE_T>(size), path.string().c_str(),
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", target, 0, 0, &raw, &errors);
	blob = raw;
	return SUCCEEDED(hr);
}
}

Engine::Graphics::D3D12_Shader::D3D12_Shader(ID3D12Device*,
	const std::filesystem::path& vertex_shader_path,
	const std::filesystem::path& fragment_shader_path,
	std::error_code& ec) {
	if (!read_blob(vertex_shader_path, "vs_5_0", vertex_bytecode) ||
		!read_blob(fragment_shader_path, "ps_5_0", pixel_bytecode)) {
		ec = std::make_error_code(std::errc::no_such_file_or_directory);
		return;
	}
}

void Engine::Graphics::D3D12_Shader::uninit() {
	vertex_bytecode.Reset();
	pixel_bytecode.Reset();
}
