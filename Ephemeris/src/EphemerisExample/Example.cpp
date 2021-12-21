/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

//asimp importer
//stl
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/vector.h"
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/string.h"

// Ephemeris BEGIN
#include "../../Terrain/src/Terrain.h"
#include "../../Sky/src/Sky.h"
#include "../../VolumetricClouds/src/VolumetricClouds.h"
#include "../../SpaceObjects/src/SpaceObjects.h"
// Ephemeris END

//Interfaces
#include "../../../../The-Forge/Common_3/OS/Interfaces/ICameraController.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IApp.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IInput.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/ITime.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IProfiler.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IScripting.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IUI.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IFont.h"

//Renderer
#include "../../../../The-Forge/Common_3/Renderer/IRenderer.h"
#include "../../../../The-Forge/Common_3/Renderer/IResourceLoader.h"

//Math
#include "../../../../The-Forge/Common_3/OS/Math/MathTypes.h"

//Memory
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"

const uint32_t      gImageCount = 3;

static bool         gToggleVSync = false;
static bool         gTogglePerformance = true;
static bool         gToggleFXAA = true;
static bool         gShowUI = true;

Renderer*           pRenderer = NULL;

Queue*              pGraphicsQueue = NULL;
CmdPool*            pGraphicsCmdPools[gImageCount];
Cmd*                pGraphicsCmds[gImageCount];

CmdPool*            pTransCmdPool;
Cmd**               ppTransCmds;

SwapChain*          pSwapChain = NULL;
RenderTarget*       pDepthBuffer = NULL;

RenderTarget*       pLinearDepthBuffer = NULL;

Shader*             pLinearDepthResolveShader = NULL;
Pipeline*           pLinearDepthResolvePipeline = NULL;

//Shader*             pLinearDepthCompShader = NULL;
//Pipeline*           pLinearDepthCompPipeline = NULL;
//RootSignature*      pLinearDepthCompRootSignature = NULL;

RenderTarget*       pTerrainResultRT = NULL;
RenderTarget*       pSkydomeResultRT = NULL;

Shader*             pFXAAShader = NULL;
Pipeline*           pFXAAPipeline = NULL;

DescriptorSet*      pExampleDescriptorSet = NULL;
RootSignature*      pExampleRootSignature = NULL;
uint32_t            gExampleRootConstantIndex = 0;

Sampler*            pBilinearClampSampler = NULL;

Fence*              pRenderCompleteFences[gImageCount] = { NULL };
Fence*              pTransitionCompleteFences = NULL;

Semaphore*          pImageAcquiredSemaphore = NULL;
Semaphore*          pRenderCompleteSemaphores[gImageCount] = { NULL };

ProfileToken        gGpuProfileToken;
UIComponent*       pMainGuiWindow = NULL;

Buffer*             pTransmittanceBuffer = NULL;

static float2       LightDirection = float2(0.0f, 270.0f);
static float4       LightColorAndIntensity = float4(1.0f, 1.0f, 1.0f, 1.0f);


#define NEAR_CAMERA 50.0f
#define FAR_CAMERA 100000000.0f

static uint         gPrevFrameIndex = 0;
static bool         bSunMove = true;
static float        SunMovingSpeed = 5.0f;

float               mChildIndent = 25.0f;
float               mHeightOffset = 20.0f;

//--------------------------------------------------------------------------------------------
// MESHES
//--------------------------------------------------------------------------------------------
typedef enum MeshResource
{
	MESH_MAT_BALL,
	MESH_CUBE,
	MESH_COUNT,
} MeshResource;

struct Vertex
{
	float3 mPos;
	float3 mNormal;
	float2 mUv;
};

struct FXAAINFO
{
	vec2 ScreenSize;
	float Use;
	float Time;
};

FXAAINFO gFXAAinfo;

#if defined(TARGET_IOS) || defined(__ANDROID__)
VirtualJoystickUI*   pVirtualJoystick = NULL;
#endif

uint32_t			gFrameIndex = 0;

ICameraController*  pCameraController = NULL;

VolumetricClouds	gVolumetricClouds;
Terrain						gTerrain;
Sky								gSky;
SpaceObjects			gSpaceObjects;

FontDrawDesc gFrameTimeDraw;
FontDrawDesc gDefaultTextDrawDesc;
uint32_t     gFontID = 0;

const char* pPipelineCacheName = "PipelineCache.cache";
PipelineCache* pPipelineCache = NULL;

static HiresTimer gTimer;

class RenderEphemeris : public IApp
{
public:

	bool Init()
	{
		initHiresTimer(&gTimer);

		// FILE PATHS
		fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "Shaders");
		fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG,   RD_SHADER_BINARIES, "CompiledShaders");
		fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG,   RD_PIPELINE_CACHE, "PipelineCaches");
		fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_GPU_CONFIG, "GPUCfg");
		fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_TEXTURES, "Textures");
		fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_MESHES, "Meshes");
		fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_FONTS, "Fonts");
		fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SCRIPTS, "Scripts");

		CameraMotionParameters cmp{ 32000.0f, 120000.0f, 40000.0f };

		float h = 6000.0f;

		vec3 camPos{ 0.0f, h, -10.0f };
		vec3 lookAt{ 0.0f, h, 1.0f };

		pCameraController = initFpsCameraController(camPos, lookAt);
		pCameraController->setMotionParameters(cmp);

		RendererDesc settings;
		memset(&settings, 0, sizeof(settings));
		settings.mD3D11Unsupported = true; 
		settings.mGLESUnsupported = true; 
		initRenderer(GetName(), &settings, &pRenderer);
		//check for init success
		if (!pRenderer)
			return false;

		QueueDesc queueDesc = {};
		queueDesc.mType = QUEUE_TYPE_GRAPHICS;
		queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
		addQueue(pRenderer, &queueDesc, &pGraphicsQueue);
		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			CmdPoolDesc cmdPoolDesc = {};
			cmdPoolDesc.pQueue = pGraphicsQueue;
			addCmdPool(pRenderer, &cmdPoolDesc, &pGraphicsCmdPools[i]);
			CmdDesc cmdDesc = {};
			cmdDesc.pPool = pGraphicsCmdPools[i];
			addCmd(pRenderer, &cmdDesc, &pGraphicsCmds[i]);
		}

		CmdPoolDesc cmdPoolDesc = {};
		cmdPoolDesc.pQueue = pGraphicsQueue;
		addCmdPool(pRenderer, &cmdPoolDesc, &pTransCmdPool);
		CmdDesc cmdDesc = {};
		cmdDesc.pPool = pTransCmdPool;
		addCmd_n(pRenderer, &cmdDesc, 1, &ppTransCmds);

		addFence(pRenderer, &pTransitionCompleteFences);

		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			addFence(pRenderer, &pRenderCompleteFences[i]);
			addSemaphore(pRenderer, &pRenderCompleteSemaphores[i]);
		}

		addSemaphore(pRenderer, &pImageAcquiredSemaphore);

		initResourceLoaderInterface(pRenderer);

		// Load fonts
		FontDesc font = {};
		font.pFontPath = "TitilliumText/TitilliumText-Bold.otf";
		fntDefineFonts(&font, 1, &gFontID);

		FontSystemDesc fontRenderDesc = {};
		fontRenderDesc.pRenderer = pRenderer;
		if (!initFontSystem(&fontRenderDesc))
			return false; // report?

		// Initialize Forge User Interface Rendering
		UserInterfaceDesc uiRenderDesc = {};
		uiRenderDesc.pRenderer = pRenderer;
		initUserInterface(&uiRenderDesc);

		PipelineCacheLoadDesc cacheDesc = {};
		cacheDesc.pFileName = pPipelineCacheName;
		loadPipelineCache(pRenderer, &cacheDesc, &pPipelineCache);

		// Initialize micro profiler and its UI.
		ProfilerDesc profiler = {};
		profiler.pRenderer = pRenderer;
		profiler.mWidthUI = mSettings.mWidth;
		profiler.mHeightUI = mSettings.mHeight;
		initProfiler(&profiler);

		gGpuProfileToken = addGpuProfiler(pRenderer, pGraphicsQueue, "GpuProfiler");

#if defined(__ANDROID__) || defined(TARGET_IOS)
		pVirtualJoystick = addVirtualJoystickUI(pRenderer, "circlepad.png");
		if (!pVirtualJoystick)
		{
			LOGERRORF("Could not initialize Virtual Joystick.");
			return false;
		}
#endif

#if defined(METAL)
        {
            float dpiScale[2];
            getDpiScale(dpiScale);
            gFrameTimeDraw.mFontSize /= dpiScale[1];
            gFrameTimeDraw.mFontID = gFontID;
            gDefaultTextDrawDesc.mFontSize /= dpiScale[1];
        }
#endif


		SamplerDesc samplerClampDesc = {
		FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
		ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE
		};
		addSampler(pRenderer, &samplerClampDesc, &pBilinearClampSampler);

		eastl::string shaderPath("");
		//eastl::string shaderFullPath;

		RootSignatureDesc rootDesc;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////

		ShaderLoadDesc FXAAShader = {};
		FXAAShader.mStages[0] = { "Triangular.vert", NULL, 0 };
		FXAAShader.mStages[1] = { "FXAA.frag", NULL, 0 };
		addShader(pRenderer, &FXAAShader, &pFXAAShader);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////

		ShaderLoadDesc depthLinearizationResolveShader = {};
		depthLinearizationResolveShader.mStages[0] = { "Triangular.vert", NULL, 0 };
		depthLinearizationResolveShader.mStages[1] = { "DepthLinearization.frag", NULL, 0 };
		addShader(pRenderer, &depthLinearizationResolveShader, &pLinearDepthResolveShader);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/*ShaderLoadDesc depthLinearizationShader = {};
		depthLinearizationShader.mStages[0] = { "depthLinearization.comp", NULL, 0 };
		addShader(pRenderer, &depthLinearizationShader, &pLinearDepthCompShader);

		rootDesc = {};
		rootDesc = { 0 };
		rootDesc.mShaderCount = 1;
		rootDesc.ppShaders = &pLinearDepthCompShader;

		addRootSignature(pRenderer, &rootDesc, &pLinearDepthCompRootSignature);*/
		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		const char* pStaticSamplerNames[] = { "g_LinearClamp" };
		Sampler* pStaticSamplers[] = { pBilinearClampSampler };
		Shader*           shaders[] = { pFXAAShader, pLinearDepthResolveShader };
		rootDesc = {};
		rootDesc.mShaderCount = 2;
		rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
		rootDesc.ppStaticSamplers = pStaticSamplers;
		rootDesc.ppShaders = shaders;
		rootDesc.mStaticSamplerCount = 1;
		addRootSignature(pRenderer, &rootDesc, &pExampleRootSignature);
		gExampleRootConstantIndex = getDescriptorIndexFromName(pExampleRootSignature, "ExampleRootConstant");

		DescriptorSetDesc setDesc = { pExampleRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 2 };
		addDescriptorSet(pRenderer, &setDesc, &pExampleDescriptorSet);

		// Add TransmittanceBuffer buffer
		BufferLoadDesc TransBufferDesc = {};
		TransBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_RW_BUFFER | DESCRIPTOR_TYPE_BUFFER;
		TransBufferDesc.mDesc.mElementCount = 3;
		TransBufferDesc.mDesc.mStructStride = sizeof(float4);
		TransBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
		TransBufferDesc.mDesc.mSize = TransBufferDesc.mDesc.mStructStride * TransBufferDesc.mDesc.mElementCount;
		TransBufferDesc.mDesc.mStartState = RESOURCE_STATE_SHADER_RESOURCE;

		//TransBufferDesc.pData = gInitializeVal.data();
		TransBufferDesc.ppBuffer = &pTransmittanceBuffer;
		addResource(&TransBufferDesc, NULL);

		gTerrain.Initialize(gImageCount, pCameraController, pGraphicsQueue, pTransCmdPool, ppTransCmds, pTransitionCompleteFences, gGpuProfileToken);
		gTerrain.Init(pRenderer, pPipelineCache);

		gSky.Initialize(gImageCount, pCameraController, pGraphicsQueue, pTransCmdPool, ppTransCmds, pTransitionCompleteFences, gGpuProfileToken, pTransmittanceBuffer);
		gSky.Init(pRenderer, pPipelineCache);

		gVolumetricClouds.Initialize(gImageCount, pCameraController, pGraphicsQueue, pTransCmdPool, ppTransCmds, pTransitionCompleteFences, pRenderCompleteFences, gGpuProfileToken, pTransmittanceBuffer);
		gVolumetricClouds.Init(pRenderer, pPipelineCache);

		gSpaceObjects.Initialize(gImageCount, pCameraController, pGraphicsQueue, pTransCmdPool, ppTransCmds, pTransitionCompleteFences, gGpuProfileToken, pTransmittanceBuffer);
		gSpaceObjects.Init(pRenderer, pPipelineCache);

		UIComponentDesc UIComponentDesc = {};
		float dpiScale[2];
		getDpiScale(dpiScale);

		UIComponentDesc.mStartPosition = vec2(960.0f / dpiScale[0], 700.0f / dpiScale[1]);
		UIComponentDesc.mStartSize = vec2(300.0f / dpiScale[0], 250.0f / dpiScale[1]);
		UIComponentDesc.mFontID = 0; 
		UIComponentDesc.mFontSize = 16.0f; 
		uiCreateComponent("Global Settings", &UIComponentDesc, &pMainGuiWindow);

		CheckboxWidget checkbox;
		checkbox.pData = &gToggleFXAA;
		luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Enable FXAA", &checkbox, WIDGET_TYPE_CHECKBOX));

		SeparatorWidget separator;
		luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

		SliderFloatWidget sliderFloat;
		sliderFloat.pData = &LightDirection.x;
		sliderFloat.mMin = -180.0f;
		sliderFloat.mMax = 180.0f;
		sliderFloat.mStep = 0.001f;
		luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Light Azimuth", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT));

		sliderFloat.pData = &LightDirection.y;
		sliderFloat.mMin = 0.0f;
		sliderFloat.mMax = 360.0f;
		sliderFloat.mStep = 0.001f;
		luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Light Elevation", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT));

		luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

		SliderFloat4Widget sliderFloat4;
		sliderFloat4.pData = &LightColorAndIntensity;
		sliderFloat4.mMin = float4(0.0f);
		sliderFloat4.mMax = float4(10.0f);
		sliderFloat4.mStep = float4(0.01f);
		luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Light Color & Intensity", &sliderFloat4, WIDGET_TYPE_SLIDER_FLOAT4));

		luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

		checkbox.pData = &bSunMove;
		luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Automatic Sun Moving", &checkbox, WIDGET_TYPE_CHECKBOX));

		sliderFloat.pData = &SunMovingSpeed;
		sliderFloat.mMin = -100.0f;
		sliderFloat.mMax = 100.0f;
		sliderFloat.mStep = 0.01f;
		luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Sun Moving Speed", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT));

		InputSystemDesc inputDesc = {};
		inputDesc.pRenderer = pRenderer;
		inputDesc.pWindow = pWindow;
		if (!initInputSystem(&inputDesc))
			return false;
		
		// Microprofiler Actions
		// #TODO: Remove this once the profiler UI is ported to use our UI system
		InputActionDesc actionDesc;
		
		// App Actions
		actionDesc = { InputBindings::BUTTON_FULLSCREEN, [](InputActionContext* ctx) { toggleFullscreen(((IApp*)ctx->pUserData)->pWindow); return true; }, this };
		addInputAction(&actionDesc);
		actionDesc = { InputBindings::BUTTON_EXIT, [](InputActionContext* ctx) { requestShutdown(); return true; } };
		addInputAction(&actionDesc);
		actionDesc =
		{
		  InputBindings::BUTTON_ANY, [](InputActionContext* ctx)
		  {
		    bool capture = uiOnButton(ctx->mBinding, ctx->mBool, ctx->pPosition);
		    setEnableCaptureInput(capture && INPUT_ACTION_PHASE_CANCELED != ctx->mPhase);
		    return true;
		  }, this
		};
		addInputAction(&actionDesc);
		typedef bool(*CameraInputHandler)(InputActionContext* ctx, uint32_t index);
		static CameraInputHandler onCameraInput = [](InputActionContext* ctx, uint32_t index)
		{
		  if (!uiIsFocused() && *ctx->pCaptured)
		    index ? pCameraController->onRotate(ctx->mFloat2) : pCameraController->onMove(ctx->mFloat2);
		  return true;
		};
		actionDesc = { InputBindings::FLOAT_RIGHTSTICK, [](InputActionContext* ctx) { return onCameraInput(ctx, 1); }, NULL, 20.0f, 200.0f, 1.0f };
		addInputAction(&actionDesc);
		actionDesc = { InputBindings::FLOAT_LEFTSTICK, [](InputActionContext* ctx) { return onCameraInput(ctx, 0); }, NULL, 20.0f, 200.0f, 1.0f };
		addInputAction(&actionDesc);
		actionDesc = { InputBindings::BUTTON_NORTH, [](InputActionContext* ctx) { pCameraController->resetView(); return true; } };
		addInputAction(&actionDesc);
		
		actionDesc = { InputBindings::BUTTON_L3, [](InputActionContext* ctx) { gShowUI = !gShowUI; return true; } };
		addInputAction(&actionDesc);
		actionDesc = { InputBindings::BUTTON_R3, [](InputActionContext* ctx) { gTogglePerformance = !gTogglePerformance; return true; } };
		addInputAction(&actionDesc);

		gFrameIndex = 0;

		return true;
	}

	void Exit()
	{
		exitCameraController(pCameraController);
		exitInputSystem();

		gFrameIndex = 0;
		gPrevFrameIndex = 0;

		removeDescriptorSet(pRenderer, pExampleDescriptorSet);

#if defined(TARGET_IOS) || defined(__ANDROID__)
		removeVirtualJoystickUI(pVirtualJoystick);
#endif

		gSpaceObjects.Exit();
		gVolumetricClouds.Exit();
		gSky.Exit();
		gTerrain.Exit();

		exitProfiler();

		exitUserInterface();

		exitFontSystem(); 

		removeResource(pTransmittanceBuffer);

		removeSampler(pRenderer, pBilinearClampSampler);

		removeShader(pRenderer, pFXAAShader);
		//removeShader(pRenderer, pLinearDepthCompShader);
		removeShader(pRenderer, pLinearDepthResolveShader);

		//removeRootSignature(pRenderer, pLinearDepthCompRootSignature);
		removeRootSignature(pRenderer, pExampleRootSignature);

		removeFence(pRenderer, pTransitionCompleteFences);

		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			removeFence(pRenderer, pRenderCompleteFences[i]);
			removeSemaphore(pRenderer, pRenderCompleteSemaphores[i]);
		}
		removeSemaphore(pRenderer, pImageAcquiredSemaphore);

		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			removeCmd(pRenderer, pGraphicsCmds[i]);
			removeCmdPool(pRenderer, pGraphicsCmdPools[i]);
		}

		removeCmd_n(pRenderer, 1, ppTransCmds);
		removeCmdPool(pRenderer, pTransCmdPool);

		removeQueue(pRenderer, pGraphicsQueue);

		PipelineCacheSaveDesc saveDesc;
		saveDesc.pFileName = pPipelineCacheName;
		savePipelineCache(pRenderer, pPipelineCache, &saveDesc);
		removePipelineCache(pRenderer, pPipelineCache);

		exitResourceLoaderInterface(pRenderer);

		exitRenderer(pRenderer);
		pRenderer = NULL; 
	}

	bool Load()
	{
		if (!addSwapChain())
			return false;

		if (!addSceneRenderTarget())
			return false;

		if (!addDepthBuffer())
			return false;

		RenderTarget* ppPipelineRenderTargets[] = {
			pSwapChain->ppRenderTargets[0],
			pDepthBuffer
		};

		if (!addFontSystemPipelines(ppPipelineRenderTargets, 2, NULL))
			return false;

		if (!addUserInterfacePipelines(ppPipelineRenderTargets[0]))
			return false;

#if defined(TARGET_IOS) || defined(__ANDROID__)
		if (!loadVirtualJoystickUI(pVirtualJoystick, pSwapChain->ppRenderTargets[0], pDepthBuffer->mFormat))
			return false;
#endif

		gTerrain.pWeatherMap = gVolumetricClouds.GetWeatherMap();
		gTerrain.InitializeWithLoad(pDepthBuffer);
		gTerrain.Load(mSettings.mWidth, mSettings.mHeight);

		gSky.InitializeWithLoad(pDepthBuffer, pLinearDepthBuffer);
		gSky.Load(&gTerrain.pTerrainRT);

		gVolumetricClouds.InitializeWithLoad(pLinearDepthBuffer, gSky.pSkyRenderTarget, pDepthBuffer);
		gVolumetricClouds.Load(&gSky.pSkyRenderTarget);

		gSpaceObjects.InitializeWithLoad(pDepthBuffer, pLinearDepthBuffer, gVolumetricClouds.pSavePrevTexture,
			gSky.GetParticleVertexBuffer(), gSky.GetParticleInstanceBuffer(), gSky.GetParticleCount(),
			sizeof(float) * 6, sizeof(ParticleData));
		gSpaceObjects.Load(&gSky.pSkyRenderTarget);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		RasterizerStateDesc rasterizerStateDesc = {};
		rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

		{
			PipelineDesc desc = {};
			desc.pCache = pPipelineCache;
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = NULL;
			pipelineSettings.pBlendState = NULL;
			pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
			pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
			pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
			pipelineSettings.pRootSignature = pExampleRootSignature;
			pipelineSettings.pRasterizerState = &rasterizerStateDesc;
			pipelineSettings.pShaderProgram = pFXAAShader;
			addPipeline(pRenderer, &desc, &pFXAAPipeline);
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////

		PipelineDesc LinearDepthResolvePipelineDesc = {};
		LinearDepthResolvePipelineDesc.pCache = pPipelineCache;
		{
			LinearDepthResolvePipelineDesc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc &pipelineSettings = LinearDepthResolvePipelineDesc.mGraphicsDesc;

			pipelineSettings = { 0 };
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = NULL;
			pipelineSettings.pColorFormats = &pLinearDepthBuffer->mFormat;
			pipelineSettings.mSampleCount = pLinearDepthBuffer->mSampleCount;
			pipelineSettings.mSampleQuality = pLinearDepthBuffer->mSampleQuality;
			pipelineSettings.pRootSignature = pExampleRootSignature;
			pipelineSettings.pShaderProgram = pLinearDepthResolveShader;
			pipelineSettings.pRasterizerState = &rasterizerStateDesc;

			addPipeline(pRenderer, &LinearDepthResolvePipelineDesc, &pLinearDepthResolvePipeline);
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/*PipelineDesc LinearDepthCompPipelineDesc = {};
		LinearDepthCompPipelineDesc.pCache = pPipelineCache;
		{
			LinearDepthCompPipelineDesc.mType = PIPELINE_TYPE_COMPUTE;
			ComputePipelineDesc &comPipelineSettings = LinearDepthCompPipelineDesc.mComputeDesc;

			comPipelineSettings = { 0 };
			comPipelineSettings.pShaderProgram = pLinearDepthCompShader;
			comPipelineSettings.pRootSignature = pLinearDepthCompRootSignature;
			addPipeline(pRenderer, &LinearDepthCompPipelineDesc, &pLinearDepthCompPipeline);
		}*/

		waitForAllResourceLoads();

		DescriptorData LinearDepthpparams[1] = {};
		LinearDepthpparams[0].pName = "SrcTexture";
		LinearDepthpparams[0].ppTextures = &pDepthBuffer->pTexture;
		updateDescriptorSet(pRenderer, 0, pExampleDescriptorSet, 1, LinearDepthpparams);

		DescriptorData Presentpparams[1] = {};
		Presentpparams[0].pName = "SrcTexture";
		Presentpparams[0].ppTextures = &(gSky.pSkyRenderTarget->pTexture);
		updateDescriptorSet(pRenderer, 1, pExampleDescriptorSet, 1, Presentpparams);

/*
		DescriptorData FXAApparams[1] = {};
		FXAApparams[0].pName = "SrcTexture";
		FXAApparams[0].ppTextures = &(gSky.pSkyRenderTarget->pTexture);
		updateDescriptorSet(pRenderer, 2, pExampleDescriptorSet, 1, FXAApparams);
*/
		return true;
	}

	void Unload()
	{
		waitQueueIdle(pGraphicsQueue);

		removeUserInterfacePipelines();

		removeFontSystemPipelines(); 

#if defined(TARGET_IOS) || defined(__ANDROID__)
		unloadVirtualJoystickUI(pVirtualJoystick);
#endif
		gTerrain.Unload();
		gSky.Unload();
		gVolumetricClouds.Unload();
		gSpaceObjects.Unload();

		removePipeline(pRenderer, pLinearDepthResolvePipeline);
		//removePipeline(pRenderer, pLinearDepthCompPipeline);
		removePipeline(pRenderer, pFXAAPipeline);
		removeRenderTarget(pRenderer, pDepthBuffer);
		removeRenderTarget(pRenderer, pLinearDepthBuffer);
		removeRenderTarget(pRenderer, pSkydomeResultRT);

		removeSwapChain(pRenderer, pSwapChain);
	}

	void Update(float deltaTime)
	{
		updateInputSystem(mSettings.mWidth, mSettings.mHeight);

#if !defined(TARGET_IOS)
		if (pSwapChain->mEnableVsync != gToggleVSync)
		{
			waitQueueIdle(pGraphicsQueue);
			::toggleVSync(pRenderer, &pSwapChain);
		}
#endif
		/************************************************************************/
		// Input
		/************************************************************************/

		pCameraController->update(deltaTime);
		/************************************************************************/
		// Scene Update
		/************************************************************************/
		static float currentTime = 0.0f;
		currentTime += deltaTime * 1000.0f;

		if (bSunMove)
		{
			LightDirection.y += deltaTime * SunMovingSpeed;

			if (LightDirection.y < 0.0f)
				LightDirection.y += 360.0f;

			LightDirection.y = fmodf(LightDirection.y, 360.0f);
		}

		float Azimuth = (PI / 180.0f) * LightDirection.x;
		float Elevation = (PI / 180.0f) * (LightDirection.y - 180.0f);		

		gSky.Azimuth = Azimuth;
		gSky.Elevation = Elevation;

		vec3 sunDirection = normalize(vec3(cosf(Azimuth)*cosf(Elevation), sinf(Elevation), sinf(Azimuth)*cosf(Elevation)));

		gSky.Update(deltaTime);
		gSky.LightDirection = v3ToF3(sunDirection);
		gSky.LightColorAndIntensity = LightColorAndIntensity;

		gTerrain.IsEnabledShadow = true;
		gTerrain.volumetricCloudsShadowCB.SettingInfo00 = vec4(1.0, gVolumetricClouds.volumetricCloudsCB.m_DataPerLayer[0].CloudCoverage, gVolumetricClouds.volumetricCloudsCB.m_DataPerLayer[0].WeatherTextureSize, 0.0);
		gTerrain.volumetricCloudsShadowCB.StandardPosition = gVolumetricClouds.volumetricCloudsCB.m_DataPerLayer[0].WindDirection;
		gTerrain.volumetricCloudsShadowCB.ShadowInfo = gVolumetricClouds.g_ShadowInfo;

		gTerrain.Update(deltaTime);
		gTerrain.LightDirection = v3ToF3(sunDirection);
		gTerrain.LightColorAndIntensity = LightColorAndIntensity;
		gTerrain.SunColor = gSky.GetSunColor();

		gVolumetricClouds.Update(deltaTime);
		gVolumetricClouds.LightDirection = v3ToF3(sunDirection);
		gVolumetricClouds.LightColorAndIntensity = LightColorAndIntensity;


		gSpaceObjects.Azimuth = Azimuth;
		gSpaceObjects.Elevation = Elevation;

		gSpaceObjects.Update(deltaTime);
		gSpaceObjects.LightDirection = v3ToF3(sunDirection);
		gSpaceObjects.LightColorAndIntensity = LightColorAndIntensity;
		
		gFXAAinfo.ScreenSize = vec2((float)mSettings.mWidth, (float)mSettings.mHeight);
		gFXAAinfo.Use = gToggleFXAA ? 1.0f : 0.0f;
		gFXAAinfo.Time = currentTime;

	}

	void Draw()
	{
		acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL, &gFrameIndex);

		// update camera with time
		//mat4 viewMat = pCameraController->getViewMatrix();

		Semaphore* pRenderCompleteSemaphore = pRenderCompleteSemaphores[gFrameIndex];
		Fence* pRenderCompleteFence = pRenderCompleteFences[gFrameIndex];

		// Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
		FenceStatus fenceStatus;
		getFenceStatus(pRenderer, pRenderCompleteFence, &fenceStatus);
		if (fenceStatus == FENCE_STATUS_INCOMPLETE)
			waitForFences(pRenderer, 1, &pRenderCompleteFence);

		resetCmdPool(pRenderer, pGraphicsCmdPools[gFrameIndex]);

		Cmd* cmd = pGraphicsCmds[gFrameIndex];
		beginCmd(cmd);

		cmdBeginGpuFrameProfile(cmd, gGpuProfileToken);

		RenderTarget* pRenderTarget = pTerrainResultRT;

		///////////////////////////////////////////////// Terrain ////////////////////////////////////////////////////

		gTerrain.gFrameIndex = gFrameIndex;
		gTerrain.Draw(cmd);

		struct CameraInfo
		{
			float nearPlane;
			float farPlane;
			float padding00;
			float padding01;
		};

		CameraInfo cameraInfo;
		cameraInfo.nearPlane = NEAR_CAMERA;
		cameraInfo.farPlane = FAR_CAMERA;
		///////////////////////////////////////////////// Depth Linearization ////////////////////////////////////////////////////
		{
			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Depth Linearization");

			RenderTargetBarrier barriersLinearDepth[] = {
				{ pLinearDepthBuffer, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET }
			};

			cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersLinearDepth);

			cmdBindRenderTargets(cmd, 1, &pLinearDepthBuffer, NULL, NULL, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pLinearDepthBuffer->mWidth, (float)pLinearDepthBuffer->mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pLinearDepthBuffer->mWidth, pLinearDepthBuffer->mHeight);

			cmdBindPipeline(cmd, pLinearDepthResolvePipeline);
			cmdBindPushConstants(cmd, pExampleRootSignature, gExampleRootConstantIndex, &cameraInfo);
			cmdBindDescriptorSet(cmd, 0, pExampleDescriptorSet);
			cmdDraw(cmd, 3, 0);

			cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

			RenderTargetBarrier barriersLinearDepthEnd[] = {
				{ pLinearDepthBuffer, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersLinearDepthEnd);

			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
		}

		///////////////////////////////////////////////// Sky ////////////////////////////////////////////////////

		gSky.gFrameIndex = gFrameIndex;
		gSky.Draw(cmd);

		///////////////////////////////////////////////// Volumetric Clouds ////////////////////////////////////////////////////

		gVolumetricClouds.Update(gFrameIndex);
		gVolumetricClouds.Draw(cmd);

		///////////////////////////////////////////////// Space Object ////////////////////////////////////////////////////

		gSpaceObjects.Draw(cmd);
		

		///////////////////////////////////////////////// FXAA ////////////////////////////////////////////////////////////////

		{
			if (gToggleFXAA)
				cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "FXAA");
			else
				cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "PresentPipeline");

			pRenderTarget = pSwapChain->ppRenderTargets[gFrameIndex];

			RenderTargetBarrier barriers[] =
			{
				{ pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET },
			};

			cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

			LoadActionsDesc loadActions = {};
			loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
			loadActions.mClearColorValues[0].r = 0.0f;
			loadActions.mClearColorValues[0].g = 0.0f;
			loadActions.mClearColorValues[0].b = 0.0f;
			loadActions.mClearColorValues[0].a = 0.0f;
			loadActions.mLoadActionDepth = LOAD_ACTION_DONTCARE;

			cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);			

			cmdBindPipeline(cmd, pFXAAPipeline);

			cmdBindDescriptorSet(cmd, 1, pExampleDescriptorSet);
			cmdBindPushConstants(cmd, pExampleRootSignature, gExampleRootConstantIndex, &gFXAAinfo);

			cmdDraw(cmd, 3, 0);

			cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
		}

		if (gShowUI)
		{
			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw UI");

			pRenderTarget = pSwapChain->ppRenderTargets[gFrameIndex];

			cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

			getHiresTimerUSec(&gTimer, true);

#if defined(TARGET_IOS) || defined(__ANDROID__)
			drawVirtualJoystickUI(pVirtualJoystick, cmd, pCameraController, { 1.0f, 1.0f, 1.0f, 1.0f });
#endif

			if (gTogglePerformance)
			{
				gFrameTimeDraw.mFontColor = 0xff00ffff;
				gFrameTimeDraw.mFontSize = 18.0f;
				gFrameTimeDraw.mFontID = gFontID;
                cmdDrawCpuProfile(cmd, float2(8.0f, 15.0f), &gFrameTimeDraw);
				cmdDrawGpuProfile(cmd, float2(8.0f, 40.0f), gGpuProfileToken, &gFrameTimeDraw);
			}

			cmdDrawUserInterface(cmd);

			cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);

			RenderTargetBarrier barriers[] = { { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT } };
			cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
		}

		cmdEndGpuFrameProfile(cmd, gGpuProfileToken);
		endCmd(cmd);

		QueueSubmitDesc submitDesc = {};
		submitDesc.mCmdCount = 1;
		submitDesc.mSignalSemaphoreCount = 1;
		submitDesc.mWaitSemaphoreCount = 1;
		submitDesc.ppCmds = &cmd;
		submitDesc.ppSignalSemaphores = &pRenderCompleteSemaphore;
		submitDesc.ppWaitSemaphores = &pImageAcquiredSemaphore;
		submitDesc.pSignalFence = pRenderCompleteFence;
		queueSubmit(pGraphicsQueue, &submitDesc);
		QueuePresentDesc presentDesc = {};
		presentDesc.mIndex = gFrameIndex;
		presentDesc.mWaitSemaphoreCount = 1;
		presentDesc.ppWaitSemaphores = &pRenderCompleteSemaphore;
		presentDesc.pSwapChain = pSwapChain;
		presentDesc.mSubmitDone = true;
		queuePresent(pGraphicsQueue, &presentDesc);
		
		flipProfiler();

		//for next frame
		gPrevFrameIndex = gFrameIndex;

		if (gVolumetricClouds.AfterSubmit(gPrevFrameIndex))
		{
			waitQueueIdle(pGraphicsQueue);

			gSpaceObjects.Unload();
			gVolumetricClouds.Unload();

			gVolumetricClouds.InitializeWithLoad(pLinearDepthBuffer, gSky.pSkyRenderTarget, pDepthBuffer);
			gVolumetricClouds.Load(&gSky.pSkyRenderTarget);

			gSpaceObjects.InitializeWithLoad(pDepthBuffer, pLinearDepthBuffer, gVolumetricClouds.pSavePrevTexture,
				gSky.GetParticleVertexBuffer(), gSky.GetParticleInstanceBuffer(), gSky.GetParticleCount(),
				sizeof(float) * 6, sizeof(ParticleData));
			gSpaceObjects.Load(&gSky.pSkyRenderTarget);

			gTerrain.pWeatherMap = gVolumetricClouds.GetWeatherMap();
		}
	}

	const char* GetName()
	{
		return "Ephemeris";
	}

	bool addSwapChain()
	{
		SwapChainDesc swapChainDesc = {};
		swapChainDesc.mWindowHandle = pWindow->handle;
		swapChainDesc.mPresentQueueCount = 1;
		swapChainDesc.ppPresentQueues = &pGraphicsQueue;
		swapChainDesc.mWidth = mSettings.mWidth;
		swapChainDesc.mHeight = mSettings.mHeight;
		swapChainDesc.mImageCount = gImageCount;
		swapChainDesc.mColorFormat = getRecommendedSwapchainFormat(true, true);
		swapChainDesc.mEnableVsync = false;
		::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

		return pSwapChain != NULL;
	}

	bool addDepthBuffer()
	{
		// Add depth buffer
		RenderTargetDesc depthRT = {};
		depthRT.mArraySize = 1;
		depthRT.mClearValue.depth = 1.0f;
		depthRT.mClearValue.stencil = 0;
		depthRT.mDepth = 1;
		depthRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
		depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
		depthRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
		depthRT.mHeight = mSettings.mHeight;
		depthRT.mSampleCount = SAMPLE_COUNT_1;
		depthRT.mSampleQuality = 0;
		depthRT.mWidth = mSettings.mWidth;
		addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);

		// Add linear depth Texture
		depthRT.mFormat = TinyImageFormat_R16_SFLOAT;
		depthRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
		depthRT.mWidth = mSettings.mWidth & (~63);
		depthRT.mHeight = mSettings.mHeight & (~63);
		addRenderTarget(pRenderer, &depthRT, &pLinearDepthBuffer);

		return pDepthBuffer != NULL && pLinearDepthBuffer != NULL;
	}

	bool addSceneRenderTarget()
	{
		RenderTargetDesc resultRT = {};
		resultRT.mArraySize = 1;
		resultRT.mDepth = 1;
		resultRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
		resultRT.mFormat = getRecommendedSwapchainFormat(true, true);
		resultRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
		resultRT.mSampleCount = SAMPLE_COUNT_1;
		resultRT.mSampleQuality = 0;

		resultRT.mWidth = mSettings.mWidth;
		resultRT.mHeight = mSettings.mHeight;

		addRenderTarget(pRenderer, &resultRT, &pSkydomeResultRT);

		return pSkydomeResultRT != NULL;
	}

	void RecenterCameraView(float maxDistance, const vec3& lookAt = vec3(0))
	{
		vec3 p = pCameraController->getViewPosition();
		vec3 d = p - lookAt;

		float lenSqr = lengthSqr(d);
		if (lenSqr > (maxDistance * maxDistance))
		{
			d *= (maxDistance / sqrtf(lenSqr));
		}

		p = d + lookAt;
		pCameraController->moveTo(p);
		pCameraController->lookAt(lookAt);
	}
};

DEFINE_APPLICATION_MAIN(RenderEphemeris)
