#pragma once
#include "Rendering/RHI/Texture.h"
#include <d3d12.h>
#include <wrl/client.h>

namespace Engine::Graphics {
class D3D12_Texture : public Texture {
public:
	D3D12_Texture(ID3D12Device* device, ID3D12CommandQueue* queue);
	void add_2D_texture(const char* fileName, bool isPixelised) override;
	void add_R8_texture_from_buffer(const unsigned char* img_buf, int width, int height, bool isPixelised) override;
	void add_text_bitmap(const char* filename, bool isPixelised, unsigned char** image_bytes, int& numOfChannels) override;
	ID3D12Resource* get_resource() const { return resource.Get(); }
private:
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
};
}
