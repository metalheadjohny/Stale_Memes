#include "Renderer.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "GameObject.h"


Renderer::Renderer()
{
	//sAlloc.init(sizeof(Renderable) * MAX_RENDERABLES);
	renderables.reserve(MAX_RENDERABLES);
}



Renderer::~Renderer() {}



bool Renderer::Initialize(int windowWidth, int windowHeight, HWND hwnd, ResourceManager& resMan, D3D& d3d, Controller& ctrl)
{
	_d3d = &d3d;
	_resMan = &resMan;
	
	_device = d3d.GetDevice();
	_deviceContext = d3d.GetDeviceContext();

	_shMan.init(_device, hwnd);

	// Setup the projection matrix.
	_fieldOfView = PI / 3.0f;
	_screenAspect = (float)windowWidth / (float)windowHeight;

	_cam = Camera(SMatrix::Identity, DirectX::XMMatrixPerspectiveFovLH(_fieldOfView, _screenAspect, SCREEN_NEAR, SCREEN_DEPTH));
	_cam._controller = &ctrl;

	return true;
}



bool Renderer::Frame(float dTime, InputManager* inMan)
{
	elapsed += dTime;
	
	bool res = UpdateRenderContext(dTime);

	return res;
}



void Renderer::setCameraMatrix(const SMatrix& camMatrix)
{
	_cam.SetCameraMatrix(camMatrix);
}



bool Renderer::UpdateRenderContext(float dTime)
{
	rc.cam = &_cam;
	rc.d3d = _d3d;
	rc.dTime = dTime;
	rc.elapsed = elapsed;
	rc.shMan = &_shMan;

	return true;
}



void Renderer::SetOSTRenderTarget(OST& ost)
{
	ost.SetRenderTarget(_deviceContext);
}



void Renderer::RevertRenderTarget()
{
	_d3d->SetBackBufferRenderTarget();
}



void Renderer::RenderSkybox(const Camera& cam, Model& skybox, const CubeMapper& skyboxCubeMapper) 
{
	_d3d->TurnOffCulling();
	_d3d->SwitchDepthToLessEquals();
	_shMan.skyboxShader.SetShaderParameters(_deviceContext, cam, rc.dTime, skyboxCubeMapper.cm_srv);
	skybox.Draw(_deviceContext, _shMan.skyboxShader);
	rc.shMan->skyboxShader.ReleaseShaderParameters(_deviceContext);
	_d3d->SwitchDepthToDefault();
	_d3d->TurnOnCulling();
}



void Renderer::addToRenderQueue(const Renderable& renderable)
{
	renderables.push_back(renderable);	//stack allocator could work, or just reserving... not sure really
}



void Renderer::clearRenderQueue()
{
	renderables.clear();
}



void Renderer::sortRenderQueue()
{
	std::sort(renderables.begin(), renderables.end());	//determine how to sort by overloading < for renderable
}



void Renderer::flushRenderQueue()
{

}



//mind all the pointers this can fail spectacularly if anything relocates...
//not for instancing... this is just a low level draw
void Renderer::render(Renderable& r)
{
	unsigned int stride = r.mat->rFormat.stride;
	unsigned int offset = r.mat->rFormat.offset;

	//extract to sort by, won't be very uniform... tex arrays can help though...
	for (int i = 0; i < r.mat->textures.size(); ++i)
		_deviceContext->PSSetShaderResources(r.mat->texturesAdded + i, 1, &(r.mat->textures[i]->srv));

	//extract to sort by... should be fairly uniform though
	_deviceContext->IASetPrimitiveTopology(r.mat->rFormat.primitiveTopology);

	//these have to change each time unless I'm packing multiple meshes per buffer... can live with that tbh
	_deviceContext->IASetVertexBuffers(0, 1, &(r.mesh->_vertexBuffer), 0, 0);
	_deviceContext->IASetIndexBuffer(r.mesh->_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	_deviceContext->DrawIndexed(r.mesh->indexCount, 0, 0);
}



#pragma region yeOldeThings
/*
///PROJECT TEXTURE
SMatrix texView = DirectX::XMMatrixLookAtLH(SVec3(0.0f, 0.0f, -1.0f), SVec3(0.0f, 0.0f, 0.0f), SVec3::Up);
texProjector.SetShaderParameters(_deviceContext, modTerrain, cam.GetViewMatrix(), cam.GetViewMatrix(), cam.GetProjectionMatrix(),
							cam.GetProjectionMatrix(), _lights[0], cam.GetCameraMatrix().Translation(), dTime, offScreenTexture.baseSrv);
modTerrain.Draw(_deviceContext, texProjector);
texProjector.ReleaseShaderParameters(_deviceContext);

///RENDERING DEPTH TEXTURE
_deviceContext->RSSetViewports(1, &altViewport);	//to the shadow texture viewport
_deviceContext->OMSetRenderTargets(1, &(offScreenTexture.rtv), _D3D->GetDepthStencilView());	//switch to drawing on ost for the prepass	
_deviceContext->ClearRenderTargetView(offScreenTexture.rtv, ccb);	//then clear it, both the colours and the depth-stencil buffer
_deviceContext->ClearDepthStencilView(_D3D->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

for (auto tm : _terrainModels) {
	depth.SetShaderParameters(_deviceContext, *tm, offScreenTexture._view, offScreenTexture._lens);
	tm->Draw(_deviceContext, depth);
}
*/

#pragma endregion yeOldeThings