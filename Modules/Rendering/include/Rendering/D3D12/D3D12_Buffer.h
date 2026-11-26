#pragma once
#include "Rendering/RHI/VertexBuffer.h"
#include "Rendering/RHI/IndexBuffer.h"
#include "Rendering/RHI/UniformBuffer.h"
#include "Rendering/RHI/StorageBuffer.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <cstddef>

namespace Engine::Graphics {
class D3D12_BufferBase {
protected:
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	Microsoft::WRL::ComPtr<ID3D12Resource> upload_resource;
	size_t size = 0;
	void* mapped = nullptr;
	bool create_buffer(size_t bytes, D3D12_HEAP_TYPE heap_type,
		D3D12_RESOURCE_STATES initial_state, std::error_code& ec);
	void copy_data(const void* data, size_t bytes, size_t offset);
public:
	ID3D12Resource* get_resource() const { return resource.Get(); }
};

class D3D12_VertexBuffer : public VertexBuffer, public D3D12_BufferBase {
public:
	D3D12_VertexBuffer(ID3D12Device* device, ID3D12CommandQueue*);
	~D3D12_VertexBuffer() override { uninit(); }
	void set_data(const void* data, size_t size, VertexBufUsage usage) override;
	void update_data(const void* data, size_t size, size_t offset = 0) const override;
	void uninit() override;
	D3D12_VERTEX_BUFFER_VIEW get_view(uint32_t base_offset = 0) const;
};

class D3D12_IndexBuffer : public IndexBuffer, public D3D12_BufferBase {
public:
	D3D12_IndexBuffer(ID3D12Device* device, ID3D12CommandQueue*);
	~D3D12_IndexBuffer() override { uninit(); }
	void set_data(const void* data, size_t size, IndexBufUsage usage) override;
	void update_data(const void* data, size_t size, size_t offset = 0) const override;
	void uninit() override;
	D3D12_INDEX_BUFFER_VIEW get_view() const;
};

class D3D12_UniformBuffer : public UniformBuffer, public D3D12_BufferBase {
public:
	explicit D3D12_UniformBuffer(ID3D12Device* device);
	~D3D12_UniformBuffer() override { uninit(); }
	void set_data(const void* data, size_t size, UniformBufUsage usage) override;
	void update_data(const void* data, size_t size, size_t offset = 0) const override;
	void uninit() override;
};

class D3D12_StorageBuffer : public StorageBuffer, public D3D12_BufferBase {
public:
	explicit D3D12_StorageBuffer(ID3D12Device* device);
	~D3D12_StorageBuffer() override { uninit(); }
	void set_data(const void* data, size_t size, StorageBufUsage usage) override;
	void update_data(const void* data, size_t size, size_t offset = 0) const override;
	void uninit() override;
};
}
