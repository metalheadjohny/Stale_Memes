#pragma once
#include "Model.h"
#include "FileUtilities.h"
#include "GuiBlocks.h"
#include "Texture.h"

class AssimpPreview
{
private:

	Assimp::Importer _importer;

	std::string _path;
	const aiScene* _scene;

	UINT _meshIndex;

	std::vector<Texture> _textures;

public:

	bool loadAiScene(const std::string& path, UINT inFlags)
	{
		_path = path;
		_meshIndex = 0;
		_textures.clear();

		assert(FileUtils::fileExists(path) && "File does not exist! ...probably.");

		unsigned int pFlags =
			aiProcessPreset_TargetRealtime_MaxQuality |
			aiProcess_Triangulate |
			aiProcess_GenSmoothNormals |
			aiProcess_FlipUVs |
			aiProcess_PreTransformVertices |
			aiProcess_ConvertToLeftHanded;

		_scene = _importer.ReadFile(path, pFlags);

		if (!_scene || _scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !_scene->mRootNode)
		{
			std::string errString("Assimp error:" + std::string(_importer.GetErrorString()));
			OutputDebugStringA(errString.c_str());
			return false;
		}

		return true;
	}
	


	void displayAiScene()
	{
		printaiNode(_scene->mRootNode, _scene, _scene->mRootNode->mTransformation);
	}



	bool printaiNode(aiNode* node, const aiScene* scene, aiMatrix4x4 parentTransform)
	{
		aiMatrix4x4 concatenatedTransform = parentTransform * node->mTransformation;
		

		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			++_meshIndex;
			printAiMesh(mesh, concatenatedTransform);
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			printaiNode(node->mChildren[i], scene, concatenatedTransform);
		}

		return true;
	}



	void printAiMesh(aiMesh* mesh, aiMatrix4x4 parentGlobal)
	{
		UINT numUVChannels = mesh->GetNumUVChannels();
		auto numUVComponents = mesh->mNumUVComponents;

		bool hasTexCoords = mesh->mTextureCoords[0];
		
		//float maxDist = 0.f;
		//Vert3D vertex;

		for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
		{
			/*
			vertex.pos = SVec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

			float curDist = vertex.pos.LengthSquared();
			if (maxDist < curDist)
				maxDist = curDist;

			vertex.normal = SVec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			//vertex.normal.Normalize();	//not sure if required, should be so already

			vertex.texCoords = hasTexCoords ? SVec2(aiMesh->mTextureCoords[0][i].x * rUVx, aiMesh->mTextureCoords[0][i].y * rUVy) : SVec2::Zero;

			mesh._vertices.push_back(vertex);
			*/
		}

		//maxDist = sqrt(maxDist);


		aiFace face;
		for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
		{
			face = mesh->mFaces[i];

			//populate indices from faces
			//for (unsigned int j = 0; j < face.mNumIndices; ++j) mesh._indices.push_back(face.mIndices[j]);
		}


		//even if it's not too fast, this is still a better solution to the previous one (bottom)
		for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
		{
			face = mesh->mFaces[i];	//used only for the name hopefully optimized away... don't need repeated allocation really...

			//assign face tangents to vertex tangents
			//for (unsigned int j = 0; j < face.mNumIndices; ++j) mesh._vertices[face.mIndices[j]].tangent += faceTangents[i];
		}

		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = _scene->mMaterials[mesh->mMaterialIndex];

			// Diffuse maps
			loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", DIFFUSE);

			//  Normal maps
			loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", NORMAL);

			// Specular maps
			loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", SPECULAR);

			// Shininess maps
			loadMaterialTextures(material, aiTextureType_SHININESS, "texture_shininess", SHININESS);

			// Opacity maps
			loadMaterialTextures(material, aiTextureType_OPACITY, "texture_opacity", OPACITY);

			// Displacement maps
			loadMaterialTextures(material, aiTextureType_DISPLACEMENT, "texture_disp", DISPLACEMENT);

			// Ambient occlusion maps
			loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_AO", AMBIENT);

			// Other maps
			loadMaterialTextures(material, aiTextureType_UNKNOWN, "texture_other", OTHER);

			// Weird properties... that I never really saw trigger
			loadMaterialTextures(material, aiTextureType_NONE, "texture_property", OTHER);
		}
	}



	void printAiMaterial()
	{

	}



	bool loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName, TextureRole role)
	{
		//iterate all textures of relevant related to the material
		for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i)
		{
			aiString obtainedTexturePath;
			mat->GetTexture(type, i, &obtainedTexturePath);

			std::string texPath = _path.substr(0, _path.find_last_of("/\\")) + "/" + std::string(obtainedTexturePath.data);
			
			Texture curTexture;
			curTexture._fileName = texPath;
			curTexture._typeName = typeName;
			curTexture._role = role;

			//try to load this texture from file
			bool loaded = curTexture.LoadFromStoredPath();

			//load from file failed - probably means it is embedded, try to load from memory instead...
			if (!loaded)
			{
				int embeddedIndex = atoi(obtainedTexturePath.C_Str() + sizeof(char));	//skip the * with + sizeof(char)
				loaded = loadEmbeddedTexture(curTexture, embeddedIndex);
			}


			//load failed completely - most likely the data is corrupted or my library doesn't support it
			if (!loaded)
			{
				OutputDebugStringA("TEX_LOAD::Texture did not load! \n"); //@TODO use logger here instead
				continue;
			}
			
			
			_textures.push_back(curTexture);
		}

		//goes through for now... I'm using some bootleg meshes, happens on occasion, don't want to terminate over it
		return true;
	}



	bool loadEmbeddedTexture(Texture& texture, UINT index)
	{
		if (!_scene->mTextures)
			return false;

		aiTexture* aiTex = _scene->mTextures[index];

		if (!aiTex)
			return false;

		size_t texSize = aiTex->mWidth;

		// Compressed (embedded) textures could have height value of 0
		if (aiTex->mHeight != 0)
			texSize *= aiTex->mHeight;

		texture.LoadFromMemory(reinterpret_cast<unsigned char*>(aiTex->pcData), texSize);

		return true;
	}

};