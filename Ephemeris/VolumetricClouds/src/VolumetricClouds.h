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
#include "../../../The-Forge/Middleware_3/UI/AppUI.h"
#include "../../../The-Forge/Common_3/Renderer/IRenderer.h"
#include "../../../The-Forge/Common_3/Renderer/GpuProfiler.h"

#include "../../../The-Forge/Common_3/Renderer/ResourceLoader.h"
#include "../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"

struct VolumetricCloudsCB
{
  mat4	m_WorldToProjMat_1st;			// Matrix for converting World to Projected Space for the first eye
  mat4	m_ProjToWorldMat_1st;			// Matrix for converting Projected Space to World Matrix for the first eye
  mat4	m_ViewToWorldMat_1st;			// Matrix for converting View Space to World for the first eye
  mat4	m_PrevWorldToProjMat_1st;		// Matrix for converting Previous Projected Space to World for the first eye (it is used for reprojection)

  mat4	m_WorldToProjMat_2nd;			// Matrix for converting World to Projected Space for the second eye
  mat4	m_ProjToWorldMat_2nd;			// Matrix for converting Projected Space to World for the second eye
  mat4	m_ViewToWorldMat_2nd;			// Matrix for converting View Space to World for the the second eye
  mat4	m_PrevWorldToProjMat_2nd;		// Matrix for converting Previous Projected Space to World for the second eye (it is used for reprojection)

  mat4	m_LightToProjMat_1st;			// Matrix for converting Light to Projected Space Matrix for the first eye
  mat4	m_LightToProjMat_2nd;			// Matrix for converting Light to Projected Space Matrix for the second eye

  uint	m_JitterX;						// the X offset of Re-projection
  uint	m_JitterY;						// the Y offset of Re-projection
  uint	MIN_ITERATION_COUNT;			// Minimum iteration number of ray-marching
  uint	MAX_ITERATION_COUNT;			// Maximum iteration number of ray-marching

  vec4	m_StepSize;						// Cap of the step size X: min, Y: max
  //float	MAX_SAMPLE_STEP;				// Cap of the farthest step size

  vec4	TimeAndScreenSize;				// X: EplasedTime, Y: RealTime, Z: FullWidth, W: FullHeight
  vec4	lightDirection;
  vec4	lightColorAndIntensity;
  vec4	cameraPosition_1st;

  vec4	cameraPosition_2nd;

  //Wind
  vec4	WindDirection;
  vec4	StandardPosition;				// The current center location for applying wind

  float	m_CorrectU;						// m_JitterX / FullWidth
  float	m_CorrectV;						// m_JitterX / FullHeight

  float	LayerThickness;

  //Cloud
  float	CloudDensity;					// The overall density of clouds. Using bigger value makes more dense clouds, but it also makes ray-marching artifact worse.  
  float	CloudCoverage;					// The overall coverage of clouds. Using bigger value makes more parts of the sky be covered by clouds. (But, it does not make clouds more dense)
  float	CloudType;						// Add this value to control the overall clouds' type. 0.0 is for Stratus, 0.5 is for Stratocumulus, and 1.0 is for Cumulus.
  float	CloudTopOffset;					// Intensity of skewing clouds along the wind direction.

  //Modeling
  float	CloudSize;						// Overall size of the clouds. Using bigger value generates larger chunks of clouds.
  float	BaseShapeTiling;				// Control the base shape of the clouds. Using bigger value makes smaller chunks of base clouds.
  float	DetailShapeTiling;				// Control the detail shape of the clouds. Using bigger value makes smaller chunks of detail clouds.
  float	DetailStrenth;					// Intensify the detail of the clouds. It is possible to lose whole shape of the clouds if the user uses too high value of it.

  float	CurlTextureTiling;				// Control the curl size of the clouds. Using bigger value makes smaller curl shapes.
  float	CurlStrenth;					// Intensify the curl effect.
  float	AnvilBias;						// Using lower value makes anvil shape.
  float	WeatherTextureSize;				// Control the size of Weather map, bigger value makes the world to be covered by larger clouds pattern.
  float	WeatherTextureOffsetX;
  float	WeatherTextureOffsetZ;


  //Lighting
  float	BackgroundBlendFactor;			// Blend clouds with the background, more background will be shown if this value is close to 0.0
  float	Contrast;						// Contrast of the clouds' color 
  float	Eccentricity;					// The bright highlights around the sun that the user needs at sunset
  float	CloudBrightness;				// The brightness for clouds

  float	Precipitation;
  float	SilverliningIntensity;			// Intensity of silver-lining
  float	SilverliningSpread;				// Using bigger value spreads more silver-lining, but the intesity of it
  float	Random00;						// Random seed for the first ray-marching offset

  float	CameraFarClip;
  float	Padding01;
  float	Padding02;
  float	Padding03;

  uint  EnabledDepthCulling;
  uint	EnabledLodDepthCulling;
  uint  padding04;
  uint  padding05;

  // VolumetricClouds' Light shaft
  uint	GodNumSamples;					// Number of godray samples

  float	GodrayMaxBrightness;
  float	GodrayExposure;					// Intensity of godray

  float	GodrayDecay;					// Using smaller value, the godray brightness applied to each iteration is reduced. The level of reduction is also reduced per iteration.
  float	GodrayDensity;					// The distance between each interation.
  float	GodrayWeight;					// Using smaller value, the godray brightness applied to each iteration is reduced. The level of reduction is not changed.
  float	m_UseRandomSeed;

  float	Test00;
  float	Test01;
  float	Test02;
  float	Test03;

  VolumetricCloudsCB()
  {
    m_WorldToProjMat_1st = mat4::identity();
    m_ProjToWorldMat_1st = mat4::identity();
    m_ViewToWorldMat_1st = mat4::identity();
    m_PrevWorldToProjMat_1st = mat4::identity();

    m_WorldToProjMat_2nd = mat4::identity();
    m_ProjToWorldMat_2nd = mat4::identity();
    m_ViewToWorldMat_2nd = mat4::identity();
    m_PrevWorldToProjMat_2nd = mat4::identity();

    m_LightToProjMat_1st = mat4::identity();
    m_LightToProjMat_2nd = mat4::identity();

    m_JitterX = 0;
    m_JitterY = 0;
    MIN_ITERATION_COUNT = 0;
    MAX_ITERATION_COUNT = 0;

    m_StepSize = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    //float	MAX_SAMPLE_STEP;				// Cap of the farthest step size

    TimeAndScreenSize = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    lightDirection = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    lightColorAndIntensity = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    cameraPosition_1st = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    cameraPosition_2nd = vec4(0.0f, 0.0f, 0.0f, 0.0f);

    //Wind
    WindDirection = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    StandardPosition = vec4(0.0f, 0.0f, 0.0f, 0.0f);

    m_CorrectU = 0.0f;
    m_CorrectV = 0.0f;

    LayerThickness = 0.0f;

    //Cloud
    CloudDensity = 0.0f;
    CloudCoverage = 0.0f;
    CloudType = 0.0f;
    CloudTopOffset = 0.0f;

    //Modeling
    CloudSize = 0.0f;
    BaseShapeTiling = 0.0f;
    DetailShapeTiling = 0.0f;
    DetailStrenth = 0.0f;

    CurlTextureTiling = 0.0f;
    CurlStrenth = 0.0f;
    AnvilBias = 0.0f;
    WeatherTextureSize = 0.0f;
    WeatherTextureOffsetX = 0.0f;
    WeatherTextureOffsetZ = 0.0f;


    //Lighting
    BackgroundBlendFactor = 0.0f;
    Contrast = 0.0f;
    Eccentricity = 0.0f;
    CloudBrightness = 0.0f;

    Precipitation = 0.0f;
    SilverliningIntensity = 0.0f;
    SilverliningSpread = 0.0f;
    Random00 = 0.0f;

    CameraFarClip = 0.0f;
    Padding01 = 0.0f;
    Padding02 = 0.0f;
    Padding03 = 0.0f;

    EnabledDepthCulling = 0;
    EnabledLodDepthCulling = 0;
    padding04 = 0;
    padding05 = 0;

    // VolumetricClouds' Light shaft
    GodNumSamples = 0;

    GodrayMaxBrightness = 0.0f;
    GodrayExposure = 0.0f;

    GodrayDecay = 0.0f;
    GodrayDensity = 0.0f;
    GodrayWeight = 0.0f;
    m_UseRandomSeed = 0.0f;

    Test00 = 0.0f;
    Test01 = 0.0f;
    Test02 = 0.0f;
    Test03 = 0.0f;
  }
};

class VolumetricClouds : public IMiddleware
{
public:
	virtual bool Init(Renderer* const renderer) final;
	virtual void Exit() final;
	virtual bool Load(RenderTarget** rts) final;
	virtual void Unload() final;
	virtual void Draw(Cmd* cmd) final;
	virtual void Update(float deltaTime) final;	

	void InitializeWithLoad(Texture* InLinearDepthTexture, Texture* InSceneColorTexture, Texture* InDepthTexture);


	void Initialize(uint InImageCount,
		ICameraController* InCameraController, Queue*	InGraphicsQueue,
		Cmd** InTransCmds, Fence* InTransitionCompleteFences, Fence** InRenderCompleteFences, GpuProfiler* InGraphicsGpuProfiler, UIApp* InGAppUI, Buffer*	pTransmittanceBuffer);


	void Update(uint frameIndex);

	bool AddHiZDepthBuffer();
	bool AddVolumetricCloudsRenderTargets();

	void AddUniformBuffers();
	void RemoveUniformBuffers();

	bool AfterSubmit(uint currentFrameIndex);
	float4 GetProjectionExtents(float fov, float aspect, float width, float height, float texelOffsetX, float texelOffsetY);

  Texture* GetWeatherMap();

	// Below are passed from Previous stage via Initialize()
	Renderer*               pRenderer;
	uint                    gImageCount;
	uint                    mWidth;
	uint                    mHeight;	

  uint                    gFrameIndex;

	Texture*                pLinearDepthTexture = NULL;
	Texture*                pSceneColorTexture = NULL;	
	ICameraController*      pCameraController = NULL;
	
	
	UIApp*                  pGAppUI = NULL;
	Queue*                  pGraphicsQueue = NULL;

	Cmd**                   ppTransCmds = NULL;
	Fence*                  pTransitionCompleteFences = NULL;
	Fence**	                pRenderCompleteFences = NULL;	
	
	GpuProfiler*            pGraphicsGpuProfiler = NULL;

	GuiComponent*           pGuiWindow = NULL;
	RenderTarget*           pCastShadowRT = NULL;
	RenderTarget**          pFinalRT;	

	float3                  LightDirection;
	float4                  LightColorAndIntensity;

	Buffer*                 pTransmittanceBuffer;

  VolumetricCloudsCB      volumetricCloudsCB;
  vec4                    g_StandardPosition;
  vec4                    g_ShadowInfo;
  Texture*                pDepthTexture = NULL;
};
