#pragma once
#include "TextureMetaData.h"

struct MaterialAsset
{
	struct TextureRef
	{
		TextureMetaData _texMetaData;
		AssetID _textureAssetID;

		template <typename Archive>
		void serialize(Archive& ar)
		{
			ar(_texMetaData, _textureAssetID);
		}
	};

	AssetID _shaderIDs;

	std::vector<TextureRef> _textures;

	bool _opaque{ false };

	MaterialAsset() = default;

	template <typename Archive>
	void serialize(Archive& ar)
	{
		ar(_shaderIDs, _textures, _opaque);
	}
};