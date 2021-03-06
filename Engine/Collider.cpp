#include "pch.h"
#include "Collider.h"
#include "ColFuncs.h"
#include "GameObject.h"


void Collider::releaseMemory()
{
	for (Hull* hull : _hulls)
	{
		delete hull;
		hull = nullptr;
	}
}


void Collider::deleteAndClearHulls()
{
	releaseMemory();
	_hulls.clear();
}


//each collider has only one parent so this works I guess... could it simply work by default though???
bool Collider::operator==(const Collider& other) const
{
	return _parent == other._parent;
}



void Collider::updateHullPositions()
{
	for (auto& hull : _hulls)
		hull->setPosition(_parent->getPosition());
}



HitResult Collider::Collide(const Collider& other, SVec3& resolutionVector) const
{
	HitResult hitRes;

	for (Hull* hull1 : _hulls)
	{
		for (Hull* hull2 : other._hulls)
		{
			if (BVT == BVT_AABB)
				hitRes = reinterpret_cast<AABB*>(hull1)->intersect(hull2, other.BVT);
			if (BVT == BVT_SPHERE)
				hitRes = reinterpret_cast<SphereHull*>(hull1)->intersect(hull2, other.BVT);

			if (hitRes.hit)
			{
				resolutionVector = hull2->getPosition() - hull1->getPosition();
				resolutionVector.Normalize();
				break;
			}
		}
	}

	return hitRes;
}