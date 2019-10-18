/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

//asimp importer
#include "../../../../The-Forge/Common_3/Tools/AssimpImporter/AssimpImporter.h"

//stl
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/vector.h"
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/string.h"

// Ephemeris BEGIN
#include "../../Terrain/src/Terrain.h"
#include "../../Sky/src/Sky.h"
#include "../../VolumetricClouds/src/VolumetricClouds.h"
// Ephemeris END

//Interfaces
#include "../../../../The-Forge/Common_3/OS/Interfaces/ICameraController.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IApp.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IInput.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/ITime.h"
#include "../../../../The-Forge/Middleware_3/UI/AppUI.h"
#include "../../../../The-Forge/Common_3/Renderer/IRenderer.h"
#include "../../../../The-Forge/Common_3/Renderer/ResourceLoader.h"
//#include "../../../../The-Forge/Common_3/Renderer/GpuProfiler.h"

//Math
#include "../../../../The-Forge/Common_3/OS/Math/MathTypes.h"

//Memory
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"

const uint32_t      gImageCount = 3;

static bool         gToggleVSync = false;
static bool         gTogglePerformance = true;
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

GpuProfiler*        pGraphicsGpuProfiler = NULL;
GuiComponent*       pMainGuiWindow = NULL;

Buffer*             pTransmittanceBuffer = NULL;

static float2       LightDirection = float2(0.0f, 270.0f);
static float4       LightColorAndIntensity = float4(1.0f, 1.0f, 1.0f, 1.0f);

#define NEAR_CAMERA 50.0f
#define FAR_CAMERA 100000000.0f

static uint gPrevFrameIndex = 0;
static bool bSunMove = true;
static float SunMovingSpeed = 5.0f;

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

#if defined(TARGET_IOS) || defined(__ANDROID__)
VirtualJoystickUI   gVirtualJoystick;
#endif

RasterizerState*	pPostProcessRast = NULL;

uint32_t			gFrameIndex = 0;

ICameraController*  pCameraController = NULL;

/// UI
UIApp				gAppUI;

VolumetricClouds	gVolumetricClouds;
Terrain				gTerrain;
Sky					gSky;

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
			PathHandle resourceDirRoot = fsAppendPathComponent(programDirectory, "../../../src/EphemerisExample");
			fsSetResourceDirectoryRootPath(resourceDirRoot);

			fsSetRelativePathForResourceDirectory(RD_TEXTURES,				"../../Resources/");
			fsSetRelativePathForResourceDirectory(RD_MESHES,					"../../Resources/");
			fsSetRelativePathForResourceDirectory(RD_BUILTIN_FONTS,		"../../Resources/Fonts");
			fsSetRelativePathForResourceDirectory(RD_ANIMATIONS,			"");
			fsSetRelativePathForResourceDirectory(RD_MIDDLEWARE_TEXT, "../../../../The-Forge/Middleware_3/Text/");
			fsSetRelativePathForResourceDirectory(RD_MIDDLEWARE_UI,		"../../../../The-Forge/Middleware_3/UI/");
		}

		// window and renderer setup
		RendererDesc settings = { 0 };
		initRenderer(GetName(), &settings, &pRenderer);
		//check for init success
		if (!pRenderer)
			return false;

		QueueDesc queueDesc = {};
		queueDesc.mType = CMD_POOL_DIRECT;
		addQueue(pRenderer, &queueDesc, &pGraphicsQueue);

		addCmdPool(pRenderer, pGraphicsQueue, false, &pGraphicsCmdPool);
		addCmd_n(pGraphicsCmdPool, false, gImageCount, &ppGraphicsCmds);

		addCmdPool(pRenderer, pGraphicsQueue, false, &pTransCmdPool);
		addCmd_n(pTransCmdPool, false, 1, &ppTransCmds);

		addFence(pRenderer, &pTransitionCompleteFences);

		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			addFence(pRenderer, &pRenderCompleteFences[i]);
			addSemaphore(pRenderer, &pRenderCompleteSemaphores[i]);
		}

		addSemaphore(pRenderer, &pImageAcquiredSemaphore);

		initResourceLoaderInterface(pRenderer);

		addGpuProfiler(pRenderer, pGraphicsQueue, &pGraphicsGpuProfiler, "GpuProfiler");

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

		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		ShaderLoadDesc basicShader = {};
		eastl::string basicShaderFullPath[2];
		ShaderPath(shaderPath, (char*)"Triangular.vert", basicShaderFullPath[0]);
		basicShader.mStages[0] = { basicShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
		ShaderPath(shaderPath, (char*)"present.frag", basicShaderFullPath[1]);
		basicShader.mStages[1] = { basicShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &basicShader, &pPresentShader);

		RootSignatureDesc rootDesc = {};
		rootDesc = { 0 };
		rootDesc.mShaderCount = 1;
		rootDesc.ppShaders = &pPresentShader;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////

		ShaderLoadDesc depthLinearizationResolveShader = {};
		eastl::string depthLinearizationResolveShaderFullPath[2];
		ShaderPath(shaderPath, (char*)"Triangular.vert", depthLinearizationResolveShaderFullPath[0]);
		depthLinearizationResolveShader.mStages[0] = { depthLinearizationResolveShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
		ShaderPath(shaderPath, (char*)"depthLinearization.frag", depthLinearizationResolveShaderFullPath[1]);
		depthLinearizationResolveShader.mStages[1] = { depthLinearizationResolveShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &depthLinearizationResolveShader, &pLinearDepthResolveShader);

		rootDesc = { 0 };
		rootDesc.mShaderCount = 1;
		rootDesc.ppShaders = &pLinearDepthResolveShader;

		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		ShaderLoadDesc depthLinearizationShader = {};
		eastl::string depthLinearizationShaderFullPath[2];
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
		Shader*           shaders[] = { pPresentShader, pLinearDepthResolveShader };
		rootDesc = {};
		rootDesc.mShaderCount = 2;
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
		screenQuadVbDesc.mDesc.mVertexStride = sizeof(float) * 5;
		screenQuadVbDesc.pData = screenQuadPoints;
		screenQuadVbDesc.ppBuffer = &pScreenQuadVertexBuffer;
		addResource(&screenQuadVbDesc);

		RasterizerStateDesc rasterizerStateDesc = {};
		rasterizerStateDesc.mCullMode = CULL_MODE_NONE;
		addRasterizerState(pRenderer, &rasterizerStateDesc, &pPostProcessRast);

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
		addResource(&TransBufferDesc);

		if (!gAppUI.Init(pRenderer))
			return false;

		gAppUI.LoadFont("TitilliumText/TitilliumText-Bold.otf", RD_BUILTIN_FONTS);

		CameraMotionParameters cmp{ 3200.0f, 12000.0f, 4000.0f };

		float h = 1250.0f;

		vec3 camPos{ 0.0f, h, -10.0f };
		vec3 lookAt{ 0.0f, h, 1.0f };

		pCameraController = createFpsCameraController(camPos, lookAt);
		//requestMouseCapture(true);

		pCameraController->setMotionParameters(cmp);

		//InputSystem::RegisterInputEvent(cameraInputEvent);

		gTerrain.Initialize(gImageCount, pCameraController, pGraphicsQueue, ppTransCmds, pTransitionCompleteFences, pGraphicsGpuProfiler, &gAppUI);
		gTerrain.Init(pRenderer);		

		gSky.Initialize(gImageCount, pCameraController, pGraphicsQueue, ppTransCmds, pTransitionCompleteFences, pGraphicsGpuProfiler, &gAppUI, pTransmittanceBuffer);
		gSky.Init(pRenderer);		

		gVolumetricClouds.Initialize(gImageCount, pCameraController, pGraphicsQueue, ppTransCmds, pTransitionCompleteFences, pRenderCompleteFences, pGraphicsGpuProfiler, &gAppUI, pTransmittanceBuffer);
		gVolumetricClouds.Init(pRenderer);

		GuiDesc guiDesc = {};
		guiDesc.mStartPosition = vec2(960.0f / getDpiScale().getX(), 700.0f / getDpiScale().getY());
		guiDesc.mStartSize = vec2(300.0f / getDpiScale().getX(), 250.0f / getDpiScale().getY());
		guiDesc.mDefaultTextDrawDesc = gDefaultTextDrawDesc;
		pMainGuiWindow = gAppUI.AddGuiComponent("Key Light", &guiDesc);

		SliderFloatWidget LD("Light Azimuth", &LightDirection.x, float(-180.0f), float(180.0f), float(0.001f));
		pMainGuiWindow->AddWidget(LD);

		SliderFloatWidget LE("Light Elevation", &LightDirection.y, float(0.0f), float(360.0f), float(0.001f));
		pMainGuiWindow->AddWidget(LE);

		SliderFloat4Widget LI("Light Color & Intensity", &LightColorAndIntensity, float(0.0f), float(10.0f), float(0.01f));
		pMainGuiWindow->AddWidget(LI);

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

		gVolumetricClouds.Exit();
		gSky.Exit();
		gTerrain.Exit();
		gAppUI.Exit();

		removeResource(pTransmittanceBuffer);
		removeResource(pScreenQuadVertexBuffer);

		removeSampler(pRenderer, pBilinearClampSampler);

		removeShader(pRenderer, pPresentShader);
		//removeRootSignature(pRenderer, pPresentRootSignature);
		//removeDescriptorBinder(pRenderer, pPresentDescriptorBinder);

		removeShader(pRenderer, pLinearDepthCompShader);
		removeRootSignature(pRenderer, pLinearDepthCompRootSignature);
		//removeDescriptorBinder(pRenderer, pLinearDepthCompDescriptorBinder);

		removeShader(pRenderer, pLinearDepthResolveShader);
		//removeRootSignature(pRenderer, pLinearDepthResolveRootSignature);
		//removeDescriptorBinder(pRenderer, pLinearDepthResolveDescriptorBinder);

		removeRootSignature(pRenderer, pExampleRootSignature);

		removeRasterizerState(pPostProcessRast);
		removeGpuProfiler(pRenderer, pGraphicsGpuProfiler);
		removeFence(pRenderer, pTransitionCompleteFences);

		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			removeFence(pRenderer, pRenderCompleteFences[i]);
			removeSemaphore(pRenderer, pRenderCompleteSemaphores[i]);
		}
		removeSemaphore(pRenderer, pImageAcquiredSemaphore);

		removeCmd_n(pGraphicsCmdPool, gImageCount, ppGraphicsCmds);
		removeCmdPool(pRenderer, pGraphicsCmdPool);

		removeCmd_n(pTransCmdPool, 1, ppTransCmds);
		removeCmdPool(pRenderer, pTransCmdPool);

		removeQueue(pGraphicsQueue);
		removeResourceLoaderInterface(pRenderer);

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

		if (!gAppUI.Load(pSwapChain->ppSwapchainRenderTargets))
			return false;

#if defined(TARGET_IOS) || defined(__ANDROID__)
		if (!gVirtualJoystick.Load(pSwapChain->ppSwapchainRenderTargets[0], pDepthBuffer->mDesc.mFormat))
			return false;
#endif

		gTerrain.pWeatherMap = gVolumetricClouds.GetWeatherMap();
		gTerrain.InitializeWithLoad(pDepthBuffer);
		gTerrain.Load(mSettings.mWidth, mSettings.mHeight);

		gSky.InitializeWithLoad(pDepthBuffer, pLinearDepthBuffer);
		gSky.Load(&gTerrain.pTerrainRT);

		gVolumetricClouds.InitializeWithLoad(pLinearDepthBuffer->pTexture, gSky.pSkyRenderTarget->pTexture, pDepthBuffer->pTexture);
		gVolumetricClouds.Load(&gSky.pSkyRenderTarget);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/*
		VertexLayout defaultVertexLayout = {};
		defaultVertexLayout.mAttribCount = 3;
		defaultVertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
		defaultVertexLayout.mAttribs[0].mFormat = ImageFormat::RGB32F;
		defaultVertexLayout.mAttribs[0].mBinding = 0;
		defaultVertexLayout.mAttribs[0].mLocation = 0;
		defaultVertexLayout.mAttribs[0].mOffUPDATE_FREQ_NONE;
		defaultVertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
		defaultVertexLayout.mAttribs[1].mFormat = ImageFormat::RGB32F;
		defaultVertexLayout.mAttribs[1].mLocation = 1;
		defaultVertexLayout.mAttribs[1].mBinding = 0;
		defaultVertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);
		defaultVertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD0;
		defaultVertexLayout.mAttribs[2].mFormat = ImageFormat::RG32F;
		defaultVertexLayout.mAttribs[2].mLocation = 2;
		defaultVertexLayout.mAttribs[2].mBinding = 0;
		defaultVertexLayout.mAttribs[2].mOffset = 6 * sizeof(float);
		 */

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

		PipelineDesc PresentPipelineDesc;
		{
			PresentPipelineDesc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc &pipelineSettings = PresentPipelineDesc.mGraphicsDesc;

			pipelineSettings = { 0 };
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = NULL;
			pipelineSettings.pColorFormats = &pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mFormat;
			//pipelineSettings.pSrgbValues = &pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSrgb;
			pipelineSettings.mSampleCount = pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleQuality;
			//pipelineSettings.pRootSignature = pPresentRootSignature;
			pipelineSettings.pRootSignature = pExampleRootSignature;
			pipelineSettings.pShaderProgram = pPresentShader;
			pipelineSettings.pVertexLayout = &vertexLayout;
			pipelineSettings.pRasterizerState = pPostProcessRast;

			addPipeline(pRenderer, &PresentPipelineDesc, &pPresentPipeline);
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
			pipelineSettings.pColorFormats = &pLinearDepthBuffer->mDesc.mFormat;
			//pipelineSettings.pSrgbValues = &pLinearDepthBuffer->mDesc.mSrgb;
			pipelineSettings.mSampleCount = pLinearDepthBuffer->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pLinearDepthBuffer->mDesc.mSampleQuality;
			//pipelineSettings.pRootSignature = pLinearDepthResolveRootSignature;
			pipelineSettings.pRootSignature = pExampleRootSignature;
			pipelineSettings.pShaderProgram = pLinearDepthResolveShader;
			pipelineSettings.pVertexLayout = &vertexLayout;
			pipelineSettings.pRasterizerState = pPostProcessRast;

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
		//Presentpparams[1].pName = "skydomeTexture";
		//Presentpparams[1].ppTextures = &pSkydomeResultRT->pTexture;
		Presentpparams[0].pName = "SrcTexture";
		Presentpparams[0].ppTextures = &(gSky.pSkyRenderTarget->pTexture);
		//Presentpparams[3].pName = "shadowTexture";
		//Presentpparams[3].ppTextures = &(gVolumetricClouds.pCastShadowRT->pTexture);
		updateDescriptorSet(pRenderer, 1, pExampleDescriptorSet, 1, Presentpparams);

		return true;
	}

	void Unload()
	{
		waitQueueIdle(pGraphicsQueue);

		gAppUI.Unload();

#if defined(TARGET_IOS) || defined(__ANDROID__)
		gVirtualJoystick.Unload();
#endif
		gTerrain.Unload();
		gSky.Unload();
		gVolumetricClouds.Unload();

		removePipeline(pRenderer, pLinearDepthResolvePipeline);
		removePipeline(pRenderer, pLinearDepthCompPipeline);
		removePipeline(pRenderer, pPresentPipeline);

		//removeRenderTarget(pRenderer, pSceneRT);
		removeRenderTarget(pRenderer, pDepthBuffer);
		removeRenderTarget(pRenderer, pLinearDepthBuffer);

		removeRenderTarget(pRenderer, pSkydomeResultRT);

		removeSwapChain(pRenderer, pSwapChain);
	}

	void Update(float deltaTime)
	{
		updateInputSystem(mSettings.mWidth, mSettings.mHeight);

#if !defined(TARGET_IOS)
		if (pSwapChain->mDesc.mEnableVsync != gToggleVSync)
		{
			waitQueueIdle(pGraphicsQueue);
			::toggleVSync(pRenderer, &pSwapChain);
		}
#endif
		/************************************************************************/
		// Input
		/************************************************************************/
		/*
		if (InputSystem::GetBoolInput(KEY_BUTTON_X_TRIGGERED))
		{
			gShowUI = !gShowUI;
		}

		if (InputSystem::GetBoolInput(KEY_BUTTON_Y_TRIGGERED))
		{
			gTogglePerformance = !gTogglePerformance;
		}
		*/

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
		gTerrain.volumetricCloudsShadowCB.SettingInfo00 = vec4(1.0, gVolumetricClouds.volumetricCloudsCB.CloudCoverage, gVolumetricClouds.volumetricCloudsCB.WeatherTextureSize, 0.0);
		gTerrain.volumetricCloudsShadowCB.StandardPosition = gVolumetricClouds.volumetricCloudsCB.WindDirection;
		gTerrain.volumetricCloudsShadowCB.ShadowInfo = gVolumetricClouds.g_ShadowInfo;

		gTerrain.Update(deltaTime);
		gTerrain.LightDirection = v3ToF3(sunDirection);
		gTerrain.LightColorAndIntensity = LightColorAndIntensity;
		gTerrain.SunColor = gSky.GetSunColor();

		gVolumetricClouds.Update(deltaTime);
		gVolumetricClouds.LightDirection = v3ToF3(sunDirection);
		gVolumetricClouds.LightColorAndIntensity = LightColorAndIntensity;

		if (gVolumetricClouds.AfterSubmit(gPrevFrameIndex))
		{
			waitQueueIdle(pGraphicsQueue);
			gVolumetricClouds.Unload();
			gVolumetricClouds.Load(&gSky.pSkyRenderTarget);
		}


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

		cmdBeginGpuFrameProfile(cmd, pGraphicsGpuProfiler);

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
			cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Depth Linearization", true);

			TextureBarrier barriersLinearDepth[] = {
			{ pDepthBuffer->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
			{ pLinearDepthBuffer->pTexture, RESOURCE_STATE_RENDER_TARGET }
			};

			cmdResourceBarrier(cmd, 0, NULL, 2, barriersLinearDepth);

			cmdBindRenderTargets(cmd, 1, &pLinearDepthBuffer, NULL, NULL, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pLinearDepthBuffer->mDesc.mWidth, (float)pLinearDepthBuffer->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pLinearDepthBuffer->mDesc.mWidth, pLinearDepthBuffer->mDesc.mHeight);

			cmdBindPipeline(cmd, pLinearDepthResolvePipeline);
			cmdBindPushConstants(cmd, pExampleRootSignature, "CameraInfoRootConstant", &cameraInfo);
			cmdBindDescriptorSet(cmd, 0, pExampleDescriptorSet);
			cmdBindVertexBuffer(cmd, 1, &pScreenQuadVertexBuffer, NULL);
			cmdDraw(cmd, 3, 0);

			cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

			TextureBarrier barriersLinearDepthEnd[] = {
			{ pLinearDepthBuffer->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, NULL, 1, barriersLinearDepthEnd);

			cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
		}

		///////////////////////////////////////////////// Sky ////////////////////////////////////////////////////

		gSky.gFrameIndex = gFrameIndex;
		gSky.Draw(cmd);

		///////////////////////////////////////////////// Volumetric Clouds ////////////////////////////////////////////////////

		gVolumetricClouds.Update(gFrameIndex);
		gVolumetricClouds.Draw(cmd);

		///////////////////////////////////////////////// Present Pipeline ////////////////////////////////////////////////////
		{
			cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "PresentPipeline", true);

			pRenderTarget = pSwapChain->ppSwapchainRenderTargets[gFrameIndex];

			TextureBarrier barriers88[] = {
					{ pRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET },
					{ pSkydomeResultRT->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, NULL, 2, barriers88);

			cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mDesc.mWidth, (float)pRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pRenderTarget->mDesc.mWidth, pRenderTarget->mDesc.mHeight);

			cmdBindPipeline(cmd, pPresentPipeline);
			cmdBindDescriptorSet(cmd, 1, pExampleDescriptorSet);
			cmdBindVertexBuffer(cmd, 1, &pScreenQuadVertexBuffer, NULL);
			cmdDraw(cmd, 3, 0);
			cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);


			cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
		}

		if (gShowUI)
		{
			cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw UI", true);

			pRenderTarget = pSwapChain->ppSwapchainRenderTargets[gFrameIndex];

			cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mDesc.mWidth, (float)pRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pRenderTarget->mDesc.mWidth, pRenderTarget->mDesc.mHeight);

			static HiresTimer gTimer;
			gTimer.GetUSec(true);

#if defined(TARGET_IOS) || defined(__ANDROID__)
			gVirtualJoystick.Draw(cmd, pCameraController, { 1.0f, 1.0f, 1.0f, 1.0f });
#endif

			if (gTogglePerformance)
			{
				gAppUI.DrawText(cmd, float2(8.0f, 15.0f), eastl::string().sprintf("CPU %f ms", gTimer.GetUSecAverage() / 1000.0f).c_str(), &gFrameTimeDraw);
				gAppUI.DrawText(cmd, float2(8.0f, 40.0f), eastl::string().sprintf("GPU %f ms", (float)pGraphicsGpuProfiler->mCumulativeTime * 1000.0f).c_str(), &gFrameTimeDraw);
#if !defined(METAL)
				gAppUI.DrawText(cmd, float2(8.0f, 300.0f), eastl::string().sprintf("Graphics Queue %f ms", (float)pGraphicsGpuProfiler->mCumulativeTime * 1000.0f).c_str(), &gFrameTimeDraw);
				gAppUI.DrawDebugGpuProfile(cmd, float2(8.0f, 325.0f), pGraphicsGpuProfiler, NULL);
#endif
			}

			gAppUI.Gui(pMainGuiWindow);
			gAppUI.Gui(gSky.pGuiWindow);
			gAppUI.Gui(gVolumetricClouds.pGuiWindow);

			gAppUI.Draw(cmd);

			cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);

			TextureBarrier barriers[] = { pRenderTarget->pTexture, RESOURCE_STATE_PRESENT };
			cmdResourceBarrier(cmd, 0, NULL, 1, barriers);

			cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
		}

		cmdEndGpuFrameProfile(cmd, pGraphicsGpuProfiler);
		endCmd(cmd);

		queueSubmit(pGraphicsQueue, 1, &cmd, pRenderCompleteFence, 1, &pImageAcquiredSemaphore, 1, &pRenderCompleteSemaphore);
		queuePresent(pGraphicsQueue, pSwapChain, gFrameIndex, 1, &pRenderCompleteSemaphore);

		//for next frame
		gPrevFrameIndex = gFrameIndex;
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
		swapChainDesc.mSampleCount = SAMPLE_COUNT_1;
		swapChainDesc.mColorFormat = getRecommendedSwapchainFormat(true);
		swapChainDesc.mEnableVsync = false;
		::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

		return pSwapChain != NULL;
	}

	void TransitionRenderTargets(RenderTarget *pRT, ResourceState state)
	{
		// Transition render targets to desired state
		beginCmd(ppTransCmds[0]);

		TextureBarrier barrier[] = {
			   { pRT->pTexture, state }
		};

		cmdResourceBarrier(ppTransCmds[0], 0, NULL, 1, barrier);
		endCmd(ppTransCmds[0]);
		queueSubmit(pGraphicsQueue, 1, &ppTransCmds[0], pTransitionCompleteFences, 0, NULL, 0, NULL);
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

	/*
	static bool cameraInputEvent(const ButtonData* data)
	{
		pCameraController->onInputEvent(data);

		return true;
	}
	*/
};

DEFINE_APPLICATION_MAIN(RenderEphemeris)
