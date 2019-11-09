#pragma once
#include <d3d11.h>
#include "Math.h"
#include "Texture.h"
#include <vector>

struct Vert3D 
{
	SVec3 pos;
	SVec2 texCoords;
	SVec3 normal;
	SVec3 tangent;

	Vert3D() {};
	Vert3D(const SVec3& position) : pos(position) {};
};



struct VertBoneData 
{
	UINT ids[4] = { 0, 0, 0, 0 };
	float weights[4] = { 0, 0, 0, 0 };
};



//@TODO jointInfluences might need changing... should see how this works first...
struct BonedVert3D
{
	SVec3 pos;
	SVec2 texCoords;
	SVec3 normal;
	VertBoneData boneData;

	void BonedVert3D::AddBoneData(unsigned int boneID, float weight)
	{
		for (unsigned int i = 0; i < 4; ++i)
		{
			if (boneData.weights[i] <= 0.0f)
			{
				boneData.ids[i] = boneID;
				boneData.weights[i] = weight;
				return;
			}
		}

		// should never get here - more bones than we have space for
		assert(0);
	}
};



class Joint
{
public:

	int index;
	std::string name;
	SMatrix meshToLocalBoneSpaceTransform, aiNodeTransform, globalTransform;
	Joint* parent = nullptr;
	std::vector<Joint*> offspring;

	Joint() {}

	Joint(int index, std::string name, SMatrix offset) 
	{
		this->index = index;
		this->name = name;
		this->meshToLocalBoneSpaceTransform = offset;

		parent = nullptr;
	}
};



struct Colour
{
	unsigned char col[4];

	Colour(int r, int g, int b, int a)
	{
		col[0] = (unsigned char)r;
		col[1] = (unsigned char)g;
		col[2] = (unsigned char)b;
		col[3] = (unsigned char)a;
	}
};



struct ColourHDR
{
	float _r, _g, _b, _a;

	ColourHDR(float r, float g, float b, float a)
	{
		_r = r;
		_g = g;
		_b = b;
		_a = a;
	}
};



static const UINT MAX_BONES = 96;
struct BoneTransformBufferType
{
	std::vector<SMatrix> boneTransforms;
};