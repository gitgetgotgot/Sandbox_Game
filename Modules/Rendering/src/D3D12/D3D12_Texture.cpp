#include "Rendering/D3D12/D3D12_Texture.h"
#include <stb_image.h>
#include <cstdlib>
#include <cstring>

Engine::Graphics::D3D12_Texture::D3D12_Texture(ID3D12Device* dev, ID3D12CommandQueue*) : device(dev) {}

void Engine::Graphics::D3D12_Texture::add_2D_texture(const char* fileName, bool) {
	int w = 0, h = 0, c = 0;
	unsigned char* pixels = stbi_load(fileName, &w, &h, &c, 4);
	if (!pixels) return;
	add_R8_texture_from_buffer(pixels, w, h, false);
	stbi_image_free(pixels);
}

void Engine::Graphics::D3D12_Texture::add_R8_texture_from_buffer(const unsigned char* img_buf, int w, int h, bool) {
	if (!device || !img_buf || w <= 0 || h <= 0) return;
	D3D12_HEAP_PROPERTIES heap{};
	heap.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = static_cast<UINT>(w);
	desc.Height = static_cast<UINT>(h);
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	HRESULT hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
	if (FAILED(hr)) return;
	void* mapped = nullptr;
	if (SUCCEEDED(resource->Map(0, nullptr, &mapped))) {
		std::memcpy(mapped, img_buf, static_cast<size_t>(w) * h * 4);
		resource->Unmap(0, nullptr);
	}
}

void Engine::Graphics::D3D12_Texture::add_text_bitmap(const char* filename, bool isPixelised,
	unsigned char** image_bytes, int& numOfChannels) {
	add_2D_texture(filename, isPixelised);
	if (image_bytes) *image_bytes = nullptr;
	numOfChannels = 4;
}
