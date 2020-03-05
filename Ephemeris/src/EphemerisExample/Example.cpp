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
#include "../../../../The-Forge/Middleware_3/UI/AppUI.h"
#include "../../../../The-Forge/Common_3/Renderer/IRenderer.h"
#include "../../../../The-Forge/Common_3/Renderer/IResourceLoader.h"

//Math
#include "../../../../The-Forge/Common_3/OS/Math/MathTypes.h"

//Memory
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"

const uint32_t      gImageCount = 3;

static bool         gToggleVSync = false;
static bool         gTogglePerformance = true;
static bool					gToggleFXAA = true;
static bool         gShowUI = true;

Renderer*           pRenderer = NULL;

Queue*              pGraphicsQueue = NULL;
CmdPool*            pGraphicsCmdPool = NULL;
Cmd**               ppGraphicsCmds = NULL;

CmdPool*            pTransCmdPool = NULL;
Cmd**               ppTransCmds = NULL;

SwapChain*          pSwapChain = NULL;
RenderTarget*       pDepthBuffer = NULL;

RenderTarget*       pLinearDepthBuffer = NULL;

Shader*             pLinearDepthResolveShader = NULL;
Pipeline*           pLinearDepthResolvePipeline = NULL;

Shader*             pLinearDepthCompShader = NULL;
Pipeline*           pLinearDepthCompPipeline = NULL;
RootSignature*      pLinearDepthCompRootSignature = NULL;

RenderTarget*       pTerrainResultRT = NULL;
RenderTarget*       pSkydomeResultRT = NULL;

Shader*							pFXAAShader = NULL;
Pipeline*						pFXAAPipeline = NULL;

Shader*             pPresentShader = NULL;
Pipeline*           pPresentPipeline = NULL;

DescriptorSet*      pExampleDescriptorSet = NULL;
RootSignature*      pExampleRootSignature = NULL;

Buffer*             pScreenQuadVertexBuffer = NULL;
Sampler*            pBilinearClampSampler = NULL;

Fence*              pRenderCompleteFences[gImageCount] = { NULL };
Fence*              pTransitionCompleteFences = NULL;

Semaphore*          pImageAcquiredSemaphore = NULL;
Semaphore*          pRenderCompleteSemaphores[gImageCount] = { NULL };

ProfileToken        gGpuProfileToken;
GuiComponent*       pMainGuiWindow = NULL;

Buffer*             pTransmittanceBuffer = NULL;

static float2				LightDirection = float2(0.0f, 270.0f);
static float4				LightColorAndIntensity = float4(1.0f, 1.0f, 1.0f, 1.0f);


#define NEAR_CAMERA 50.0f
#define FAR_CAMERA 100000000.0f

static uint				gPrevFrameIndex = 0;
static bool				bSunMove = true;
static float			SunMovingSpeed = 5.0f;

float							mChildIndent = 25.0f;
float							mHeightOffset = 20.0f;

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
VirtualJoystickUI   gVirtualJoystick;
#endif

uint32_t			gFrameIndex = 0;

ICameraController*  pCameraController = NULL;

/// UI
UIApp				gAppUI;

VolumetricClouds	gVolumetricClouds;
Terrain						gTerrain;
Sky								gSky;
SpaceObjects			gSpaceObjects;

TextDrawDesc gFrameTimeDraw = TextDrawDesc(0, 0xff00ffff, 18);
TextDrawDesc gDefaultTextDrawDesc = TextDrawDesc(0, 0xffffffff, 16);

static void ShaderPath(const eastl::string &shaderPath, char* pShaderName, eastl::string &result)
{
	result.resize(0);
	eastl::string shaderName(pShaderName);
	result = shaderPath + shaderName;
}

class RenderEphemeris : public IApp
{
public:

	bool Init()
	{
		// FILE PATHS
		PathHandle programDirectory = fsCopyProgramDirectoryPath();
		if (!fsPlatformUsesBundledResources())
		{
			PathHandle resourceDirRoot = fsAppendPathComponent(programDirectory, "../../../../../../Custom-Middleware/Ephemeris/src/EphemerisExample");
			fsSetResourceDirectoryRootPath(resourceDirRoot);

			fsSetRelativePathForResourceDirectory(RD_TEXTURES, "../../Resources/");
			fsSetRelativePathForResourceDirectory(RD_MESHES, "../../Resources/");
			fsSetRelativePathForResourceDirectory(RD_BUILTIN_FONTS, "../../Resources/Fonts");
			fsSetRelativePathForResourceDirectory(RD_ANIMATIONS, "");
			fsSetRelativePathForResourceDirectory(RD_MIDDLEWARE_TEXT, "../../../../The-Forge/Middleware_3/Text/");
			fsSetRelativePathForResourceDirectory(RD_MIDDLEWARE_UI, "../../../../The-Forge/Middleware_3/UI/");
		}

		// window and renderer setup
		RendererDesc settings = { 0 };
		initRenderer(GetName(), &settings, &pRenderer);
		//check for init success
		if (!pRenderer)
			return false;

		QueueDesc queueDesc = {};
		queueDesc.mType = QUEUE_TYPE_GRAPHICS;
		queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
		addQueue(pRenderer, &queueDesc, &pGraphicsQueue);
		CmdPoolDesc cmdPoolDesc = {};
		cmdPoolDesc.pQueue = pGraphicsQueue;
		addCmdPool(pRenderer, &cmdPoolDesc, &pGraphicsCmdPool);
		CmdDesc cmdDesc = {};
		cmdDesc.pPool = pGraphicsCmdPool;
		addCmd_n(pRenderer, &cmdDesc, gImageCount, &ppGraphicsCmds);

		addCmdPool(pRenderer, &cmdPoolDesc, &pTransCmdPool);
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

		initProfiler();

        gGpuProfileToken = addGpuProfiler(pRenderer, pGraphicsQueue, "GpuProfiler");

#if defined(__ANDROID__) || defined(TARGET_IOS)
		if (!gVirtualJoystick.Init(pRenderer, "circlepad.png", FSR_Textures))
		{
			LOGERRORF("Could not initialize Virtual Joystick.");
			return false;
		}
#endif

#if defined(METAL)
		gFrameTimeDraw.mFontSize /= getDpiScale().getY();
		gDefaultTextDrawDesc.mFontSize /= getDpiScale().getY();
#endif


		SamplerDesc samplerClampDesc = {
		FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
		ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE
		};
		addSampler(pRenderer, &samplerClampDesc, &pBilinearClampSampler);

		eastl::string shaderPath("");
		//eastl::string shaderFullPath;

		RootSignatureDesc rootDesc = {};

		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		ShaderLoadDesc basicShader = {};
		eastl::string basicShaderFullPath[2];
		ShaderPath(shaderPath, (char*)"Triangular.vert", basicShaderFullPath[0]);
		basicShader.mStages[0] = { basicShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
		ShaderPath(shaderPath, (char*)"present.frag", basicShaderFullPath[1]);
		basicShader.mStages[1] = { basicShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &basicShader, &pPresentShader);		

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////

		ShaderLoadDesc FXAAShader = {};
		eastl::string fxaaShaderFullPath[2];
		ShaderPath(shaderPath, (char*)"Triangular.vert", fxaaShaderFullPath[0]);
		FXAAShader.mStages[0] = { fxaaShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
		ShaderPath(shaderPath, (char*)"FXAA.frag", fxaaShaderFullPath[1]);
		FXAAShader.mStages[1] = { fxaaShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

		addShader(pRenderer, &FXAAShader, &pFXAAShader);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////

		ShaderLoadDesc depthLinearizationResolveShader = {};
		eastl::string depthLinearizationResolveShaderFullPath[2];
		ShaderPath(shaderPath, (char*)"Triangular.vert", depthLinearizationResolveShaderFullPath[0]);
		depthLinearizationResolveShader.mStages[0] = { depthLinearizationResolveShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
		ShaderPath(shaderPath, (char*)"depthLinearization.frag", depthLinearizationResolveShaderFullPath[1]);
		depthLinearizationResolveShader.mStages[1] = { depthLinearizationResolveShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &depthLinearizationResolveShader, &pLinearDepthResolveShader);	

		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		ShaderLoadDesc depthLinearizationShader = {};
		eastl::string depthLinearizationShaderFullPath[1];
		ShaderPath(shaderPath, (char*)"depthLinearization.comp", depthLinearizationShaderFullPath[0]);
		depthLinearizationShader.mStages[0] = { depthLinearizationShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &depthLinearizationShader, &pLinearDepthCompShader);

		rootDesc = {};
		rootDesc = { 0 };
		rootDesc.mShaderCount = 1;
		rootDesc.ppShaders = &pLinearDepthCompShader;

		addRootSignature(pRenderer, &rootDesc, &pLinearDepthCompRootSignature);
		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		const char* pStaticSamplerNames[] = { "g_LinearClamp" };
		Sampler* pStaticSamplers[] = { pBilinearClampSampler };
		Shader*           shaders[] = { pPresentShader, pFXAAShader, pLinearDepthResolveShader };
		rootDesc = {};
		rootDesc.mShaderCount = 3;
		rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
		rootDesc.ppStaticSamplers = pStaticSamplers;
		rootDesc.ppShaders = shaders;
		rootDesc.mStaticSamplerCount = 1;
		addRootSignature(pRenderer, &rootDesc, &pExampleRootSignature);

		DescriptorSetDesc setDesc = { pExampleRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 2 };
		addDescriptorSet(pRenderer, &setDesc, &pExampleDescriptorSet);

		float screenQuadPoints[] = {
				-1.0f,  3.0f, 0.5f, 0.0f, -1.0f,
				-1.0f, -1.0f, 0.5f, 0.0f, 1.0f,
				3.0f, -1.0f, 0.5f, 2.0f, 1.0f,
		};

		uint64_t screenQuadDataSize = 5 * 3 * sizeof(float);
		BufferLoadDesc screenQuadVbDesc = {};
		screenQuadVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
		screenQuadVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
		screenQuadVbDesc.mDesc.mSize = screenQuadDataSize;
		screenQuadVbDesc.pData = screenQuadPoints;
		screenQuadVbDesc.ppBuffer = &pScreenQuadVertexBuffer;
		addResource(&screenQuadVbDesc, NULL, LOAD_PRIORITY_NORMAL);

		// Add TransmittanceBuffer buffer
		BufferLoadDesc TransBufferDesc = {};
		TransBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_RW_BUFFER | DESCRIPTOR_TYPE_BUFFER;
		TransBufferDesc.mDesc.mElementCount = 3;
		TransBufferDesc.mDesc.mStructStride = sizeof(float4);
		TransBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;

#ifdef METAL
		TransBufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
#endif

		TransBufferDesc.mDesc.mSize = TransBufferDesc.mDesc.mStructStride * TransBufferDesc.mDesc.mElementCount;

		//TransBufferDesc.pData = gInitializeVal.data();
		TransBufferDesc.ppBuffer = &pTransmittanceBuffer;
		addResource(&TransBufferDesc, NULL, LOAD_PRIORITY_NORMAL);

		if (!gAppUI.Init(pRenderer))
			return false;

		gAppUI.LoadFont("TitilliumText/TitilliumText-Bold.otf", RD_BUILTIN_FONTS);

		CameraMotionParameters cmp{ 32000.0f, 120000.0f, 40000.0f };

		float h = 6000.0f;

		vec3 camPos{ 0.0f, h, -10.0f };
		vec3 lookAt{ 0.0f, h, 1.0f };

		pCameraController = createFpsCameraController(camPos, lookAt);

		pCameraController->setMotionParameters(cmp);

		gTerrain.Initialize(gImageCount, pCameraController, pGraphicsQueue, ppTransCmds, pTransitionCompleteFences, gGpuProfileToken, &gAppUI);
		gTerrain.Init(pRenderer);		

		gSky.Initialize(gImageCount, pCameraController, pGraphicsQueue, ppTransCmds, pTransitionCompleteFences, gGpuProfileToken, &gAppUI, pTransmittanceBuffer);
		gSky.Init(pRenderer);		

		gVolumetricClouds.Initialize(gImageCount, pCameraController, pGraphicsQueue, ppTransCmds, pTransitionCompleteFences, pRenderCompleteFences, gGpuProfileToken, &gAppUI, pTransmittanceBuffer);
		gVolumetricClouds.Init(pRenderer);

		gSpaceObjects.Initialize(gImageCount, pCameraController, pGraphicsQueue, ppTransCmds, pTransitionCompleteFences, gGpuProfileToken, &gAppUI, pTransmittanceBuffer);
		gSpaceObjects.Init(pRenderer);

		GuiDesc guiDesc = {};
		guiDesc.mStartPosition = vec2(960.0f / getDpiScale().getX(), 700.0f / getDpiScale().getY());
		guiDesc.mStartSize = vec2(300.0f / getDpiScale().getX(), 250.0f / getDpiScale().getY());
		guiDesc.mDefaultTextDrawDesc = gDefaultTextDrawDesc;
		pMainGuiWindow = gAppUI.AddGuiComponent("Global Settings", &guiDesc);

		pMainGuiWindow->AddWidget(CheckboxWidget("Enable FXAA", &gToggleFXAA));

		pMainGuiWindow->AddWidget(SeparatorWidget());

		

		SliderFloatWidget LD("Light Azimuth", &LightDirection.x, float(-180.0f), float(180.0f), float(0.001f));
		pMainGuiWindow->AddWidget(LD);

		SliderFloatWidget LE("Light Elevation", &LightDirection.y, float(0.0f), float(360.0f), float(0.001f));
		pMainGuiWindow->AddWidget(LE);

		pMainGuiWindow->AddWidget(SeparatorWidget());

		SliderFloat4Widget LI("Light Color & Intensity", &LightColorAndIntensity, float(0.0f), float(10.0f), float(0.01f));
		pMainGuiWindow->AddWidget(LI);

		pMainGuiWindow->AddWidget(SeparatorWidget());

		CheckboxWidget SM("Automatic Sun Moving", &bSunMove);
		pMainGuiWindow->AddWidget(SM);

		SliderFloatWidget SS("Sun Moving Speed", &SunMovingSpeed, float(-100.0f), float(100.0f), float(0.01f));
		pMainGuiWindow->AddWidget(SS);


		if (!initInputSystem(pWindow))
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
				bool capture = gAppUI.OnButton(ctx->mBinding, ctx->mBool, ctx->pPosition);
				setEnableCaptureInput(capture && INPUT_ACTION_PHASE_CANCELED != ctx->mPhase);
				return true;
			}, this
		};
		addInputAction(&actionDesc);
		typedef bool(*CameraInputHandler)(InputActionContext* ctx, uint32_t index);
		static CameraInputHandler onCameraInput = [](InputActionContext* ctx, uint32_t index)
		{
			if (!gAppUI.IsFocused() && *ctx->pCaptured)
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

		return true;
	}

	void Exit()
	{
		waitQueueIdle(pGraphicsQueue);
		destroyCameraController(pCameraController);

		exitInputSystem();
		removeDescriptorSet(pRenderer, pExampleDescriptorSet);

#if defined(TARGET_IOS) || defined(__ANDROID__)
		gVirtualJoystick.Exit();
#endif

		gSpaceObjects.Exit();
		gVolumetricClouds.Exit();
		gSky.Exit();
		gTerrain.Exit();
		gAppUI.Exit();

		exitProfiler();

		removeResource(pTransmittanceBuffer);
		removeResource(pScreenQuadVertexBuffer);

		removeSampler(pRenderer, pBilinearClampSampler);

		removeShader(pRenderer, pPresentShader);
		//removeRootSignature(pRenderer, pPresentRootSignature);
		//removeDescriptorBinder(pRenderer, pPresentDescriptorBinder);
		removeShader(pRenderer, pFXAAShader);
		removeShader(pRenderer, pLinearDepthCompShader);
		removeRootSignature(pRenderer, pLinearDepthCompRootSignature);
		//removeDescriptorBinder(pRenderer, pLinearDepthCompDescriptorBinder);

		removeShader(pRenderer, pLinearDepthResolveShader);
		//removeRootSignature(pRenderer, pLinearDepthResolveRootSignature);
		//removeDescriptorBinder(pRenderer, pLinearDepthResolveDescriptorBinder);

		removeRootSignature(pRenderer, pExampleRootSignature);

		removeFence(pRenderer, pTransitionCompleteFences);

		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			removeFence(pRenderer, pRenderCompleteFences[i]);
			removeSemaphore(pRenderer, pRenderCompleteSemaphores[i]);
		}
		removeSemaphore(pRenderer, pImageAcquiredSemaphore);

		removeCmd_n(pRenderer, gImageCount, ppGraphicsCmds);
		removeCmdPool(pRenderer, pGraphicsCmdPool);

		removeCmd_n(pRenderer, 1, ppTransCmds);
		removeCmdPool(pRenderer, pTransCmdPool);

		removeQueue(pRenderer, pGraphicsQueue);
		exitResourceLoaderInterface(pRenderer);

		removeRenderer(pRenderer);
	}

	bool Load()
	{
		if (!addSwapChain())
			return false;

		if (!addSceneRenderTarget())
			return false;


		if (!addDepthBuffer())
			return false;

		if (!gAppUI.Load(pSwapChain->ppRenderTargets))
			return false;

#if defined(TARGET_IOS) || defined(__ANDROID__)
		if (!gVirtualJoystick.Load(pSwapChain->ppRenderTargets[0], pDepthBuffer->mFormat))
			return false;
#endif

		loadProfilerUI(&gAppUI, mSettings.mWidth, mSettings.mHeight);

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

		//layout and pipeline for ScreenQuad
		VertexLayout vertexLayout = {};
		vertexLayout.mAttribCount = 2;
		vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
		vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
		vertexLayout.mAttribs[0].mBinding = 0;
		vertexLayout.mAttribs[0].mLocation = 0;
		vertexLayout.mAttribs[0].mOffset = 0;

		vertexLayout.mAttribs[1].mSemantic = SEMANTIC_TEXCOORD0;
		vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32_SFLOAT;
		vertexLayout.mAttribs[1].mBinding = 0;
		vertexLayout.mAttribs[1].mLocation = 1;
		vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		RasterizerStateDesc rasterizerStateDesc = {};
		rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

		PipelineDesc PresentPipelineDesc;
		{
			PresentPipelineDesc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc &pipelineSettings = PresentPipelineDesc.mGraphicsDesc;

			pipelineSettings = { 0 };
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = NULL;
			pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
			pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
			pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
			pipelineSettings.pRootSignature = pExampleRootSignature;
			pipelineSettings.pShaderProgram = pPresentShader;
			pipelineSettings.pVertexLayout = &vertexLayout;
			pipelineSettings.pRasterizerState = &rasterizerStateDesc;

			addPipeline(pRenderer, &PresentPipelineDesc, &pPresentPipeline);
		}

		{
			PipelineDesc desc = {};
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
			pipelineSettings.pVertexLayout = &vertexLayout;
			pipelineSettings.pRasterizerState = &rasterizerStateDesc;
			pipelineSettings.pShaderProgram = pFXAAShader;
			addPipeline(pRenderer, &desc, &pFXAAPipeline);
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////

		PipelineDesc LinearDepthResolvePipelineDesc;
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
			pipelineSettings.pVertexLayout = &vertexLayout;
			pipelineSettings.pRasterizerState = &rasterizerStateDesc;

			addPipeline(pRenderer, &LinearDepthResolvePipelineDesc, &pLinearDepthResolvePipeline);
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		PipelineDesc LinearDepthCompPipelineDesc;
		{
			LinearDepthCompPipelineDesc.mType = PIPELINE_TYPE_COMPUTE;
			ComputePipelineDesc &comPipelineSettings = LinearDepthCompPipelineDesc.mComputeDesc;

			comPipelineSettings = { 0 };
			comPipelineSettings.pShaderProgram = pLinearDepthCompShader;
			comPipelineSettings.pRootSignature = pLinearDepthCompRootSignature;
			addPipeline(pRenderer, &LinearDepthCompPipelineDesc, &pLinearDepthCompPipeline);
		}

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

		unloadProfilerUI();
		gAppUI.Unload();

#if defined(TARGET_IOS) || defined(__ANDROID__)
		gVirtualJoystick.Unload();
#endif
		gTerrain.Unload();
		gSky.Unload();
		gVolumetricClouds.Unload();
		gSpaceObjects.Unload();

		removePipeline(pRenderer, pLinearDepthResolvePipeline);
		removePipeline(pRenderer, pLinearDepthCompPipeline);
		removePipeline(pRenderer, pPresentPipeline);
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

		/************************************************************************/
		// UI
		/************************************************************************/
		gAppUI.Update(deltaTime);

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
		
		Cmd* cmd = ppGraphicsCmds[gFrameIndex];
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

		const uint32_t quadStride = sizeof(float) * 5;

		///////////////////////////////////////////////// Depth Linearization ////////////////////////////////////////////////////
		{
			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Depth Linearization");

			RenderTargetBarrier barriersLinearDepth[] = {
			{ pDepthBuffer, RESOURCE_STATE_SHADER_RESOURCE },
			{ pLinearDepthBuffer, RESOURCE_STATE_RENDER_TARGET }
			};

			cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, barriersLinearDepth);

			cmdBindRenderTargets(cmd, 1, &pLinearDepthBuffer, NULL, NULL, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pLinearDepthBuffer->mWidth, (float)pLinearDepthBuffer->mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pLinearDepthBuffer->mWidth, pLinearDepthBuffer->mHeight);

			cmdBindPipeline(cmd, pLinearDepthResolvePipeline);
			cmdBindPushConstants(cmd, pExampleRootSignature, "CameraInfoRootConstant", &cameraInfo);
			cmdBindDescriptorSet(cmd, 0, pExampleDescriptorSet);
			cmdBindVertexBuffer(cmd, 1, &pScreenQuadVertexBuffer, &quadStride, NULL);
			cmdDraw(cmd, 3, 0);

			cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

			RenderTargetBarrier barriersLinearDepthEnd[] = {
			{ pLinearDepthBuffer, RESOURCE_STATE_SHADER_RESOURCE }
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
				{ pRenderTarget, RESOURCE_STATE_RENDER_TARGET },
				{ pSkydomeResultRT, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, barriers);

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
			cmdBindPushConstants(cmd, pExampleRootSignature, "FXAARootConstant", &gFXAAinfo);

			cmdBindVertexBuffer(cmd, 1, &pScreenQuadVertexBuffer, &quadStride, NULL);
			cmdDraw(cmd, 3, 0);

			cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
		}
/*
		///////////////////////////////////////////////// Present Pipeline ////////////////////////////////////////////////////
		{
			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "PresentPipeline", true);

			pRenderTarget = pSwapChain->ppRenderTargets[gFrameIndex];

			TextureBarrier barriers88[] = {
					{ pRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET },
					{ pSkydomeResultRT->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, NULL, 2, barriers88);

			cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

			cmdBindPipeline(cmd, pPresentPipeline);
			cmdBindDescriptorSet(cmd, 1, pExampleDescriptorSet);
			cmdBindVertexBuffer(cmd, 1, &pScreenQuadVertexBuffer, NULL);
			cmdDraw(cmd, 3, 0);
			cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);


			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
		}
*/
		if (gShowUI)
		{
			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw UI");

			pRenderTarget = pSwapChain->ppRenderTargets[gFrameIndex];

			cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

			static HiresTimer gTimer;
			gTimer.GetUSec(true);

#if defined(TARGET_IOS) || defined(__ANDROID__)
			gVirtualJoystick.Draw(cmd, pCameraController, { 1.0f, 1.0f, 1.0f, 1.0f });
#endif

			if (gTogglePerformance)
			{
                cmdDrawCpuProfile(cmd, float2(8.0f, 15.0f), &gFrameTimeDraw);
				cmdDrawGpuProfile(cmd, float2(8.0f, 40.0f), gGpuProfileToken);

				cmdDrawProfilerUI();
			}
			
			gAppUI.Gui(pMainGuiWindow);
			gAppUI.Gui(gSky.pGuiWindow);
			gAppUI.Gui(gVolumetricClouds.pGuiWindow);

			gAppUI.Draw(cmd);

			cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);

			RenderTargetBarrier barriers[] = { { pRenderTarget, RESOURCE_STATE_PRESENT } };
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
		swapChainDesc.mColorFormat = getRecommendedSwapchainFormat(true);
		swapChainDesc.mEnableVsync = false;
		::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

		return pSwapChain != NULL;
	}

	void TransitionRenderTargets(RenderTarget *pRT, ResourceState state)
	{
		// Transition render targets to desired state
		beginCmd(ppTransCmds[0]);

		RenderTargetBarrier barrier[] = {
			   { pRT, state }
		};

		cmdResourceBarrier(ppTransCmds[0], 0, NULL, 0, NULL, 1, barrier);
		endCmd(ppTransCmds[0]);
		QueueSubmitDesc submitDesc = {};
		submitDesc.mCmdCount = 1;
		submitDesc.ppCmds = &ppTransCmds[0];
		submitDesc.pSignalFence = pTransitionCompleteFences;
		submitDesc.mSubmitDone = true;
		queueSubmit(pGraphicsQueue, &submitDesc);
		waitForFences(pRenderer, 1, &pTransitionCompleteFences);
	}

	bool addDepthBuffer()
	{
		// Add depth buffer
		RenderTargetDesc depthRT = {};
		depthRT.mArraySize = 1;
		depthRT.mClearValue.depth = 1.0f;
		depthRT.mClearValue.stencil = 0;
		depthRT.mDepth = 1;
		depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
		depthRT.mHeight = mSettings.mHeight;
		depthRT.mSampleCount = SAMPLE_COUNT_1;
		depthRT.mSampleQuality = 0;
		depthRT.mWidth = mSettings.mWidth;
#if defined(METAL)
		depthRT.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
#endif
		addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);

#if defined(VULKAN)
		TransitionRenderTargets(pDepthBuffer, ResourceState::RESOURCE_STATE_DEPTH_WRITE);
#endif

		// Add linear depth Texture
		depthRT.mFormat = TinyImageFormat_R16_SFLOAT;
		depthRT.mWidth = mSettings.mWidth & (~63);
		depthRT.mHeight = mSettings.mHeight & (~63);
		addRenderTarget(pRenderer, &depthRT, &pLinearDepthBuffer);

#if defined(VULKAN)
		TransitionRenderTargets(pLinearDepthBuffer, ResourceState::RESOURCE_STATE_UNORDERED_ACCESS);
#endif

		return pDepthBuffer != NULL && pLinearDepthBuffer != NULL;
	}

	bool addSceneRenderTarget()
	{
		RenderTargetDesc resultRT = {};
		resultRT.mArraySize = 1;
		resultRT.mDepth = 1;
		resultRT.mFormat = TinyImageFormat_R8G8B8A8_UNORM;
		resultRT.mSampleCount = SAMPLE_COUNT_1;
		resultRT.mSampleQuality = 0;

		resultRT.mWidth = mSettings.mWidth;
		resultRT.mHeight = mSettings.mHeight;

		addRenderTarget(pRenderer, &resultRT, &pSkydomeResultRT);

#if defined(VULKAN)
		TransitionRenderTargets(pSkydomeResultRT, ResourceState::RESOURCE_STATE_RENDER_TARGET);
#endif

		return pSkydomeResultRT != NULL;
	}

	void RecenterCameraView(float maxDistance, vec3 lookAt = vec3(0))
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
