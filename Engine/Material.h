#pragma once
#include "Texture.h"
#include "ShaderManager.h"


class Material
{
public:

	int id = 0;

	Texture* diffuse;
	Texture* normalMap;
	Texture* displaceMap;
	Texture* roughnessMap;
	
	//std::vector<Texture*>	make a vector of enum/texture pairs? a map? seems like a waste to have 4 pointers everywhere when mostly it will only use 1...

	float diffusion = .5f;
	float shininess = 8.f;


	Material();
	~Material();
};

