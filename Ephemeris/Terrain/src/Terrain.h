/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#pragma once

#include "Hemisphere.h"
#include "../../VolumetricClouds/src/VolumetricClouds.h"

#include "../../../The-Forge/Common_3/OS/Interfaces/ICameraController.h"
#include "../../../The-Forge/Common_3/OS/Interfaces/IMiddleware.h"
#include "../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/string.h"
#include "../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/unordered_map.h"
#include "../../../The-Forge/Middleware_3/UI/AppUI.h"
#include "../../../The-Forge/Common_3/Renderer/IRenderer.h"
#include "../../../The-Forge/Common_3/Renderer/GpuProfiler.h"

#include "../../../The-Forge/Common_3/Renderer/ResourceLoader.h"
#include "../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../The-Forge/Common_3/OS/Interfaces/ILogManager.h"
#include "../../../The-Forge/Common_3/OS/Interfaces/IMemoryManager.h"

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

enum PerlinInterpolate { LINEAR , COSINE , CUBIC };

#define INTERP_METHOD PerlinInterpolate::CUBIC
#define OCTAVES 16
#define PERSISTANCE 0.5
#define SCALE2D 15.0f
#define OVERALLSCALE 100.0f

struct VolumetricCloudsShadowCB
{
  vec4	SettingInfo00;			// x : EnableCastShadow, y : CloudCoverage, z : WeatherTextureTiling, w : Time
  vec4	StandardPosition;		// xyz : The current center location for applying wind, w : ShadowBrightness
  vec4  ShadowInfo; //vec4(gAppSettings.m_ShadowBrightness, gAppSettings.m_ShadowSpeed, gAppSettings.m_ShadowTiling, 0.0);
};

class Perlin
{
public:
	Perlin() {};
	~Perlin() {};
	
	static float noise2D(const int32_t x, const int32_t y);	

	static float interpolate(const float, const float, const float, const float, const float);
	static float linearInterpolate(const float, const float, const float);
	static float cosineInterpolate(const float, const float, const float);
	static float cubicInterpolate(const float, const float, const float, const float, const float);

	static float smoothedNoise2D(const int32_t, const int32_t);
	static float interpolatedNoise2D(const float, const float);
	static float perlinNoise2D(const float, const float);
	static float clamp(const float, const float, const float);
};

class Terrain : public IMiddleware
{
public:

	virtual bool Init(Renderer* const renderer) final;
	virtual void Exit() final;	
	virtual bool Load(RenderTarget** rts) final;
	virtual void Unload() final;
	virtual void Draw(Cmd* cmd) final;
	virtual void Update(float deltaTime) final;	


	void InitializeWithLoad(RenderTarget* InDepthRenderTarget);

	void Initialize(uint InImageCount,
		ICameraController* InCameraController, Queue*	InGraphicsQueue,
		Cmd** InTransCmds, Fence* InTransitionCompleteFences, GpuProfiler*	InGraphicsGpuProfiler, UIApp* InGAppUI);

	bool Load(int32_t width, int32_t height);
	void GenerateTerrainFromHeightmap(float height, float radius);
	bool GenerateNormalMap(Cmd* cmd);

	// Below are passed from Previous stage via Initialize()
	Renderer*                 pRenderer;

	uint                      gImageCount;
	uint                      mWidth;
	uint                      mHeight;	

  uint                      gFrameIndex;

	GuiComponent*             pGuiWindow = nullptr;
	UIApp*                    pGAppUI;

	Queue*                    pGraphicsQueue = NULL;
	Cmd**                     ppTransCmds = NULL;
	Fence*                    pTransitionCompleteFences = NULL;

	GpuProfiler*              pGraphicsGpuProfiler = NULL;

	RenderTarget*             pDepthBuffer = NULL;
	RenderTarget**            pFinalRT;

	ICameraController*        pCameraController = NULL;

	RasterizerState*          pRasterizerForTerrain = NULL;
	RasterizerState*          pRasterizerForTerrain_CULLFRONT = NULL;
	RasterizerState*          pRasterizerForTerrain_CULLBACK = NULL;

	RenderTarget*             pTerrainRT = NULL;

	DepthState*               pDepth = NULL;

	Perlin                    noiseGenerator;

  eastl::unordered_map < uint32_t, Zone* > gZoneMap;
  eastl::vector <uint32_t>  indexBuffer;

	Buffer*                   pGlobalZoneIndexBuffer;
	Buffer*                   pGlobalVertexBuffer;

  eastl::vector<MeshSegment> meshSegments;
	Buffer*                   pGlobalTriangularVertexBuffer;
  
  Buffer*                   pLightingTerrainUniformBuffer[3];
  Buffer*                   pRenderTerrainUniformBuffer[3];

	Texture*                  pTerrainHeightMap;
	RenderTarget*             pNormalMapFromHeightmapRT;

	Texture*                  pTerrainNormalTexture;
	Texture*                  pTerrainMaskTexture;

  eastl::vector<Texture*>   pTerrainTiledColorTextures;
  eastl::vector<Texture*>   pTerrainTiledNormalTextures;
	
	Texture*                  gTerrainTiledColorTexturesStorage = NULL;
	Texture*                  gTerrainTiledNormalTexturesStorage = NULL;

  eastl::vector<Texture*>   gTerrainTiledColorTexturesPacked;
  eastl::vector<Texture*>   gTerrainTiledNormalTexturesPacked;

	bool                      bFirstDraw = true;

	TerrainFrustum            terrainFrustum;
	mat4                      TerrainProjectionMatrix;

	Sampler*                  pLinearMirrorSampler;
	Sampler*                  pLinearWrapSampler;
	Sampler*                  pLinearBorderSampler;

	RenderTarget*             pGBuffer_BasicRT = NULL;
	RenderTarget*             pGBuffer_NormalRT = NULL;

	uint32_t                  TerrainPathVertexCount;

	float3                    LightDirection;
	float4                    LightColorAndIntensity;
	float3                    SunColor;

  bool                      IsEnabledShadow;
  VolumetricCloudsShadowCB  volumetricCloudsShadowCB; 
  Texture*                  pWeatherMap;

  Buffer*                   pVolumetricCloudsShadowBuffer;
};
