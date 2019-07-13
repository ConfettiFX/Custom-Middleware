/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#pragma once

#include "../../../The-Forge/Common_3/OS/Interfaces/ICameraController.h"
#include "../../../The-Forge/Common_3/OS/Interfaces/IMiddleware.h"
#include "../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/string.h"
#include "../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/unordered_map.h"
#include "../../../The-Forge/Middleware_3/UI/AppUI.h"
#include "../../../The-Forge/Common_3/Renderer/IRenderer.h"
#include "../../../The-Forge/Common_3/Renderer/GpuProfiler.h"

#include "../../../The-Forge/Common_3/Renderer/ResourceLoader.h"
#include "../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"

#include "SkyCommon.h"

class Sky : public IMiddleware
{
public:

	virtual bool Init(Renderer* const renderer) final;
	virtual void Exit() final;
	virtual bool Load(RenderTarget** rts) final;
	virtual void Unload() final;
	virtual void Draw(Cmd* cmd) final;
	virtual void Update(float deltaTime) final;

	void Initialize(uint InImageCount,
		ICameraController* InCameraController, Queue*	InGraphicsQueue,
		Cmd** InTransCmds, Fence* InTransitionCompleteFences, GpuProfiler*	InGraphicsGpuProfiler, UIApp* InGAppUI, Buffer*	pTransmittanceBuffer);	
	
	void InitializeWithLoad(RenderTarget* InDepthRenderTarget, RenderTarget* InLinearDepthRenderTarget);

	void CalculateLookupData();
	float3 GetSunColor();

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

	Texture*              pTransmittanceTexture;
	Texture*              pIrradianceTexture;		//unsigned int irradianceTexture;//unit 2, E table
	Texture*              pInscatterTexture;		//unsigned int inscatterTexture;//unit 3, S table
	Texture*              pMoonTexture;
	
	Buffer*               pGlobalTriangularVertexBuffer;

	Sampler*              pLinearClampSampler;

	RasterizerState*      pRasterizerForSky = NULL;
	RenderTarget*         pPreStageRenderTarget;
	RenderTarget*         pSkyRenderTarget;
	RenderTarget*         pDepthBuffer = NULL;
	RenderTarget*         pLinearDepthBuffer = NULL;

	ICameraController*    pCameraController = NULL;

	float3                LightDirection;
	float4                LightColorAndIntensity;

	mat4                  SkyProjectionMatrix;

	Buffer*               pTransmittanceBuffer;
};
