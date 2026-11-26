#include "Rendering/D3D12/D3D12_VertexLayout.h"

void Engine::Graphics::D3D12_VertexLayout::add_attribute(
	uint32_t location, VertexAttributeType type, size_t stride, size_t offset) {
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	switch (type) {
	case VertexAttributeType::FLOAT: format = DXGI_FORMAT_R32_FLOAT; break;
	case VertexAttributeType::FLOAT_VEC2: format = DXGI_FORMAT_R32G32_FLOAT; break;
	case VertexAttributeType::FLOAT_VEC3: format = DXGI_FORMAT_R32G32B32_FLOAT; break;
	case VertexAttributeType::FLOAT_VEC4: format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
	case VertexAttributeType::UINT: format = DXGI_FORMAT_R32_UINT; break;
	case VertexAttributeType::UINT_VEC2: format = DXGI_FORMAT_R32G32_UINT; break;
	case VertexAttributeType::UINT_VEC3: format = DXGI_FORMAT_R32G32B32_UINT; break;
	case VertexAttributeType::UINT_VEC4: format = DXGI_FORMAT_R32G32B32A32_UINT; break;
	case VertexAttributeType::INT: format = DXGI_FORMAT_R32_SINT; break;
	case VertexAttributeType::INT_VEC2: format = DXGI_FORMAT_R32G32_SINT; break;
	case VertexAttributeType::INT_VEC3: format = DXGI_FORMAT_R32G32B32_SINT; break;
	case VertexAttributeType::INT_VEC4: format = DXGI_FORMAT_R32G32B32A32_SINT; break;
	}
	D3D12_INPUT_ELEMENT_DESC desc{};
	desc.SemanticName = "ATTRIBUTE";
	desc.SemanticIndex = location;
	desc.Format = format;
	desc.InputSlot = 0;
	desc.AlignedByteOffset = static_cast<UINT>(offset);
	desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	desc.InstanceDataStepRate = 0;
	elements.push_back(desc);
}
