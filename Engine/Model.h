#pragma once

#include <string>
#include <vector>
#include <d3d11.h>

#include "Mesh.h"

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>


namespace Procedural { class Terrain; }


class Model : public Resource
{
private:

	bool processNode(ID3D11Device* device, aiNode* node, const aiScene* scene, aiMatrix4x4 parentTransform);

public:

	std::string _path;

	std::vector<Mesh> _meshes;

	SMatrix _transform;

	//remove this eventually when game object becomes better defined... used model for it so far...
	Collider* collider;

	Model() : collider(nullptr) {}
	Model(const std::string& path);
	Model(const Collider& collider, ID3D11Device* device);
	~Model();

	// Separate model and terrain completely, terrain needs a different way to render
	Model(const Procedural::Terrain& terrain, ID3D11Device* device);	

	bool LoadModel(ID3D11Device* device, const std::string& path);
	bool LoadFromScene(ID3D11Device* device, const aiScene* scene);

	template<class Archive>
	void serialize(Archive& archive, std::vector<UINT>& meshIndices)
	{
		archive(_transform, _meshes.size(), meshIndices);
	}
};