#pragma once
#include "Rendering/RHI/TextureArray.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

namespace Engine::Graphics {
class D3D12_TextureArray : public TextureArray {
public:
	D3D12_TextureArray(ID3D12Device* device, ID3D12CommandQueue* queue);
	void setup_texture_array(uint32_t width, uint32_t height, uint32_t depth,
		FilterMode filter, WrapMode wrap, DataFormat data_format,
		uint8_t channels, ResizeStrategy resize_str, std::error_code& ec) override;
	void add_2D_texture(const char* fileName, std::error_code& ec) override;
	void add_2D_texture(const unsigned char* img_buf, int width, int height, int numOfChannels, std::error_code& ec) override;
	ID3D12Resource* get_resource() const { return resource.Get(); }
private:
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	std::vector<unsigned char> pixel_data;
};
}
