#pragma once
#include "Math.h"
#include "MeshDataStructs.h"
#include "AnimChannel.h"
#include <string>
#include <vector>
#include <map>

class Bone;


class Animation
{
private:

	std::string _name;
	std::vector<AnimChannel> _channels;

	double _ticks			{ 0. };
	double _ticksPerSecond	{ 0. };
	double _duration		{ 0. };
	double _tickDuration	{ 0. };

public:

	Animation() noexcept;


	Animation(std::string& name, double ticks, double ticksPerSecond, int nc) noexcept;


	void getTransformAtTime(const std::vector<Bone>& bones, const SMatrix& glInvT, float elapsed, std::vector<SMatrix>& vec) const;


	uint32_t getAnimChannelIndex(const char* name) const
	{
		auto it = std::find_if(_channels.begin(), _channels.end(), 
			[&name](const AnimChannel& ac)
			{
				return ac._boneName == name;
			});

		return (it != _channels.end()) ? std::distance(_channels.begin(), it) : static_cast<uint32_t>(~0);
	}


	inline const AnimChannel* getAnimChannel(uint32_t index) const
	{ 
		return index > _channels.size() ? nullptr : &_channels[index];
	}


	inline const std::vector<AnimChannel>* getChannels()
	{
		return &_channels;
	}


	inline void addChannel(const AnimChannel& ac) { _channels.push_back(ac); }

	inline float getNumTicks()			const { return _ticks; };
	inline float getTicksPerSec()		const { return _ticksPerSecond; }
	inline float getTickDuration()		const { return _tickDuration; }
	inline float getDuration()			const { return _duration; }
	inline uint32_t getNumChannels()	const { return _channels.size(); }
	inline std::string getName()		const { return _name; }

	template <typename Archive>
	void serialize(Archive& ar)
	{
		ar(_name, _channels, _ticks, _ticksPerSecond, _duration, _tickDuration);
	}
};