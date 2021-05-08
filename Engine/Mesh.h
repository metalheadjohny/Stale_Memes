#pragma once
#include "Resource.h"
#include "VBuffer.h"
#include "IBuffer.h"
#include "Material.h"
#include "Math.h"

#include "MeshDataStructs.h"
#include "AssimpWrapper.h"
#include "VertSignature.h"

namespace Procedural
{
	class Geometry;
	class Terrain;
}


class Mesh : public Resource
{
public:

	VertSignature _vertSig;

	//vertices and indices should be cleared after pushing to the gpu, leaving only the vector memory cost
	std::vector<uint8_t> _vertices;
	std::vector<uint32_t> _indices;

	//handles to GPU data abstracted in my own classes (useful if I ever get to supporting multiple API-s)
	VBuffer _vertexBuffer;
	IBuffer _indexBuffer;

	std::shared_ptr<Material> _material;

	SMatrix _transform;

	Mesh() {};
	~Mesh() = default;

	// Not so sure, seems like heavy coupling for no reason really!
	Mesh(const SVec2& pos, const SVec2& size, ID3D11Device* device, float z = 0.f);	//this is used for the screen quads...
	Mesh(const Procedural::Geometry& g, ID3D11Device* device, bool setUp = true, bool hasTangents = true);
	Mesh(const Procedural::Terrain& terrain, ID3D11Device* device);

	void loadFromAssimp(const aiScene* scene, ID3D11Device* device, aiMesh* aiMesh,
		std::vector<std::shared_ptr<Material>>& mats, const std::string& path);

	inline const SMatrix& renderTransform() const
	{
		return _transform;
	}


	//@TODO - pull D3D11_BUFFER_DESC from a parameter?
	bool setupMesh(ID3D11Device* device);



	void draw(ID3D11DeviceContext* dc)
	{
		//update and set cbuffers
		_material->getVS()->updateBuffersAuto(dc, *this);
		_material->getVS()->setBuffers(dc);
		_material->getPS()->updateBuffersAuto(dc, *this);
		_material->getPS()->setBuffers(dc);

		//set shaders and similar geebees
		_material->bind(dc);

		//could sort by this as well... should be fairly uniform though
		dc->IASetPrimitiveTopology(_material->_primitiveTopology);

		//these have to change each time unless I'm packing multiple meshes per buffer... can live with that tbh
		dc->IASetVertexBuffers(0, 1, _vertexBuffer.ptr(), &_vertexBuffer._stride, &_vertexBuffer._offset);
		dc->IASetIndexBuffer(_indexBuffer.ptr(), DXGI_FORMAT_R32_UINT, 0);

		dc->DrawIndexed(_indexBuffer.getIdxCount(), 0, 0);
	}


	void bind(ID3D11DeviceContext* context)
	{
		_vertexBuffer.bind(context);
		_indexBuffer.bind(context);
	}

	inline UINT getStride() const { return _vertexBuffer._stride; }

	inline UINT getOffset() const { return _vertexBuffer._offset; }



	template<class Archive>
	void serialize(Archive& archive, UINT matID)
	{
		archive(_indices.size(), _vertices.size(), matID, _transform, _indices, _vertices);
	}
};