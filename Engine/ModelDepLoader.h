#pragma once
#include "ModelLoader.h"
#include "TextureLoader.h"
#include "MaterialManager.h"
#include "AeonLoader.h"


class ModelDepLoader
{
public:
	
	AssetLedger* assetLedger;
	MaterialManager* materialManager;
	AeonLoader* aeonLoader;


	std::unique_ptr<SkModel> loadSkModel(AssetID assetID)
	{
		auto filePath = assetLedger->get(assetID);

		if (!filePath)
		{
			assert(false && "File path not found for given assetID.");
			return {};
		}

		//aeonLoader->requestAsset(assetID, filePath->c_str());
		
		aeonLoader->request(filePath->c_str(), 
			[](Blob&& loadedBlob)
			{
				std::istringstream iss(std::ios_base::binary | std::ios_base::beg);
				iss.rdbuf()->pubsetbuf(loadedBlob.dataAsType<char>(), loadedBlob.size());
				
				cereal::BinaryInputArchive bia(iss);
				
				ModelAsset modelAsset;
				modelAsset.serialize(bia);

				return modelAsset;
			});

		auto skModelAsset = AssetHelpers::DeserializeFromFile<SkModelAsset>(filePath->c_str());

		auto result = std::make_unique<SkModel>();

		result->_meshes.reserve(skModelAsset.model.meshes.size());

		for (auto& meshAsset : skModelAsset.model.meshes)
		{
			result->_meshes.push_back(Mesh{});
			Mesh& mesh = result->_meshes.back();

			mesh._vertices = std::move(meshAsset.vertices);
			mesh._indices = std::move(meshAsset.indices);
			mesh._vertSig = std::move(meshAsset.vertSig);

			// External dependency, use the cached version
			//mesh._material = MaterialLoader::LoadMaterialFromID(meshAsset.material, *assetLedger);
			materialManager->get(meshAsset.material);
		}

		return result;
	}
};