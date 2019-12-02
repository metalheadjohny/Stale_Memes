#include "TDLevel.h"
#include "Terrain.h"
#include "Geometry.h"

#define DEBUG_OCTREE

void TDLevel::init(Systems& sys)
{
	skybox.LoadModel(device, "../Models/Skysphere.fbx");
	skyboxCubeMapper.LoadFromFiles(device, "../Textures/day.dds");

	LightData lightData(SVec3(0.1, 0.7, 0.9), .03f, SVec3(0.8, 0.8, 1.0), .2, SVec3(0.3, 0.5, 1.0), 0.7);
	pLight = PointLight(lightData, SVec4(0, 500, 0, 1));

	float tSize = 200;
	Procedural::Terrain t(2, 2, SVec3(tSize));
	t.setOffset(-tSize * .5f, -0.f, -tSize * .5f);
	t.SetUp(device);
	floorModel = Model(t, device);

	//NodeBase* wat = _sg.insert(pLight);		//decide how to handle node ownership here... 
	//_sg.insert(floorModel);

	_oct.init(AABB(SVec3(), SVec3(100)), 3);
	_oct.prellocateRootOnly();
	//_oct.preallocateTree();	//with 5 it is reaaallly big

	boiModel.LoadModel(device, "../Models/Skysphere.fbx");

	Procedural::Geometry g;
	
	g.GenBox(SVec3(1));

	debugModel.meshes.push_back(Mesh(g, device));

	Actor actor;

	for (int i = 0; i < 125; ++i)
	{
		SVec3 pos = SVec3(i % 5, (i / 5) % 5, (i / 25) % 5) * 20.f + SVec3(5.f);
		dubois.emplace_back(SMatrix::CreateTranslation(pos), GraphicComponent(&boiModel, &randy._shMan.light));
		dubois[i].collider = new Collider();
		dubois[i].collider->BVT = BVT_SPHERE;
		dubois[i].collider->dynamic = true;
		dubois[i].collider->hulls.push_back(new SphereHull(pos, 1));	//eliminate duplicate pos, redundant and pain to update

		_oct.insertObject(static_cast<SphereHull*>(dubois[i].collider->hulls.back()));
	}

	
	tempBoxes.reserve(1000);
	octNodeMatrices.reserve(1000);
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
	updateCam(rc.dTime);

	for (auto& boi : dubois)
	{
		for (auto& hull : boi.collider->hulls)
			hull->setPosition(boi.getPosition());
	}

	_oct.updateAll();
	_oct.lazyTrim();
	//this works well to reduce the number of checked branches with simple if(null) but only profiling
	//can tell if it's better this way or by just leaving them allocated (which means deeper checks, less allocations)
}



void TDLevel::draw(const RenderContext& rc)
{
	rc.d3d->ClearColourDepthBuffers();
	rc.d3d->TurnOffCulling();

	shady.light.SetShaderParameters(context, floorModel.transform, *rc.cam, pLight, rc.dTime);
	floorModel.Draw(context, shady.light);
	shady.light.ReleaseShaderParameters(context);

	auto www = _sys._resMan.getResourceByName("FlyingMage");
	Model* m = static_cast<Model*> (www);

	SMatrix rotMatrix = SMatrix::CreateFromAxisAngle(SVec3(0, 1, 0), .1 * rc.dTime);

	shady.light.SetShaderParameters(context, m->transform, *rc.cam, pLight, rc.dTime);
	m->Draw(context, shady.light);
	shady.light.ReleaseShaderParameters(context);

	randy.RenderSkybox(*rc.cam, skybox, skyboxCubeMapper);

	for (int i = 0; i < dubois.size(); ++i)
	{
		dubois[i].transform = dubois[i].transform * rotMatrix;
		dubois[i].Draw(context, *rc.cam, pLight, rc.dTime);
	}
	
#ifdef DEBUG_OCTREE
	shady.instanced.SetShaderParameters(context, debugModel, *rc.cam, pLight, rc.dTime);
	debugModel.DrawInstanced(context, shady.instanced);
	shady.instanced.ReleaseShaderParameters(context);
#endif

	//renderString("FPS", std::string("FPS: " + std::to_string(1 / rc.dTime)));
	renderString("Octree", std::string("OCT node count " + std::to_string(_oct.getNodeCount())));

	rc.d3d->EndScene();
}