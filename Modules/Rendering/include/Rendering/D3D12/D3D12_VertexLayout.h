#pragma once
#include "Rendering/RHI/VertexLayout.h"
#include <d3d12.h>
#include <vector>

namespace Engine::Graphics {
class D3D12_VertexLayout : public VertexLayout {
public:
	void add_attribute(uint32_t location, VertexAttributeType type, size_t stride, size_t offset) override;
	void uninit() override { elements.clear(); }
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& get_elements() const { return elements; }
private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
};
}
