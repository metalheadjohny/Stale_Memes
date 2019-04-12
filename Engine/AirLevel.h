#pragma once
#include "Level.h"



class AirLevel : public Level
{
public:

	PointLight pointLight;
	Model skybox;
	Model testBall;
	CubeMapper skyboxCubeMapper;

	Model barrens;

	Texture dragonScales;

	AirLevel(Systems& sys) : Level(sys) {};
	void init(Systems& sys);
	void procGen() {};
	void draw(const RenderContext& rc);
	void demolish() { this->~AirLevel(); };
};
