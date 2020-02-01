#pragma once
#include <d3d11.h>
#include <vector>
#include "RenderState.h"


//used to check which state entries need to be updated and which stay the same, stores the last used state
class RenderStateManager
{
	RenderState _gpuState;

public:

	RenderState _current;

	//@TODO make this replace the majority of renderer's render
	void deltaUpdate(RenderState next)
	{
		//if state x not value y, update to value y and set current to new... for each state
	}

	void applyToGPU()
	{
		//...set all the states to gpu state if not different
	}

};