#pragma once
#include <unordered_map>
#include <string>
#include "ResourceHandle.h"
#include "ResourceDef.h"

// Holds handles to assets
class ResourceLedger
{
	std::unordered_map<std::string, ResourceDef> _resMap;
	
};