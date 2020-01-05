#include "TDLevel.h"
#include "Terrain.h"
#include "Geometry.h"
#include "AStar.h"
#include "Picker.h"
#include "ColFuncs.h"
#include "Shader.h"
#include "Steering.h"

//#define DEBUG_OCTREE


inline float pureDijkstra(const NavNode& n1, const NavNode& n2) { return 0.f; }


void TDLevel::init(Systems& sys)
{
	S_INMAN.registerController(&_tdController);
	skyboxCubeMapper.LoadFromFiles(S_DEVICE, "../Textures/day.dds");

	_tdgui.init(ImVec2(_sys.getScrW() - 500, _sys.getScrH() - 200), ImVec2(500, 200));

	LightData lightData(SVec3(0.1, 0.7, 0.9), .03f, SVec3(0.8, 0.8, 1.0), .2, SVec3(0.3, 0.5, 1.0), 0.7);
	pLight = PointLight(lightData, SVec4(0, 500, 0, 1));

	float tSize = 500;
	terrain = Procedural::Terrain(2, 2, SVec3(tSize));
	terrain.setOffset(-tSize * .5f, -0.f, -tSize * .5f);
	terrain.SetUp(S_DEVICE);
	floorModel = Model(terrain, S_DEVICE);

	_octree.init(AABB(SVec3(), SVec3(tSize * .5)), 4);	//with depth 5 it's really big, probably not worth it for my game
	_octree.prellocateRootOnly();						//_oct.preallocateTree();	

	_navGrid = NavGrid(10, 10, SVec2(50.f), terrain.getOffset());
	_navGrid.createAllEdges();
	AStar<pureDijkstra>::fillGraph(_navGrid._cells, _navGrid._edges, GOAL_INDEX);
	_navGrid.setGoalIndex(GOAL_INDEX);
	_navGrid.fillFlowField();

	creeps.reserve(NUM_ENEMIES);
	for (int i = 0; i < NUM_ENEMIES; ++i)
	{
		//float offset = (i % 10) * ((i % 2) * 2 - 1);
		SVec3 pos = SVec3(200, 0, 200) + 5 * SVec3(i % 10, 0, (i / 10) % 10);

		creeps.emplace_back(S_RESMAN.getByName<Model*>("FlyingMage"), SMatrix::CreateTranslation(pos));
		
		for (Renderable& r : creeps[i].renderables)
		{
			r.mat = S_MATCACHE.getMaterial("creepMat");
			r.pLight = &pLight;
		}

		_octree.insertObject(static_cast<SphereHull*>(creeps[i]._collider->hulls.back()));
	}

	selectBuilding("GuardTower");

	_tdgui.addBuildingGuiDef(
		"Guard tower is a common, yet powerful defensive building.", 
		"Guard tower", 
		S_RESMAN.getByName<Texture>("guard_tower")->srv);

	_tdgui.addBuildingGuiDef(
		"Produces 10 wood per minute. Time to get lumber-jacked.",
		"Lumberyard",
		S_RESMAN.getByName<Texture>("lumber_yard")->srv);


	_eco.createResource("Coin", 1000);
	_eco.createResource("Wood", 1000);

#ifdef DEBUG_OCTREE
	Procedural::Geometry g;
	g.GenBox(SVec3(1));
	debugModel.meshes.push_back(Mesh(g, S_DEVICE));
	tempBoxes.reserve(1000);
	octNodeMatrices.reserve(1000);
#endif
}



void TDLevel::update(const RenderContext& rc)
{
#ifdef DEBUG_OCTREE
	_oct.getTreeAsAABBVector(tempBoxes);

	for (int i = 0; i < tempBoxes.size(); ++i)
	{
		octNodeMatrices.push_back(
			(
				SMatrix::CreateScale(tempBoxes[i].getHalfSize() * 2.f) *
				SMatrix::CreateTranslation(tempBoxes[i].getPosition())
				).Transpose()
		);
	}

	shady.instanced.UpdateInstanceData(octNodeMatrices);
	octNodeMatrices.clear();
	tempBoxes.clear();
#endif

	ProcessSpecialInput(rc.dTime);
	

	//this works well to reduce the number of checked branches with simple if(null) but only profiling
	//can tell if it's better this way or by just leaving them allocated (which means deeper checks, but less allocations)
	//Another alternative is having a bool empty; in the octnode...
	_octree.updateAll();	//@TODO redo this, does redundant work and blows in general
	_octree.lazyTrim();
	_octree.collideAll();
	

	//not known to individuals as it depends on group size, therefore should not be in a unit component I'd say... 
	SVec2 stopArea(sqrt(creeps.size()), sqrt(creeps.size()));
	stopArea *= 3.f;
	float stopDistance = stopArea.Length();


	for (int i = 0; i < creeps.size(); ++i)
	{
		//pathfinding and steering, needs to turn off once nobody is moving...

		if (creeps[i]._steerComp._active)
		{
			std::list<Actor*> neighbourCreeps;	//this should be on the per-frame allocator
			_octree.findWithin(creeps[i].getPosition(), 4.f, neighbourCreeps);
			creeps[i]._steerComp.update(_navGrid, rc.dTime, neighbourCreeps, i, stopDistance);
		}

		//height
		float h = terrain.getHeightAtPosition(creeps[i].getPosition());
		float intervalPassed = fmod(rc.elapsed * 5.f + i * 2.f, 10.f);
		float sway = intervalPassed < 5.f ? Math::smoothstep(0, 5, intervalPassed) : Math::smoothstep(10, 5, intervalPassed);
		Math::setHeight(creeps[i].transform, h + 2 * sway + FLYING_HEIGHT);

		//propagate transforms to children
		creeps[i].propagate();
	}

	_selectedBuilding.propagate();

	rayPick(rc.cam);

	/// FRUSTUM CULLING
	numCulled = 0;
	const SMatrix v = rc.cam->GetViewMatrix();
	const SVec3 v3c(v._13, v._23, v._33);
	const SVec3 camPos = rc.cam->GetPosition();
	
	
	//cull and add to render queue
	for (int i = 0; i < creeps.size(); ++i)
	{
		if(Col::FrustumSphereIntersection(rc.cam->frustum, *static_cast<SphereHull*>(creeps[i]._collider->hulls[0])))
		{
			float zDepth = (creeps[i].transform.Translation() - camPos).Dot(v3c);
			for (auto& r : creeps[i].renderables)
			{
				r.zDepth = zDepth;
				S_RANDY.addToRenderQueue(r);
			}
		}
		else
		{
			numCulled++;
		}
	}

	handleInput();

}



void TDLevel::draw(const RenderContext& rc)
{
	rc.d3d->ClearColourDepthBuffers();
	rc.d3d->setRSSolidNoCull();

	S_SHADY.light.SetShaderParameters(S_CONTEXT, floorModel.transform, *rc.cam, pLight, rc.dTime);
	floorModel.Draw(S_CONTEXT, S_SHADY.light);
	S_SHADY.light.ReleaseShaderParameters(S_CONTEXT);

#ifdef DEBUG_OCTREE
	shady.instanced.SetShaderParameters(context, debugModel, *rc.cam, pLight, rc.dTime);
	debugModel.DrawInstanced(context, shady.instanced);
	shady.instanced.ReleaseShaderParameters(context);
#endif

	
	S_RANDY.sortRenderQueue();
	S_RANDY.flushRenderQueue();
	S_RANDY.clearRenderQueue();
	
	S_RANDY.renderSkybox(*rc.cam, *(S_RESMAN.getByName<Model>("Skysphere")), skyboxCubeMapper);

	if (_building)
		_selectedBuilding.render(S_RANDY);
	
	for (Actor& building : _built)
	{
		building.render(S_RANDY);
	}



	std::vector<GuiElement> guiElems =
	{
		{"Octree",	std::string("OCT node count " + std::to_string(_octree.getNodeCount()))},
		{"FPS",		std::string("FPS: " + std::to_string(1 / rc.dTime))},
		{"Culling", std::string("Objects culled:" + std::to_string(numCulled))}
	};

	startGuiFrame();
	renderGuiElems(guiElems);

	static int selectedTower = 0;
	_tdgui.renderBuildingWidget(_building, selectedTower);

	endGuiFrame();

	rc.d3d->EndScene();
}



void TDLevel::demolish()
{
	finished = true;
}



void TDLevel::rayPick(Camera* cam)
{
	MCoords mc = _sys._inputManager.getAbsXY();
	SRay ray = Picker::generateRay(_sys.getWinW(), _sys.getWinH(), mc.x, mc.y, *cam);

	//intersect base plane... for now, can do terrain as well but it's slower, no need yet
	SVec3 POI;
	ray.direction *= 500.f;
	Col::RayPlaneIntersection(ray, SVec3(0, 0, 0), SVec3(1, 0, 0), SVec3(0, 0, 1), POI);

	if (_building)
	{
		SVec3 snappedPos = _navGrid.snapToCell(POI);
		Math::SetTranslation(_selectedBuilding.transform, snappedPos);
	}

	if(S_INMAN.isKeyDown('R'))
		for (int i = 0; i < creeps.size(); ++i)
		{
			Math::SetTranslation(creeps[i].transform, SVec3(200, 0, 200) + 5 * SVec3(i % 10, 0, (i / 10) % 10));
			creeps[i]._steerComp._active = true;
		}
}



void TDLevel::handleInput()
{
	//check if the spot is taken - using the nav grid, only clear cells can do! and update the navgrid after

	InputEventTD inEvent;
	
	while (_tdController.consumeNextAction(inEvent))
	{
		if (inEvent == InputEventTD::BUILD && _building)
		{
			if (_navGrid.tryAddObstacle(_selectedBuilding.getPosition()))
			{
				AStar<pureDijkstra>::fillGraph(_navGrid._cells, _navGrid._edges, GOAL_INDEX);
				_navGrid.fillFlowField();
				_built.push_back(_selectedBuilding);
				_building = false;
			}
			else
			{
				//detected path blocking, can't build, pop some gui warning etc...
			}
		}
		else if (inEvent == InputEventTD::STOP_BUILDING)
		{
			_building = false;
		}
	}
}



void TDLevel::selectBuilding(const std::string& name)
{
	_selectedBuilding = Actor(S_RESMAN.getByName<Model>(name));
	_selectedBuilding.transform = SMatrix::CreateScale(.33);
	for (Renderable& r : _selectedBuilding.renderables)
	{
		r.mat->setVS(_sys._shaderCache.getVertShader("basicVS"));
		r.mat->setPS(_sys._shaderCache.getPixShader("lightPS"));
		r.pLight = &pLight;		//this is awkward and I don't know how to do it properly right now...
	}
}





/* old sphere placement, it's here because it's rad
for (int i = 0; i < 125; ++i)
{
	SVec3 pos = SVec3(i % 5, (i / 5) % 5, (i / 25) % 5) * 20.f + SVec3(5.f);
}*/

/*
Procedural::Geometry g;
g.GenBox(SVec3(_navGrid.getCellSize().x, 1, _navGrid.getCellSize().y));
boxModel.meshes.push_back(Mesh(g, S_DEVICE, true, false));
box = Actor(SMatrix(), &boxModel);
box.renderables[0].mat = &creepMat;
box.renderables[0].pLight = &pLight;
*/