#pragma once
#include "Math.h"
#include <vector>
#include <array>

#include "Geometry.h"
#include "GeometricPrimitive.h"

class Camera;

// To draw the frustum for debugging just draw a uniform box (dimensions depends on API) using (camView * camProj).Invert() as world mat
class Frustum
{
public:
	//left, right, bottom, top, near, far
	std::array<SPlane, 6> _planes;

	float _fov;
	float _ar;
	float _zn;
	float _zf;

	
	Frustum() {}
	Frustum(float fov, float ar, float zn, float zf);
	Frustum(const SMatrix& pm);

	static std::array<SVec3, 8> extractCorners(const SMatrix& vpMat);	// Also works with combined vp matrix to obtain world coordinates
	
	void update(const SMatrix& vpMat);
	std::vector<float> calcSplitDistances(uint8_t n, float minZ, float maxZ) const;
	std::vector<SMatrix> createCascadeProjMatrices(uint8_t n) const;

private:

	//from the fast extraction paper...
	inline void extractPlanesFromMatrix(const SMatrix& vpm)
	{
		//xyzw data representation of a plane, xyz being the normal, w distance to origin

		_planes[0] = { vpm._14 + vpm._11, vpm._24 + vpm._21, vpm._34 + vpm._31, vpm._44 + vpm._41 };
		_planes[1] = { vpm._14 - vpm._11, vpm._24 - vpm._21, vpm._34 - vpm._31, vpm._44 - vpm._41 };
		_planes[2] = { vpm._14 + vpm._12, vpm._24 + vpm._22, vpm._34 + vpm._32, vpm._44 + vpm._42 };
		_planes[3] = { vpm._14 - vpm._12, vpm._24 - vpm._22, vpm._34 - vpm._32, vpm._44 - vpm._42 };
		_planes[4] = { vpm._13, vpm._23, vpm._33, vpm._43 };
		_planes[5] = { vpm._14 - vpm._13, vpm._24 - vpm._23, vpm._34 - vpm._33, vpm._44 - vpm._43 };

		for (SPlane& p : _planes)
			p.Normalize();
	}
};