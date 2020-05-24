#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <d3d11.h>
#include "AssimpWrapper.h"
#include "SkeletalMesh.h"
#include "AnimationInstance.h"
#include "Skeleton.h"



//@TODO Change code to use assimp wrapper
class SkeletalModel
{
public:

	std::vector<SkeletalMesh> _meshes;

	std::string directory;
	std::string name;

	SMatrix transform;
	
	std::vector<Animation> anims;

	Skeleton _skeleton;

	SkeletalModel();
	~SkeletalModel();


	bool loadModel(ID3D11Device* dvc, const std::string& path, float rUVx = 1, float rUVy = 1)
	{
		assert(FileUtils::fileExists(path) && "File does not exist! ...probably.");

		unsigned int pFlags = aiProcessPreset_TargetRealtime_MaxQuality |
			aiProcess_Triangulate |
			aiProcess_GenSmoothNormals |
			aiProcess_FlipUVs |
			aiProcess_ConvertToLeftHanded |
			aiProcess_LimitBoneWeights;

		// Read file via ASSIMP
		Assimp::Importer importer;

		const aiScene* scene = AssimpWrapper::loadScene(importer, path, pFlags);

		directory = path.substr(0, path.find_last_of('/'));
		name = path.substr(path.find_last_of('/') + 1, path.size());

		aiMatrix4x4 globInvTrans = scene->mRootNode->mTransformation;
		_skeleton._globalInverseTransform = SMatrix(&globInvTrans.a1).Transpose().Invert();

		processNode(dvc, scene->mRootNode, scene, rUVx, rUVy);	/*scene->mRootNode->mTransformation*/

		//adds parent/child relationships
		//relies on names to detect bones amongst other nodes (processNode already collected all bone names using loadBones)
		//and then on map searches to find relationships between the bones

		AssimpWrapper::loadBones(scene, scene->mRootNode, _skeleton);

		// This might be wrong
		const aiNode* skelRoot = AssimpWrapper::findSkeletonRoot(scene->mRootNode, _skeleton);

		if (skelRoot)
			AssimpWrapper::linkSkeletonHierarchy(skelRoot, _skeleton);

		AssimpWrapper::loadAnimations(scene, anims);

		return true;
	}



	bool processNode(ID3D11Device* dvc, aiNode* node, const aiScene* scene, float rUVx, float rUVy)	//aiMatrix4x4 parentTransform, 
	{
		//aiMatrix4x4 concatenatedTransform = parentTransform * node->mTransformation;	//or reversed! careful!
		
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
			_meshes.push_back(processSkeletalMesh(dvc, scene->mMeshes[node->mMeshes[i]], scene, _meshes.size(), rUVx, rUVy)); /*concatenatedTransform*/

		// After we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++)
			this->processNode(dvc, node->mChildren[i], scene, rUVx, rUVy);

		return true;
	}



	SkeletalMesh processSkeletalMesh(ID3D11Device* device, aiMesh *mesh, const aiScene *scene, unsigned int ind/*, aiMatrix4x4 parentTransform*/, float rUVx, float rUVy)
	{
		// Data to fill
		std::vector<BonedVert3D> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> locTextures;

		bool hasTexCoords = mesh->HasTextureCoords(0);

		float radius = AssimpWrapper::loadVertices(mesh, hasTexCoords, vertices);

		AssimpWrapper::loadIndices(mesh, indices);

		AssimpWrapper::loadMaterials(directory, scene, mesh, locTextures);

		for (Texture& t : locTextures)
			t.SetUpAsResource(device);

		AssimpWrapper::loadBonesAndSkinData(*mesh, vertices, _skeleton);

		return SkeletalMesh(vertices, indices, locTextures, device, ind);
	}

};



class SkeletalModelInstance
{
public:

	CBuffer _skeletonTransforms;
	std::vector<SMatrix> _skeletonMatrices;

	SkeletalModel* _skm;

	std::vector<AnimationInstance> _animInstances;

	SkeletalModelInstance() : _skm(nullptr) {}

	bool init(ID3D11Device* dvc, SkeletalModel* skm)
	{
		_skm = skm;

		for (Animation& anim : skm->anims)
		{
			_animInstances.emplace_back(anim);
		}

		
		D3D11_BUFFER_DESC desc = ShaderCompiler::createBufferDesc(sizeof(SMatrix) * 96);
		_skeletonMatrices.resize(_skm->_skeleton._boneMap.size());

		if (FAILED(dvc->CreateBuffer(&desc, NULL, &_skeletonTransforms._cbPtr)))
			return false;

		return true;
	}



	void update(float dTime, UINT animIndex = 0u)
	{

		for (int i = 0; i < _animInstances.size(); ++i)
			_animInstances[i].update(dTime);

		_animInstances[animIndex].getTransformAtTime(*_skm->_skeleton._root, _skeletonMatrices, SMatrix::Identity, _skm->_skeleton._globalInverseTransform);
	}



	void draw(ID3D11DeviceContext* context)
	{
		_skeletonTransforms.updateWholeBuffer(
			context, _skeletonTransforms._cbPtr, _skeletonMatrices.data(), sizeof(SMatrix) * _skm->_skeleton._boneMap.size());

		context->VSSetConstantBuffers(1, 1, &_skeletonTransforms._cbPtr);

		for (SkeletalMesh& m : _skm->_meshes)
		{
			m.draw(context);
		}
	}
};