/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#pragma once

#include "Hemisphere.h"
#include "../../src/Perlin.h"
#include "../../VolumetricClouds/src/VolumetricClouds.h"

#include "../../../../The-Forge/Common_3/OS/Interfaces/ICameraController.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMiddleware.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IProfiler.h"
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/string.h"
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/unordered_map.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IUI.h"
#include "../../../../The-Forge/Common_3/Renderer/IRenderer.h"

#define GRID_SIZE 256
#define TILE_CENTER 50

#define USE_PROCEDUAL_TERRAIN 0

struct Zone
{
	bool visible;
	uint32_t X;
	uint32_t Z;

	//vec3 positions[GRID_SIZE][GRID_SIZE];
	eastl::vector<float> PosAndUVs;

	Buffer*	pZoneVertexBuffer = NULL;
	Buffer*	pZoneIndexBuffer = NULL;

	uint32_t GetKey()
	{
		uint32_t z = Z;
		uint32_t x = X;
		while (z > 0)
		{
			z /= 10;
			x *= 10;
		}

		return (x + Z);
	}

	Zone()
	{
		visible = false;
		X = TILE_CENTER;
		Z = TILE_CENTER;
	}
};

typedef eastl::unordered_map<uint32_t, Zone*> ZoneMap;

struct VolumetricCloudsShadowCB
{
	vec4	SettingInfo00;			// x : EnableCastShadow, y : CloudCoverage, z : WeatherTextureTiling, w : Time
	vec4	StandardPosition;		// xyz : The current center location for applying wind, w : ShadowBrightness
	vec4  ShadowInfo; //vec4(gAppSettings.m_ShadowBrightness, gAppSettings.m_ShadowSpeed, gAppSettings.m_ShadowTiling, 0.0);
};

class Terrain : public IMiddleware
{
public:

	virtual bool Init(Renderer* renderer, PipelineCache* pCache = NULL) final;
	virtual void Exit() final;
	virtual bool Load(RenderTarget** rts, uint32_t count = 1) final;
	virtual void Unload() final;
	virtual void Draw(Cmd* cmd) final;
	virtual void Update(float deltaTime) final;


	void InitializeWithLoad(RenderTarget* InDepthRenderTarget);

	void Initialize(uint InImageCount,
		ICameraController* InCameraController, Queue*	InGraphicsQueue, CmdPool* InTransCmdPool,
		Cmd** InTransCmds, Fence* InTransitionCompleteFences, ProfileToken InGraphicsGpuProfiler);

	bool Load(int32_t width, int32_t height);
	void GenerateTerrainFromHeightmap(float height, float radius);
	bool GenerateNormalMap(Cmd* cmd);

	// Below are passed from Previous stage via Initialize()
	Renderer*                 pRenderer = NULL;
	PipelineCache*            pPipelineCache = NULL;

	uint                      gImageCount = 0;
	uint                      mWidth = 0;
	uint                      mHeight = 0;

	uint                      gFrameIndex = 0;

	UIComponent*             pGuiWindow = NULL;

	Queue*                    pGraphicsQueue = NULL;
	CmdPool*                  pTransCmdPool = NULL;
	Cmd**                     ppTransCmds = NULL;
	Fence*                    pTransitionCompleteFences = NULL;

	RenderTarget*             pDepthBuffer = NULL;
	RenderTarget**            pFinalRT = NULL;

	ICameraController*        pCameraController = NULL;

	RenderTarget*             pTerrainRT = NULL;

	Perlin                    noiseGenerator;

	ProfileToken               gGpuProfileToken = {};

	eastl::unordered_map < uint32_t, Zone* > gZoneMap;
	eastl::vector <uint32_t>  indexBuffer;

	Buffer*                   pGlobalZoneIndexBuffer = NULL;
	Buffer*                   pGlobalVertexBuffer = NULL;

	eastl::vector<MeshSegment> meshSegments;
	Buffer*                   pGlobalTriangularVertexBuffer = NULL;

	Buffer*                   pLightingTerrainUniformBuffer[3] = {};
	Buffer*                   pRenderTerrainUniformBuffer[3] = {};

	Texture*                  pTerrainHeightMap = NULL;
	RenderTarget*             pNormalMapFromHeightmapRT = NULL;

	Texture*                  pTerrainNormalTexture = NULL;
	Texture*                  pTerrainMaskTexture = NULL;

	Texture**                  gTerrainTiledColorTexturesStorage = NULL;
	Texture**                  gTerrainTiledNormalTexturesStorage = NULL;

	const uint32_t             gTerrainTextureCount = 5;

	bool                      bFirstDraw = true;

	TerrainFrustum            terrainFrustum;
	mat4                      TerrainProjectionMatrix;

	Sampler*                  pLinearMirrorSampler = NULL;
	Sampler*                  pLinearWrapSampler = NULL;
	Sampler*                  pLinearBorderSampler = NULL;

	RenderTarget*             pGBuffer_BasicRT = NULL;
	RenderTarget*             pGBuffer_NormalRT = NULL;

	uint32_t                  TerrainPathVertexCount = 0;

	float3                    LightDirection;
	float4                    LightColorAndIntensity;
	float3                    SunColor;

	bool                      IsEnabledShadow = false;
	VolumetricCloudsShadowCB  volumetricCloudsShadowCB;
	Texture*                  pWeatherMap = NULL;

	Buffer*                   pVolumetricCloudsShadowBuffer = NULL;
};
