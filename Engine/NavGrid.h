#pragma once
#include "Math.h"
#include "NavGraphTypes.h"
#include "AStar.h"
#include <algorithm>


struct NavCell : public NavNode
{
	SVec3 _direction;
	bool obstructed = false;
	bool _buildable = true;
};



class NavGrid
{
private:
	int _w, _h;
	float _fw, _fh;
	SVec2 _cellSize, _invCellSize;
	SVec3 _offset;
	int _goalIndex;
	float _leeway;
	UINT _activeCellCount;
	std::vector<UINT> _forbiddenCells;

public:
	std::vector<NavCell> _cells;
	std::vector<NavEdge> _edges;

	NavGrid()
	{
		sizeof(NavCell);
		sizeof(NavEdge);
		_w = _h = 0;
		_fw = _fh = 0;
	}



	NavGrid(int w, int h, SVec2 cellSize, SVec3 offset = SVec3()) : _w(w), _h(h), _cellSize(cellSize), _offset(offset)
	{
		_fw = _w * _cellSize.x;
		_fh = _h * _cellSize.y;

		_invCellSize = SVec2(1.f / _cellSize.x, 1.f / _cellSize.y);

		_leeway = _cellSize.Length() * 0.5f;

		_cells.resize(_w * _h);
		_edges.reserve(2 * _w * _h - _w - _h);

		_activeCellCount = _cells.size();
	}



	void setOffset(SVec3 offset)
	{
		_offset = offset;
	}



	//this is a naive implementation that simply adds all edges, does not check for obstacles
	void NavGrid::createAllEdges(float cardinalWeight = 1.f)
	{
		//create and connect edges...
		int thisCell = 0, neighbour = 0, edgeCount = 0;

		float diagonalWeight = sqrt(2) * cardinalWeight;

		//wastes some but likely faster... this could fragment a lot though as _cells is a vector...
		//should use a std::array<int, 8> instead, should be much much better performance wise... however
		//a general use nav mesh node NEEDS to support a variable number of edges!!! :\ reee
		for (auto& cell : _cells)
			cell.edges.reserve(8);

		for (int i = 0; i < _h; i++)
		{
			for (int j = 0; j < _w; j++)
			{
				thisCell = i * _w + j;

				//top edge
				if (i != _h - 1)
				{
					neighbour = thisCell + _w;

					_edges.emplace_back(thisCell, neighbour, cardinalWeight);
					_cells[thisCell].edges.push_back(edgeCount);
					_cells[neighbour].edges.push_back(edgeCount);
					++edgeCount;
				}

				//top right diagonal edge
				if (i != _h - 1 && j != _w - 1)
				{
					neighbour = thisCell + _w + 1;

					_edges.emplace_back(thisCell, neighbour, diagonalWeight);
					_cells[thisCell].edges.push_back(edgeCount);
					_cells[neighbour].edges.push_back(edgeCount);
					++edgeCount;
				}

				//right edge
				if (j != _w - 1)
				{
					neighbour = thisCell + 1;

					_edges.emplace_back(thisCell, neighbour, cardinalWeight);
					_cells[thisCell].edges.push_back(edgeCount);
					_cells[neighbour].edges.push_back(edgeCount);
					++edgeCount;
				}

				//bottom right diagonal edge
				if (i != 0  && j != _w - 1)
				{
					neighbour = thisCell - _w + 1;

					_edges.emplace_back(thisCell, neighbour, diagonalWeight);
					_cells[thisCell].edges.push_back(edgeCount);
					_cells[neighbour].edges.push_back(edgeCount);
					++edgeCount;
				}

			}
		}
	}



	void NavGrid::fillFlowField()
	{
		for (int i = 0; i < _cells.size(); ++i)
		{
			SVec3 myPos = cellIndexToPos(i);
			float minPathCost = (std::numeric_limits<float>::max)();
			int bestNIndex = -1;

			for (int j = 0; j < _cells[i].edges.size(); ++j)
			{
				const NavEdge& edge = _edges[_cells[i].edges[j]];
				
				if (!edge.active)
					continue;

				int nIndex = edge.getNeighbourIndex(i);

				const NavCell& neighbour = _cells[nIndex];
				float curPathCost = neighbour.pathWeight;

				if (neighbour.pathWeight < minPathCost)
				{
					minPathCost = neighbour.pathWeight;
					bestNIndex = nIndex;
				}
			}

			//all edges have been examined! assign the best one to the flow field cell and normalize it
			_cells[i]._direction = cellIndexToPos(bestNIndex) - myPos;
			_cells[i]._direction.Normalize();
		}
	}


	
	inline SVec3 snapToCell(const SVec3& pos) const
	{
		return cellIndexToPos(posToCellIndex(pos));
	}



	bool tryAddObstacle(const SVec3& pos)
	{
		int obstacleCellIndex = posToCellIndex(pos);

		if (!_cells[obstacleCellIndex]._buildable || isForbidden(obstacleCellIndex))
			return false;

		std::list<std::pair<int, bool>> backUp;

		if (addObstacle(obstacleCellIndex, backUp))
		{
			if (countReachable() == getActiveCellCount())
			{
				return true;
			}
			else
			{
				removeObstacle(obstacleCellIndex, backUp);
				return false;
			}
		}
		return false;
	}



	UINT countReachable(UINT startIndex = 0u) const
	{
		UINT curNodeIndex = startIndex;
		UINT count = 0u;

		std::vector<bool> visitedList(_cells.size(), false);
		std::list<int> toCheck;
		toCheck.push_back(curNodeIndex);
		
		while (!toCheck.empty())
		{
			curNodeIndex = toCheck.front();
			toCheck.pop_front();

			const NavCell& curCell = _cells[curNodeIndex];

			for (int edgeIndex : curCell.edges)
			{
				const NavEdge& edge = _edges[edgeIndex];
				
				if (!edge.active)
					continue;

				UINT nbrIndex = edge.getNeighbourIndex(curNodeIndex);
				const NavCell& nbr = _cells[nbrIndex];

				if (visitedList[nbrIndex] == false)
				{
					++count;
					visitedList[nbrIndex] = true;
					toCheck.push_back(nbrIndex);
				}
			}
		}
		return count;
	}



	//returns the count of cells NOT covered by obstacles
	UINT getActiveCellCount()
	{
		return _activeCellCount;
	}



	int posToCellIndex(const SVec3& pos) const
	{
		SVec3 offsetFromGrid = pos - _offset;

		offsetFromGrid.x = Math::clamp(0, _fw - .01f, offsetFromGrid.x);
		offsetFromGrid.z = Math::clamp(0, _fh - .01f, offsetFromGrid.z);

		int row = floor(offsetFromGrid.z * _invCellSize.y);
		int column = floor(offsetFromGrid.x * _invCellSize.x);
		return row * _w + column;
	}


	SVec3 cellIndexToPos(int i) const
	{
		int row = i % _w;
		int column = floor(i / _w);

		SVec3 posInGrid(row * _cellSize.x, 0, column * _cellSize.y);
		SVec3 cellCenterOffset(_cellSize.x * .5f, 0, _cellSize.y * .5f);

		return _offset + posInGrid + cellCenterOffset;
	}


	inline SVec3 flowAtIndex(int i) const
	{
		if (!_cells[i].obstructed)
			return _cells[i]._direction;
		else
			return SVec3::Zero;
	}


	inline SVec3 flowObstacleCorrection(const SVec3& pos) const
	{
		int obsIndex = posToCellIndex(pos);
		if (_cells[obsIndex].obstructed)
			return Math::getNormalizedVec3(pos - cellIndexToPos(obsIndex));

		return SVec3::Zero;
	}


	inline SVec3 flowAtPosition(SVec3 pos) const
	{
		return _cells[posToCellIndex(pos)]._direction;
	}


	inline SVec2 getCellSize() const
	{
		return _cellSize;
	}


	inline int getGoalIndex() const
	{
		return _goalIndex;
	}


	inline void setGoalIndex(int gi)
	{
		_goalIndex = gi;
	}


	inline float getLeeway() const
	{
		return _leeway;
	}


	inline void setLeeway(float leeway)
	{
		_leeway = leeway;
	}


	void forbidCell(UINT index)
	{
		_forbiddenCells.push_back(index);
	}


	void allowCell(UINT index)
	{
		_forbiddenCells.erase(std::remove(_forbiddenCells.begin(), _forbiddenCells.end(), index));
	}


private:

	//this could be sorted out better I guess... with a bool in cell or similar... @TODO if necessary
	bool isObstacle(UINT index)
	{
		/*for (int i : _cells[index].edges)
			if (_edges[i].active)
				return false;
		return true;*/
		return _cells[index].obstructed;
	}



	bool addObstacle(UINT index, std::list<std::pair<int, bool>>& backUp)
	{
		if (isObstacle(index))
			return false;

		_cells[index].obstructed = true;	//mark current cell as obstructed

		for (int edgeIndex : _cells[index].edges)
		{
			//_edges[edgeIndex].weight = (std::numeric_limits<float>::max)(); // hmmm, maybe?
			backUp.push_back(std::make_pair(edgeIndex, _edges[edgeIndex].active));
			_edges[edgeIndex].active = false;	//prevents deleting in the vector... list is slower to iterate so this!
		}

		handleDiagonals(index, backUp);
		
		--_activeCellCount;

		return true;
	}


	void removeObstacle(int index, const std::list<std::pair<int, bool>>& backUp)
	{
		_cells[index].obstructed = false;

		for (const std::pair<int, bool>& ib_pair : backUp)
			_edges[ib_pair.first].active = ib_pair.second;

		++_activeCellCount;
	}


	//returns the index of the edge connecting the cells at the two provided indices, returns -1 if none found
	int getEdgeIndexFromCells(UINT first, UINT second)
	{
		for (auto index : _cells[first].edges)
		{
			if (_edges[index].getNeighbourIndex(first) == second)
				return index;
		}
		return -1;
	}



	void obstructEdgeBetween(UINT c1, UINT c2, std::list<std::pair<int, bool>>& backUpList)
	{
		int edgeIndex = getEdgeIndexFromCells(c1, c2);
		backUpList.push_back(std::make_pair(edgeIndex, _edges[edgeIndex].active));
		_edges[edgeIndex].active = false;
	}


	//returns the list of pairs (consisting of indices of edges and their activity status) used to revert 
	//those edges to their previous states if the obstacle is not allowed
	void handleDiagonals(UINT index, std::list<std::pair<int, bool>>& backUpList)
	{
		UINT row = floor(index / _w);
		UINT column = index % _w;

		if (row < _h - 1 && column < _w - 1)
			obstructEdgeBetween(index + 1, index + _w, backUpList);
		
		if (row < _h - 1 && column > 0)
			obstructEdgeBetween(index - 1, index + _w, backUpList);
	
		if (row > 0 && column > 0)
			obstructEdgeBetween(index - 1, index - _w, backUpList);
		
		if (row > 0 && column < _w - 1)
			obstructEdgeBetween(index + 1, index - _w, backUpList);
	}


	bool isForbidden(UINT index) const
	{
		return (std::find(_forbiddenCells.begin(), _forbiddenCells.end(), index) != _forbiddenCells.end());
	}
};


//UINT tr = index + _w + 1;
//if (_cells[tr].obstructed)	//disable edge between right and top

//UINT tl = index + _w - 1;
//if (_cells[tl].obstructed)	//disable edge between left and top

//UINT bl = index - _w - 1;
//if (_cells[bl].obstructed)	//disable edge between left and bottom

//UINT br = index - _w + 1;
//if (_cells[br].obstructed)	//disable edge between right and bottom