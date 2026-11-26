#pragma once
#include <Rendering/RHI/Renderer.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>
#include <memory>

namespace Engine::Graphics {
	class D3D12_Renderer : public Renderer {
	public:
		D3D12_Renderer(HWND hwnd, std::error_code& ec);
		~D3D12_Renderer();
	public:
		void wait_for_frame_slot(std::error_code& ec) override;
		void start_frame(std::error_code& ec) override;
		void clear(float r, float g, float b) override;
		void present_frame(std::error_code& ec) override;
		void on_window_resize(uint32_t window_width, uint32_t window_height, uint8_t mode_code) override;
		void set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		void use_depth_test(bool use) override;
		void use_depth_write(bool use) override;
		void use_depth_func(DepthFunc func) override;
		void set_scissor_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		void bind_pipeline(const Pipeline& p) override;
		void bind_pipeline_push_constant(const Pipeline& p, const void* data) override;

		std::unique_ptr<ShaderHandle> create_ShaderHandle(
			const std::filesystem::path& vertex_shader_path,
			const std::filesystem::path& fragment_shader_path,
			std::error_code& ec
		) const override;
		std::unique_ptr<VertexLayout> create_VertexLayout(std::error_code& ec) const override;
		std::unique_ptr<VertexBuffer> create_VertexBuffer(std::error_code& ec) const override;
		std::unique_ptr<IndexBuffer> create_IndexBuffer(std::error_code& ec) const override;
		std::unique_ptr<UniformBuffer> create_UniformBuffer(std::error_code& ec) const override;
		std::unique_ptr<StorageBuffer> create_StorageBuffer(std::error_code& ec) const override;
		std::unique_ptr<Texture> create_Texture(std::error_code& ec) const override;
		std::unique_ptr<TextureArray> create_TextureArray(std::error_code& ec) const override;
		std::unique_ptr<Pipeline> create_Pipeline(std::error_code& ec) const override;

		void renderIndexedData(
			const VertexBuffer& vb,
			const IndexBuffer& ib,
			uint32_t vb_offset,
			uint32_t indexCount,
			uint32_t baseIndex = 0
		) override;

		void renderInstancedData(
			const VertexBuffer& vb,
			const IndexBuffer& ib,
			uint32_t indexCount,
			uint32_t instanceCount,
			uint32_t baseInstance = 0
		) override;
	private:
		HWND hwnd = nullptr;
		static constexpr uint32_t FRAME_COUNT = 2;
		Microsoft::WRL::ComPtr<ID3D12Device> device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> swap_chain;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap;
		Microsoft::WRL::ComPtr<ID3D12Resource> depth_buffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> render_targets[FRAME_COUNT];
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocators[FRAME_COUNT];
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		HANDLE fence_event = nullptr;
		UINT64 fence_values[FRAME_COUNT]{};
		UINT rtv_descriptor_size = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		bool depth_test = false;
		bool depth_write = true;
		bool frame_started = false;

		void create_swap_chain(std::error_code& ec);
		void create_render_targets(std::error_code& ec);
		void create_depth_buffer(std::error_code& ec);
		void wait_for_gpu(std::error_code& ec);
		void move_to_next_frame(std::error_code& ec);
	public:
		ID3D12Device* get_device() const { return device.Get(); }
		ID3D12CommandQueue* get_command_queue() const { return command_queue.Get(); }
		ID3D12GraphicsCommandList* get_command_list() const { return command_list.Get(); }
		uint32_t get_frame_count() const { return FRAME_COUNT; }
	};
}