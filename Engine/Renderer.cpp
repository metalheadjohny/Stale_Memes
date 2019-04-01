#include "Renderer.h"
#include "InputManager.h"

Renderer::Renderer()
{
	_D3D = 0;
	drawUI = false;
}


Renderer::~Renderer() {}

#define RES _resMan._level1
#define EYE_POS _cam.GetCameraMatrix().Translation()

bool Renderer::Initialize(int windowWidth, int windowHeight, HWND hwnd, InputManager& inMan)
{
	_D3D = new D3D;
	if(!_D3D)
		return false;

	if(!_D3D->Initialize(windowWidth, windowHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR))
	{
		MessageBox(hwnd, L"Could not initialize Direct3D.", L"Error", MB_OK);
		return false;
	}

	_device = _D3D->GetDevice();
	_deviceContext = _D3D->GetDeviceContext();



	_inMan = &inMan;

	_colEngine.init(_device, _deviceContext);

	shMan.init(_device, hwnd);
	_resMan.init(_device);

	RES.pSys.setShader(&shMan.shaderBase);

	_rekt = new Rekt(_device, _deviceContext);
	screenRect = _rekt->AddUINODE(_rekt->getRoot(), SVec2(0.75f, 0.75f), SVec2(0.25f, 0.25f));

	RES.maze.Init(10, 10, 32.f);
	RES.maze.CreateModel(_device);

	_colEngine.registerModel(&(RES.maze.model), BVT_AABB);

	///CAMERA INITIALISATION
	SMatrix projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(_D3D->_fieldOfView, _D3D->_screenAspect, SCREEN_NEAR, SCREEN_DEPTH);

	_cam = Camera(SMatrix::Identity, projectionMatrix);
	_controller = Controller(&inMan);
	_cam._controller = &_controller;

	_colEngine.registerController(_controller);	//works both ways

	return true;
}


bool Renderer::Frame(float dTime)
{
	ProcessSpecialInput();
	elapsed += dTime;

	if (!_controller.isFlying())
	{
		SVec3 oldPos = EYE_POS;
		float newHeight = RES.proceduralTerrain.getHeightAtPosition(EYE_POS);
		SMatrix newMat = _cam.GetCameraMatrix();
		Math::SetTranslation(newMat, SVec3(oldPos.x, newHeight, oldPos.z));
		_cam.SetCameraMatrix(newMat);
	}

	_cam.update(dTime);

	Math::SetTranslation(RES.modSkybox.transform, _cam.GetCameraMatrix().Translation());

	OutputFPS(dTime);

	return RenderFrame(dTime);
}



bool Renderer::RenderFrame(float dTime)
{
	ParticleUpdateData pud = { SVec3(-5, 2, 5), 1.f, dTime };	//wind direction, wind velocity multiplier and delta time
	RES.pSys.updateStdFunc(&pud);

	_deviceContext->RSSetViewports(1, &_D3D->viewport);	//use default viewport for output dimensions
	_D3D->SetBackBufferRenderTarget();					//set default screen buffer as output target
	_D3D->BeginScene(clearColour);						//clear colour and depth buffer

	///RENDERING OLD TERRAIN 
	/*
	for (auto tm : _terrainModels) {
		shaderShadow.SetShaderParameters(_deviceContext, *tm, _cam.GetViewMatrix(), offScreenTexture._view, _cam.GetProjectionMatrix(),
			offScreenTexture._lens, pointLight, _cam.GetCameraMatrix().Translation(), offScreenTexture.srv);
		tm->Draw(_deviceContext, shaderShadow);
		shaderShadow.ReleaseShaderParameters(_deviceContext);
	}
	*/

	_D3D->TurnOffCulling();
	_D3D->SwitchDepthToLessEquals();

	shMan.shaderSkybox.SetShaderParameters(_deviceContext, RES.modSkybox, _cam.GetViewMatrix(), _cam.GetProjectionMatrix(),
		_cam.GetCameraMatrix().Translation(), dTime, RES.skyboxCubeMapper.cm_srv);
	RES.modSkybox.Draw(_deviceContext, shMan.shaderSkybox);
	shMan.shaderSkybox.ReleaseShaderParameters(_deviceContext);

	_D3D->SwitchDepthToDefault();
	_D3D->TurnOnCulling();

	if (RES.isTerGenerated) 
	{

		RES.proceduralTerrain.Draw(_deviceContext, shMan.shaderTerrain,
			SMatrix(), _cam.GetViewMatrix(), _cam.GetProjectionMatrix(),
			RES.pointLight, elapsed, _cam.GetCameraMatrix().Translation());

		shMan.shaderTree.SetShaderParameters(_deviceContext, RES.treeModel.transform,
			_cam, RES.pointLight, elapsed);
		RES.treeModel.Draw(_deviceContext, shMan.shaderLight);
		shMan.shaderLight.ReleaseShaderParameters(_deviceContext);
		
	}

	shMan.shaderMaze.SetShaderParameters(_deviceContext, RES.maze.model, _cam, RES.pointLight, elapsed, RES.mazeDiffuseMap, RES.mazeNormalMap);
	RES.maze.model.Draw(_deviceContext, shMan.shaderMaze);
	shMan.shaderMaze.ReleaseShaderParameters(_deviceContext);

	/*
 	std::vector<InstanceData> instanceData(100);

	for (int i = 0; i < instanceData.size(); ++i)
		instanceData[i]._m = pSys._particles[i]->transform.Transpose();

	shMan.shaderInstanced.UpdateInstanceData(instanceData);
	shMan.shaderInstanced.SetShaderParameters(&shMan.spl);
	RES.modBall.Draw(_deviceContext, shMan.shaderInstanced);
	shMan.shaderInstanced.ReleaseShaderParameters(_deviceContext);
	*/

	_D3D->TurnOnAlphaBlending();

	//shMan.shVolumFire.SetShaderParameters(_deviceContext, RES.will, _cam, elapsed);
	//RES.will.Draw(_deviceContext, shMan.shVolumFire);

	shMan.shVolumAir.SetShaderParameters(_deviceContext, RES.will, _cam, elapsed);
	RES.will.Draw(_deviceContext, shMan.shVolumAir);

	_D3D->TurnOffAlphaBlending();

	_D3D->EndScene();
	return true;
}



void Renderer::Shutdown() 
{
	if (_D3D) 
	{
		_D3D->Shutdown();
		delete _D3D;
		_D3D = 0;
	}
}



void Renderer::OutputFPS(float dTime) 
{
	std::ostringstream ss;
	ss << "Frame time: " << 1.0f / dTime << "\n";
	std::string s(ss.str());
	OutputDebugStringA(ss.str().c_str());
}



void Renderer::ProcessSpecialInput() 
{
	if (_inMan->IsKeyDown(VK_SPACE))
		RES.procGen(_device);

	if(_inMan->IsKeyDown((short)'F'))
		_controller.toggleFly();
}



#pragma region oldScene

/*
///PROJECT TEXTURE
SMatrix texView = DirectX::XMMatrixLookAtLH(SVec3(0.0f, 0.0f, -1.0f), SVec3(0.0f, 0.0f, 0.0f), SVec3::Up);
shaderPT.SetShaderParameters(_deviceContext, modTerrain, cam.GetViewMatrix(), cam.GetViewMatrix(), cam.GetProjectionMatrix(),
							cam.GetProjectionMatrix(), _lights[0], cam.GetCameraMatrix().Translation(), dTime, offScreenTexture.srv);
modTerrain.Draw(_deviceContext, shaderPT);
shaderPT.ReleaseShaderParameters(_deviceContext);
*/

/*

///REFLECTION CUBE MAP START
_deviceContext->RSSetViewports(1, &(cubeMapper.cm_viewport));
cubeMapper.UpdateCams(modBall.transform.Translation());

SMatrix wot = DirectX::XMMatrixInverse(nullptr, cubeMapper.cameras[0]);
Math::SetTranslation(modSkybox.transform, wot.Translation());
for (int i = 0; i < 6; i++) {

	_deviceContext->ClearRenderTargetView(cubeMapper.cm_rtv[i], cubeMapper.clearCol);
	_deviceContext->ClearDepthStencilView(cubeMapper.cm_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	_deviceContext->OMSetRenderTargets(1, &cubeMapper.cm_rtv[i], cubeMapper.cm_depthStencilView);

	shaderLight.SetShaderParameters(_deviceContext, modTerrain, cubeMapper.cameras[i], cubeMapper.lens, pointLight, _cam.GetCameraMatrix().Translation(), dTime);
	modTerrain.Draw(_deviceContext, shaderLight);
	shaderLight.ReleaseShaderParameters(_deviceContext);

	shaderLight.SetShaderParameters(_deviceContext, modTreehouse, cubeMapper.cameras[i], cubeMapper.lens, pointLight, _cam.GetCameraMatrix().Translation(), dTime);
	modTreehouse.Draw(_deviceContext, shaderLight);
	shaderLight.ReleaseShaderParameters(_deviceContext);

	_D3D->TurnOffCulling();
	_D3D->SwitchDepthToLessEquals();


	shaderSkybox.SetShaderParameters(_deviceContext, modSkybox, cubeMapper.cameras[i], cubeMapper.lens,
		_cam.GetCameraMatrix().Translation(), dTime, skyboxCubeMapper.cm_srv);
	modSkybox.Draw(_deviceContext, shaderSkybox);
	shaderSkybox.ReleaseShaderParameters(_deviceContext);

	_D3D->SwitchDepthToDefault();
	_D3D->TurnOnCulling();
}
Math::SetTranslation(modSkybox.transform, _cam.GetCameraMatrix().Translation());
///REFLECTION CUBE MAP DONE


///RENDERING UI
_deviceContext->RSSetViewports(1, &_D3D->viewport);
_D3D->SetBackBufferRenderTarget();
_D3D->BeginScene(clearColour);

if (drawUI)
_rekt->draw(_deviceContext, shaderHUD, offScreenTexture.srv);
///RENDERING UI DONE


///RENDERING DEPTH TEXTURE
_deviceContext->RSSetViewports(1, &altViewport);	//to the shadow texture viewport
_deviceContext->OMSetRenderTargets(1, &(offScreenTexture.rtv), _D3D->GetDepthStencilView());	//switch to drawing on ost for the prepass	
_deviceContext->ClearRenderTargetView(offScreenTexture.rtv, ccb);	//then clear it, both the colours and the depth-stencil buffer
_deviceContext->ClearDepthStencilView(_D3D->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

for (auto tm : _terrainModels) {
	shaderDepth.SetShaderParameters(_deviceContext, *tm, offScreenTexture._view, offScreenTexture._lens);
	tm->Draw(_deviceContext, shaderDepth);
}

shaderLight.SetShaderParameters(_deviceContext, modBall, offScreenTexture._view, offScreenTexture._lens, pointLight,
	_cam.GetCameraMatrix().Translation(), dTime);
modBall.Draw(_deviceContext, shaderLight);
shaderLight.ReleaseShaderParameters(_deviceContext);

///RENDERING TERRAIN
_deviceContext->ClearDepthStencilView(_D3D->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
_deviceContext->RSSetViewports(1, &_D3D->viewport);
_D3D->SetBackBufferRenderTarget();

for (auto tm : _terrainModels) {
	shaderShadow.SetShaderParameters(_deviceContext, *tm, _cam.GetViewMatrix(), offScreenTexture._view, _cam.GetProjectionMatrix(),
		offScreenTexture._lens, pointLight, _cam.GetCameraMatrix().Translation(), offScreenTexture.srv);
	tm->Draw(_deviceContext, shaderShadow);
	shaderShadow.ReleaseShaderParameters(_deviceContext);
}
///RENDERING TERRAIN DONE

///RENDERING WIREFRAME
_D3D->TurnOnAlphaBlending();
shaderWireframe.SetShaderParameters(_deviceContext, modBallStand, _cam.GetViewMatrix(), _cam.GetProjectionMatrix());
modBallStand.Draw(_deviceContext, shaderWireframe);
shaderWireframe.ReleaseShaderParameters(_deviceContext);
_D3D->TurnOffAlphaBlending();
///RENDERING WIREFRAME DONE




///RENDERING REFLECTION SPHERE/*
shaderCM.SetShaderParameters(_deviceContext, modBall, _cam.GetViewMatrix(), _cam.GetProjectionMatrix(), dirLight,
	_cam.GetCameraMatrix().Translation(), dTime, cubeMapper.cm_srv);
modBall.Draw(_deviceContext, shaderCM);
shaderCM.ReleaseShaderParameters(_deviceContext);
///RENDERING REFLECTION SPHERE DONE



///RENDERING SKYBOX
_D3D->TurnOffCulling();
_D3D->SwitchDepthToLessEquals();
shaderSkybox.SetShaderParameters(_deviceContext, modSkybox, _cam.GetViewMatrix(), _cam.GetProjectionMatrix(),
	_cam.GetCameraMatrix().Translation(), dTime, skyboxCubeMapper.cm_srv);
modSkybox.Draw(_deviceContext, shaderSkybox);
shaderSkybox.ReleaseShaderParameters(_deviceContext);
_D3D->SwitchDepthToDefault();
_D3D->TurnOnCulling();
///RENDERING SKYBOX DONE


_D3D->TurnOnAlphaBlending();





_D3D->TurnOffAlphaBlending();

*/

#pragma endregion oldScene