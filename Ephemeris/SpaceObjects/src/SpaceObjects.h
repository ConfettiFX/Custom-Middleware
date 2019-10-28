/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#pragma once

//#include "../../src/Perlin.h"
//#include "B_Spline.h"
//#include "Aurora.h"

#include "../../../../The-Forge/Common_3/OS/Interfaces/ICameraController.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMiddleware.h"
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/string.h"
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/unordered_map.h"
#include "../../../../The-Forge/Middleware_3/UI/AppUI.h"
#include "../../../../The-Forge/Common_3/Renderer/IRenderer.h"
#include "../../../../The-Forge/Common_3/Renderer/GpuProfiler.h"

class SpaceObjects : public IMiddleware
{
public:

	virtual bool Init(Renderer* renderer) final;
	virtual void Exit() final;
	virtual bool Load(RenderTarget** rts, uint32_t count = 1) final;
	virtual void Unload() final;
	virtual void Draw(Cmd* cmd) final;
	virtual void Update(float deltaTime) final;

	void Initialize(uint InImageCount,
		ICameraController* InCameraController, Queue*	InGraphicsQueue,
		Cmd** InTransCmds, Fence* InTransitionCompleteFences, GpuProfiler*	InGraphicsGpuProfiler, UIApp* InGAppUI, Buffer*	pTransmittanceBuffer);

	void InitializeWithLoad(RenderTarget* InDepthRenderTarget, RenderTarget* InLinearDepthRenderTarget, Texture* SavePrevTexture, Buffer* ParticleVertexBuffer, Buffer* ParticleInstanceBuffer, uint32_t ParticleCountParam);


	void GenerateRing(eastl::vector<float> &vertices, eastl::vector<uint32_t> &indices, uint32_t WidthDividor, uint32_t HeightDividor, float radius = 1.0f, float height = 1.0f);

	uint                  gFrameIndex;

	Renderer*             pRenderer;

	uint                  gImageCount;
	uint                  mWidth;
	uint                  mHeight;

	GuiComponent*         pGuiWindow = nullptr;
	UIApp*                pGAppUI;

	Queue*                pGraphicsQueue = NULL;
	Cmd**                 ppTransCmds = NULL;
	Fence*                pTransitionCompleteFences = NULL;

	GpuProfiler*          pGraphicsGpuProfiler = NULL;

	Texture*              pMoonTexture;

	Texture*							pSavePrevTexture;

	Buffer*               pGlobalTriangularVertexBuffer;

	Sampler*              pLinearClampSampler;
	Sampler*              pLinearBorderSampler;

	RasterizerState*      pRasterizerForSpaceObjects = NULL;
	RenderTarget*         pPreStageRenderTarget;
	//RenderTarget*         pSkyRenderTarget;
	RenderTarget*         pDepthBuffer = NULL;
	RenderTarget*         pLinearDepthBuffer = NULL;

	ICameraController*    pCameraController = NULL;

	Buffer*								pParticleVertexBuffer = NULL;
	Buffer*								pParticleInstanceBuffer = NULL;
	uint32_t							ParticleCount;

	mat4                  SpaceProjectionMatrix;

	float                 Azimuth;
	float                 Elevation;
	float3                LightDirection;
	float4                LightColorAndIntensity;
};
