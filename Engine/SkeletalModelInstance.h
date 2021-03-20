#pragma once
#include "SkeletalModel.h"



class SkeletalModelInstance
{
public:

	SkeletalModel* _skm;
	SMatrix _transform;

	CBuffer _skMatsBuffer;
	std::vector<SMatrix> _skeletonMatrices;

	std::vector<AnimationInstance> _animInstances;

	SkeletalModelInstance() : _skm(nullptr) {}

	bool init(ID3D11Device* device, SkeletalModel* skm)
	{
		_skm = skm;

		for (Animation* anim : skm->_anims)
			_animInstances.emplace_back(anim);

		UINT numBones = 144;

		_skMatsBuffer.init(device, CBuffer::createDesc(sizeof(SMatrix) * numBones));

		// Jingle bells, code smells... Law of Demeter RIP
		//_skeletonMatrices.resize(_skm->_skeleton->_boneMap.size());
		_skeletonMatrices.resize(_skm->_skeleton->_bones.size());
		return true;
	}


	// This is bad but temporary
	void update(float dTime, UINT animIndex = 0u)
	{
		for (int i = 0; i < _animInstances.size(); ++i)
			_animInstances[i].update(dTime);

		for (int i = 0; i < _skm->_meshes.size(); ++i)
		{
			_skm->_meshes[i]._transform = _skm->_meshes[i]._localTransform * _transform;
		}

		if (_animInstances.size() > animIndex)	// Avoid crashing when no anim is loaded
		{
			_animInstances[animIndex].getTransformAtTime(_skm->_skeleton->_bones[0], _skeletonMatrices, SMatrix::Identity, _skm->_skeleton->_globalInverseTransform);
		}
		else
		{
			for (SMatrix& mat : _skeletonMatrices)
				mat = SMatrix::Identity;
		}

		for (SMatrix& mat : _skeletonMatrices)
			mat = mat.Transpose();
	}



	void draw(ID3D11DeviceContext* context)
	{
		_skMatsBuffer.update(context, _skeletonMatrices.data(), sizeof(SMatrix) * _skm->_skeleton->_bones.size());
		_skMatsBuffer.bindToVS(context, 1);

		for (SkeletalMesh& m : _skm->_meshes)
		{
			m.draw(context);
		}
	}
};