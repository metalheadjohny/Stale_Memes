#pragma once
#include <vector>

struct NavNode
{
	std::vector<int> edges;
	float pathWeight;
	bool visited;
	int pathPredecessor;
};

struct NavEdge
{
	//int li1, li2, ri1, ri2;	//indices in the points vector in both nodes

	//two fields that share this border
	int first;
	int last;
	float weight;

	NavEdge(int f, int l, float w = 1.f) : first(f), last(l), weight(w) {}
};