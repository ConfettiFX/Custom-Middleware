/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#pragma once

#include "SkyCommon.h"
#include "Icosahedron.h"
#include "../../src/Perlin.h"

#include "../../../../The-Forge/Common_3/OS/Interfaces/ICameraController.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMiddleware.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IProfiler.h"
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/string.h"
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/unordered_map.h"
#include "../../../../The-Forge/Middleware_3/UI/AppUI.h"
#include "../../../../The-Forge/Common_3/Renderer/IRenderer.h"

typedef struct ParticleData
{
	vec4 ParticlePositions;
	vec4 ParticleColors;
	vec4 ParticleInfo;        // x: temperature, y: particle size, z: blink time seed,
}ParticleData;

typedef struct ParticleDataSet
{
	eastl::vector<ParticleData> ParticleDataArray;
}ParticleDataSet;

typedef struct ParticleSystem
{
	Buffer* pParticleVertexBuffer;
	Buffer* pParticleInstanceBuffer;
	ParticleDataSet particleDataSet;
} ParticleSystem;



class Sky : public IMiddleware
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

	void InitializeWithLoad(RenderTarget* InDepthRenderTarget, RenderTarget* InLinearDepthRenderTarget);

	void CalculateLookupData();
	float3 GetSunColor();
	void GenerateIcosahedron(float **ppPoints, eastl::vector<float> &vertices, eastl::vector<uint32_t> &indices, int numberOfDivisions, float radius = 1.0f);

	Buffer*								GetParticleVertexBuffer();
	Buffer*								GetParticleInstanceBuffer();
	uint32_t							GetParticleCount();

	uint                  gFrameIndex;

	Renderer*             pRenderer;
	PipelineCache*        pPipelineCache;

	uint                  gImageCount;
	uint                  mWidth;
	uint                  mHeight;

	GuiComponent*         pGuiWindow = nullptr;
	UIApp*                pGAppUI;

	Queue*                pGraphicsQueue = NULL;
	CmdPool*              pTransCmdPool = NULL;
	Cmd**                 ppTransCmds = NULL;
	Fence*                pTransitionCompleteFences = NULL;

	Texture*              pTransmittanceTexture;
	Texture*              pIrradianceTexture;		//unsigned int irradianceTexture;//unit 2, E table
	Texture*              pInscatterTexture;		//unsigned int inscatterTexture;//unit 3, S table

	Sampler*              pLinearClampSampler;
	Sampler*              pLinearBorderSampler;

	RenderTarget*         pPreStageRenderTarget;
	RenderTarget*         pSkyRenderTarget;
	RenderTarget*         pDepthBuffer = NULL;
	RenderTarget*         pLinearDepthBuffer = NULL;

	ICameraController*    pCameraController = NULL;

	float                 Azimuth;
	float                 Elevation;
	float3                LightDirection;
	float4                LightColorAndIntensity;

	mat4                  SkyProjectionMatrix;
	mat4                  SpaceProjectionMatrix;

	Buffer*               pTransmittanceBuffer;

	Perlin                noiseGenerator;

    ProfileToken          gGpuProfileToken;

	ParticleSystem				gParticleSystem;
};
