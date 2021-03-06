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
#include "../../../../The-Forge/Common_3/OS/Interfaces/IProfiler.h"
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/string.h"
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/unordered_map.h"
#include "../../../../The-Forge/Middleware_3/UI/AppUI.h"
#include "../../../../The-Forge/Common_3/Renderer/IRenderer.h"

class SpaceObjects : public IMiddleware
{
public:

	virtual bool Init(Renderer* renderer, PipelineCache* pCache = NULL) final;
	virtual void Exit() final;
	virtual bool Load(RenderTarget** rts, uint32_t count = 1) final;
	virtual void Unload() final;
	virtual void Draw(Cmd* cmd) final;
	virtual void Update(float deltaTime) final;

	void Initialize(uint InImageCount,
		ICameraController* InCameraController, Queue*	InGraphicsQueue, CmdPool* InTransCmdPool,
		Cmd** InTransCmds, Fence* InTransitionCompleteFences, ProfileToken InGraphicsGpuProfiler, UIApp* InGAppUI, Buffer*	pTransmittanceBuffer);

	void InitializeWithLoad(RenderTarget* InDepthRenderTarget, RenderTarget* InLinearDepthRenderTarget, Texture* SavePrevTexture, Buffer* ParticleVertexBuffer, Buffer* ParticleInstanceBuffer, uint32_t ParticleCountParam, uint32_t ParticleVertexStride, uint32_t ParticleInstanceStride);


	void GenerateRing(eastl::vector<float> &vertices, eastl::vector<uint32_t> &indices, uint32_t WidthDividor, uint32_t HeightDividor, float radius = 1.0f, float height = 1.0f);

	uint                  gFrameIndex = 0;

	Renderer*             pRenderer = NULL;
	PipelineCache*        pPipelineCache = NULL;

	uint                  gImageCount = 0;
	uint                  mWidth = 0;
	uint                  mHeight = 0;

	GuiComponent*         pGuiWindow = nullptr;
	UIApp*                pGAppUI = NULL;

	Queue*                pGraphicsQueue = NULL;
	CmdPool*              pTransCmdPool = NULL;
	Cmd**                 ppTransCmds = NULL;
	Fence*                pTransitionCompleteFences = NULL;

	ProfileToken          gGpuProfileToken = {};

	Texture*              pMoonTexture = NULL;

	Texture*              pSavePrevTexture = NULL;

	Buffer*               pGlobalTriangularVertexBuffer = NULL;

	Sampler*              pLinearClampSampler = NULL;
	Sampler*              pLinearBorderSampler = NULL;

	RenderTarget*         pPreStageRenderTarget = NULL;
	//RenderTarget*         pSkyRenderTarget;
	RenderTarget*         pDepthBuffer = NULL;
	RenderTarget*         pLinearDepthBuffer = NULL;

	ICameraController*    pCameraController = NULL;

	Buffer*								pParticleVertexBuffer = NULL;
	Buffer*								pParticleInstanceBuffer = NULL;
	uint32_t							ParticleCount = 0;
	uint32_t							ParticleVertexStride = 0;
	uint32_t							ParticleInstanceStride = 0;

	mat4                  SpaceProjectionMatrix;

	float                 Azimuth = 0.0f;
	float                 Elevation = 0.0f;
	float3                LightDirection;
	float4                LightColorAndIntensity;
};
