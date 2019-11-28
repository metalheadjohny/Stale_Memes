#pragma once
#include <vector>
#include <unordered_map>

#include "Resource.h"
#include "Texture.h"
#include "Model.h"
#include "Audio.h"
#include "LevelReader.h"
#include "ProjectLoader.h"


//intended for a level-based game... not going to do open world yet until I understand how to implement streaming well

class ResourceManager
{
	ProjectLoader _projLoader;
	LevelReader _levelReader;

public:
	ResourceManager();
	~ResourceManager();

	void init();
	void pushLevel(int i);
	void popLevel(int i);
	bool loadResource();
	bool removeResource();

	std::unordered_map<std::string, Resource*> _resourceMap;
	ProjectLoader& getProjectLoader() { return _projLoader; }
};