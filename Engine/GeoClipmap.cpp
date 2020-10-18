#include "GeoClipmap.h"
#include "VertSignature.h"
#include "Shader.h"



GeoClipmap::GeoClipmap(UINT numLayers, UINT edgeSizeLog2, float xzScale)
	: _numLayers(numLayers), _edgeVertCount(pow(2, edgeSizeLog2) - 1), _coreVertSpacing(xzScale)
{
	_blockEdgeVertCount = (_edgeVertCount + 1) / 4;	// Size of the outer layer block in vertices
	_gapSize = _blockEdgeVertCount * 3;	// Cardinal gaps
	_layers.resize(_numLayers);

	// Texture size
	_texSize[0] = _texSize[1] = (_edgeVertCount + 1) + 2 * _numLayers * _blockEdgeVertCount;
}



void GeoClipmap::init(ID3D11Device* device)
{
	// Vertex buffers - consider doing this in morton order?
	



	// Vertex shader
	VertSignature vertSig;
	vertSig.addAttribute(VAttribSemantic::POS, VAttribType::FLOAT2, 1u, 4u);	// Will be backed into float16
	std::vector<D3D11_INPUT_ELEMENT_DESC> inLayDesc = vertSig.createVertInLayElements();

	D3D11_BUFFER_DESC bufferDesc = CBuffer::createDesc(sizeof(GeoClipmapBuffer));

	VertexShader vs(device, L"GeoClipmapVS.hlsl", inLayDesc, { bufferDesc });


	// Textures - 2 per layer, but instead packed into two arrays of _numLayers for faster binding
	D3D11_TEXTURE2D_DESC hmDesc = Texture::create2DTexDesc(_texSize[0], _texSize[1], DXGI_FORMAT_R32_FLOAT,
		D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, 0u, 0u, 1u,
		_numLayers);

	_heightMap.create(device, &hmDesc, nullptr);

	D3D11_TEXTURE2D_DESC nmDesc = Texture::create2DTexDesc(2.f * _texSize[0], 2.f * _texSize[1], DXGI_FORMAT_R8G8B8A8_SNORM,
		D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, 0u, 0u, 1u,
		_numLayers);

	_normalMap.create(device, &nmDesc, nullptr);


	_coreSize = SVec2(_edgeVertCount * _coreVertSpacing);
	_coreOffset = SVec2(-0.5 * _coreSize);

	// Ring layer setup - towards bottom left per layer, starting with central block's offset
	SVec2 accumulatedSize(_coreSize);
	
	// Vertex spacing doubles for every ring layer, including the first
	float baseBlockSize = 2. * _blockEdgeVertCount * _coreVertSpacing;

	for (int i = 0; i < _layers.size(); ++i)
	{
		int scaleModifier = 1 << i;	// 1, 2, 4, 8...

		float blockSize = scaleModifier * baseBlockSize;
		
		RingLayer& rl = _layers[i];
		rl._blockSize = SVec2(blockSize);
		
		accumulatedSize += 2.f * rl._blockSize;	// Increase by the width of new blocks, one each side

		rl._size = accumulatedSize;
		rl._offset = -0.5 * accumulatedSize;
		
		// Each corner has 3 blocks, one in the corner, and two bordering it
		for (int j = 0; j < 4; j++)
		{
			// 2 3
			// 0 1
			SVec2 cornerOffset = rl._offset + SVec2(j & 1, j > 1) * (accumulatedSize - SVec2(blockSize));
			
			float xSign =  1. - 2. * (j % 2);	// 1 - 2 * (0, 1, 0, 1) = 1, -1, 1, -1
			float zSign = -1. + 2. * (j < 2);	// 1, 1, -1, -1
			rl._blockOffsets[j * 3 + 0] = cornerOffset;
			rl._blockOffsets[j * 3 + 1] = cornerOffset + SVec2(blockSize * xSign, 0);
			rl._blockOffsets[j * 3 + 2] = cornerOffset + SVec2(0, blockSize * zSign);
		}
	}
}



// @TODO replace with bufferless, no need for it on newer GPUs (probably not worth it for the rim though)
void GeoClipmap::createBuffers(ID3D11Device* device)
{
	// Central patch buffers
	std::vector<SVec2> vertexData;	// Reuse everywhere
	for (UINT i = 0; i < _edgeVertCount; ++i)
	{
		for (UINT j = 0; j < _edgeVertCount; ++j)
			vertexData.emplace_back(j, i);
	}

	_coreVB = VBuffer(device, vertexData.data(), vertexData.size() * sizeof(SVec2), sizeof(SVec2));
	_coreIB = IBuffer(device, createGridIndices(_edgeVertCount, _edgeVertCount));

	vertexData.clear();

	// Block buffer
	for (UINT i = 0; i < _blockEdgeVertCount; ++i)
	{
		for (UINT j = 0; j < _blockEdgeVertCount; ++j)
			vertexData.emplace_back(j, i);
	}

	_blockVB = VBuffer(device, vertexData.data(), vertexData.size() * sizeof(SVec2), sizeof(SVec2));
	_blockIB = IBuffer(device, createGridIndices(_blockEdgeVertCount, _blockEdgeVertCount));

	vertexData.clear();
	
	// L rim buffer
	UINT lineStripSize = (2 * _blockEdgeVertCount + 1) * 2;
	UINT rimSize = 2 * lineStripSize - 2;

	for (UINT i = 0; i < lineStripSize; ++i)
	{
		vertexData.emplace_back(i & 1, i);	// Vertical strip
	}


	// Degenerate triangles surrounding the layer (or inside it, who do I assign this to)
}



void GeoClipmap::update(ID3D11DeviceContext* context)
{

}



void GeoClipmap::draw(ID3D11DeviceContext* context)
{
}



std::vector<UINT> GeoClipmap::createGridIndices(UINT numCols, UINT numRows)
{
	UINT x = numCols - 1;
	UINT y = numRows - 1;

	std::vector<UINT> indices;
	indices.reserve(x * y * 2 * 3);	// Rows and columns of squares, two triangles each, 3 UINTs each

	for (UINT i = 0; i < y; ++i)		// For every row
	{
		for (UINT j = 0; j < x; ++j)	// For every column in the row
		{
			UINT tli = i * numCols + j;
			UINT tri = tli + 1;
			UINT bli = tli + _edgeVertCount;
			UINT bri = bli + 1;

			indices.insert(indices.end(), { tli, tri, bli });
			indices.insert(indices.end(), { bli, tri, bri });
		}
	}
	
	return indices;
}