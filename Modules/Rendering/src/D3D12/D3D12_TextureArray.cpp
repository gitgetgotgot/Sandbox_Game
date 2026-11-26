#include "Rendering/D3D12/D3D12_TextureArray.h"
#include <stb_image.h>
#include <cstring>
#include <system_error>

Engine::Graphics::D3D12_TextureArray::D3D12_TextureArray(ID3D12Device* dev, ID3D12CommandQueue* q)
	: device(dev), queue(q) {}

void Engine::Graphics::D3D12_TextureArray::setup_texture_array(
	uint32_t w, uint32_t h, uint32_t d, FilterMode f, WrapMode wa,
	DataFormat df, uint8_t ch, ResizeStrategy rs, std::error_code& ec) {
	width = w; height = h; depth = d; filter = f; wrap = wa;
	data_format = df; channels = ch; resize_strategy = rs; current_size = 0;
	pixel_data.assign(static_cast<size_t>(w) * h * d * 4, 0);
	if (!device) { ec = std::make_error_code(std::errc::invalid_argument); return; }
	D3D12_HEAP_PROPERTIES heap{};
	heap.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = w;
	desc.Height = h;
	desc.DepthOrArraySize = static_cast<UINT16>(d);
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	HRESULT hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
	if (FAILED(hr)) ec = std::error_code(static_cast<int>(hr), std::system_category());
}

void Engine::Graphics::D3D12_TextureArray::add_2D_texture(const char* fileName, std::error_code& ec) {
	int w = 0, h = 0, c = 0;
	unsigned char* pixels = stbi_load(fileName, &w, &h, &c, 4);
	if (!pixels) { ec = std::make_error_code(std::errc::no_such_file_or_directory); return; }
	add_2D_texture(pixels, w, h, 4, ec);
	stbi_image_free(pixels);
}

void Engine::Graphics::D3D12_TextureArray::add_2D_texture(
	const unsigned char* img_buf, int w, int h, int, std::error_code& ec) {
	if (!resource || !img_buf || w <= 0 || h <= 0 || current_size >= depth) {
		ec = std::make_error_code(std::errc::invalid_argument); return;
	}
	void* mapped = nullptr;
	if (SUCCEEDED(resource->Map(0, nullptr, &mapped))) {
		const size_t slice = static_cast<size_t>(width) * height * 4;
		std::memcpy(static_cast<unsigned char*>(mapped) + slice * current_size, img_buf,
			std::min(slice, static_cast<size_t>(w) * h * 4));
		resource->Unmap(0, nullptr);
		++current_size;
	}
}
