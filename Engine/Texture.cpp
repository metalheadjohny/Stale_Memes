#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_PERLIN_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "stb_image_write.h"
#include "stb_perlin.h"
#include "WICTextureLoader.h"
#include "Texture.h"
#include "Perlin.h"


Texture::Texture() : _dxID(nullptr), _srv(nullptr) {}



Texture::Texture(ID3D11Device* device, const std::string& fileName) : _fileName(fileName), _dxID(nullptr), _srv(nullptr)
{
	if (!loadFromStoredPath())
	{
		OutputDebugStringA("Texture not in file, checking memory... \n");
		return;
	}

	SetUpAsResource(device);
}



Texture::Texture(const std::string& fileName) : _fileName(fileName), _dxID(nullptr), _srv(nullptr)
{
	if (!loadFromStoredPath())
	{
		OutputDebugStringA("Texture not in file, checking memory... \n");
	}
}



Texture::Texture(const Texture& other)
	: _w(other._w), _h(other._h), _nc(other._nc), _mdata(other._mdata), _fileName(other._fileName), 
	_dxID(other._dxID), _srv(other._srv)
{
	if(_dxID)
		_dxID->AddRef();

	if(_srv)
		_srv->AddRef();
}



Texture::Texture(Texture&& other)
	: _w(other._w), _h(other._h), _nc(other._nc), _mdata(std::move(other._mdata)),
	_fileName(std::move(other._fileName)), _dxID(std::move(other._dxID)), _srv(std::move(other._srv))
{
	// do not add refs because it's moved as opposed to copied
	// Damage control :\ I'm not sure if this is well implemented so I need to know when I start using it
	//assert(false);	
}



Texture& Texture::operator=(const Texture& other)
{
	_w = other._w;
	_h = other._h;
	_nc = other._nc;

	_mdata = other._mdata;

	_fileName = other._fileName;

	_dxID->AddRef();
	_srv->AddRef();

	return *this;
}



Texture::~Texture()
{
	if (_dxID)
		_dxID->Release();

	if(_srv)
		_srv->Release();
}



// Handle DirectX BS where it doesn't allow to pass 3 byte tex and automatically pad...
int Texture::GetFormatFromFile(const char* filename)
{
	int w, h, n;
	stbi_info(filename, &w, &h, &n);	//_nc is our format but holds this temporarily
	return n == 3 ? 4 : 0;
}



int Texture::GetFormatFromMemory(const unsigned char* data, size_t size)
{
	int w, h, n;
	stbi_info_from_memory(data, size, &w, &h, &n);
	return n == 3 ? 4 : 0;
}



void Texture::loadFromFile(const char* filename)
{
	int fileFormat;
	int desiredFormat = GetFormatFromFile(filename);
	_mdata = std::shared_ptr<unsigned char>(stbi_load(filename, &_w, &_h, &fileFormat, desiredFormat));
	_nc = fileFormat == 3 ? 4 : fileFormat;
}



bool Texture::loadFromStoredPath()
{
	try
	{
		loadFromFile(_fileName.c_str());
		return (_mdata.get() != nullptr);
	}
	catch (...)
	{
		OutputDebugStringA( ("Error loading texture '" + _fileName + "' \n").c_str() );
		return false;
	}
}



bool Texture::loadFromPath(const char* path)
{
	_fileName = path;

	try
	{
		loadFromFile(path);
		return (_mdata.get() != nullptr);
	}
	catch (...)
	{
		OutputDebugStringA(("Error loading texture '" + _fileName + "' \n").c_str());
		return false;
	}
}



std::vector<float> Texture::LoadAsFloatVec(const std::string& path)
{
	float* temp;
	try
	{
		int tw, th, tn;

		// Staying as it is to avoid reworking strife level but should remove the copy.
		temp = stbi_loadf(path.c_str(), &tw, &th, &tn, 4);
		std::vector<float> result(temp, temp + tw * th * tn);

		delete temp;

		return result;
	}
	catch (...)
	{
		if (temp)
			delete temp;

		OutputDebugStringA(("Error loading texture '" + path + "' \n").c_str());
		return std::vector<float>();
	}
}



bool Texture::LoadFromMemory(const unsigned char* data, size_t size)
{
	try
	{
		int fileFormat;
		int desiredFormat = GetFormatFromMemory(data, size);
		_mdata = std::shared_ptr<unsigned char>(stbi_load_from_memory(data, size, &_w, &_h, &fileFormat, desiredFormat));
		_nc = fileFormat == 3 ? 4 : fileFormat;

		return (_mdata.get() != nullptr);
	}
	catch (...)
	{
		OutputDebugStringA("Error loading texture from memory. \n");
		return false;
	}
}



bool Texture::LoadFromPerlin(ID3D11Device* device, Procedural::Perlin& perlin)
{
	_w = perlin._w;
	_h = perlin._h;
	_nc = 1;
	
	_mdata = std::shared_ptr<unsigned char>(perlin.getUCharVector().data());

	return SetUpAsResource(device);
}



void Texture::LoadWithMipLevels(ID3D11Device* device, ID3D11DeviceContext* context, const std::string& path)
{
	std::wstring temp(path.begin(), path.end());
	const wchar_t* widecstr = temp.c_str();

	HRESULT result = DirectX::CreateWICTextureFromFile(device, context, widecstr, nullptr, &_srv, 0);

	if (FAILED(result))
	{
		OutputDebugStringA("Can't create texture2d with mip levels (WIC). \n");
		exit(4201);
	}
}



bool Texture::SetUpAsResource(ID3D11Device* device) 
{
	DXGI_FORMAT inferredFormat = N_TO_FORMAT_DX11[_nc - 1];

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = _w;
	desc.Height = _h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = inferredFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	// For now always uses 1 byte per channel textures, @todo add byte width flag as well
	UINT pixelWidth = _nc;

	D3D11_SUBRESOURCE_DATA texData;
	texData.pSysMem = (void *)(_mdata.get());
	texData.SysMemPitch = desc.Width * pixelWidth;
	texData.SysMemSlicePitch = 0;

	if (FAILED(device->CreateTexture2D(&desc, &texData, &_dxID)))
	{
		OutputDebugStringA("Can't create texture2d. \n");
		exit(42);
	}


	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = desc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	if (FAILED(device->CreateShaderResourceView(_dxID, &shaderResourceViewDesc, &_srv)))
	{
		OutputDebugStringA("Can't create shader resource view. \n");
		exit(43);
	}

	return true;
}



//for comp: 1=Y, 2=YA, 3=RGB, 4=RGBA 
void Texture::WriteToFile(const std::string& targetFile, int w, int h, int comp, void* data, int stride_in_bytes)
{
	try
	{
		int result = stbi_write_png(targetFile.c_str(), w, h, comp, data, stride_in_bytes);
	}
	catch (...)
	{
		OutputDebugStringA( ("Error writing texture to '" + targetFile + "'; ").c_str() );
	}
}



float Texture::Perlin3D(float x, float  y, float z, UINT xw, UINT yw, UINT zw)
{
	return stb_perlin_noise3(x, y, z, xw, yw, zw);
}



float Texture::Perlin3DFBM(float x, float  y, float z, float lacunarity, float gain, UINT octaves, UINT xw, UINT yw, UINT zw)
{
	return stb_perlin_fbm_noise3(x, y, z, lacunarity, gain, octaves, xw, yw, zw);
}



float Texture::Turbulence3D(float x, float  y, float z, float lacunarity, float gain, UINT octaves, UINT xw, UINT yw, UINT zw)
{
	return stb_perlin_turbulence_noise3(x, y, z, lacunarity, gain, octaves, xw, yw, zw);
}



float Texture::Ridge3D(float x, float  y, float z, float lacunarity, float gain, float offset, UINT octaves, UINT xw, UINT yw, UINT zw)
{
	return stb_perlin_ridge_noise3(x, y, z, lacunarity, gain, offset, octaves, xw, yw, zw);
}



std::vector<float> Texture::generateTurbulent(int w, int h, float z, float lacunarity, float gain, UINT octaves, UINT xw, UINT yw, UINT zw)
{
	//std::vector<unsigned char> result;
	std::vector<float> result;
	
	result.reserve(w * h);

	float wInverse = 1.f / (float)w;
	float hInverse = 1.f / (float)h;

	for (int i = 0; i < w; ++i)
	{
		for (int j = 0; j < h; ++j)
		{
			float x = (float)i * wInverse;
			float y = (float)j * hInverse;

			float noiseVal = Texture::Turbulence3D(x, y, z, lacunarity, gain, octaves, xw, yw, zw);
			result.push_back(noiseVal);
		}
	}

	return result;
}



std::vector<float> Texture::generateRidgey(int w, int h, float z, float lacunarity, float gain, float offset, UINT octaves, UINT xw, UINT yw, UINT zw)
{
	std::vector<float> result;
	result.reserve(w * h);

	float wInverse = 1.f / (float)w;
	float hInverse = 1.f / (float)h;

	for (int i = 0; i < w; ++i)
	{
		for (int j = 0; j < h; ++j)
		{
			float x = (float)i * wInverse;
			float y = (float)j * hInverse;

			float noiseVal = Texture::Ridge3D(x, y, z, lacunarity, gain, offset, octaves, xw, yw, zw);
			result.push_back(noiseVal);
		}
	}

	return result;
}