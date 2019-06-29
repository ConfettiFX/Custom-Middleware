/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "Sky.h"

#define SKY_NEAR 50.0f
#define SKY_FAR 100000000.0f

int				gNumberOfSpherePoints;
const int		gSphereResolution = 30;    // Increase for higher resolution spheres
const float		gSphereDiameter = 0.5f;

//Precomputed Atmospheric Sacttering
Shader*				pPAS_Shader = NULL;
Pipeline*			pPAS_Pipeline = NULL;

Shader*   pSpaceShader = NULL;
Pipeline* pSpacePipeline = NULL;

BlendState*			pBlendStateSpace;
BlendState*			pBlendStateSun;

Shader*   pSunShader = NULL;
Pipeline* pSunPipeline = NULL;

Shader*   pMoonShader = NULL;
Pipeline* pMoonPipeline = NULL;

DescriptorBinder* pSkyDescriptorBinder = NULL;
RootSignature*    pSkyRootSignature = NULL;

Buffer*   pSphereVertexBuffer = NULL;
Buffer*   pRenderSkyUniformBuffer[3] = {NULL};
Buffer*   pSpaceUniformBuffer[3] = { NULL };
Buffer*   pSunUniformBuffer[3] = { NULL };

mat4	  ViewProjMat;

static float g_ElapsedTime = 0.0f;

typedef struct SkySettings
{
	float4		SkyInfo; // x: fExposure, y: fInscatterIntencity, z: fInscatterContrast, w: fUnitsToM
	float4		OriginLocation;
} SkySettings;

SkySettings	gSkySettings;

struct RenderSkyUniformBuffer
{
  mat4	InvViewMat;
  mat4	InvProjMat;

  float4	LightDirection; // w:exposure
  float4	CameraPosition;
  float4	QNNear;
  float4	InScatterParams;

  float4	LightIntensity;
};

struct SpaceUniformBuffer
{
	mat4 ViewProjMat;
	float4 LightDirection;
	float4 ScreenSize;
};

struct SunUniformBuffer
{
  mat4 ViewMat;
  mat4 ViewProjMat;
  float4 LightDirection;
  float4 Dx;
  float4 Dy;
};

#if defined(VULKAN)
static void TransitionRenderTargets(RenderTarget *pRT, ResourceState state, Renderer* renderer, Cmd* cmd, Queue* queue, Fence* fence)
{
  // Transition render targets to desired state
  beginCmd(cmd);

  TextureBarrier barrier[] = {
         { pRT->pTexture, state }
  };

  cmdResourceBarrier(cmd, 0, NULL, 1, barrier, false);
  endCmd(cmd);

  queueSubmit(queue, 1, &cmd, fence, 0, NULL, 0, NULL);
  waitForFences(renderer, 1, &fence);
}
#endif


static void ShaderPath(const eastl::string &shaderPath, char* pShaderName, eastl::string &result)
{
	result.resize(0);
  eastl::string shaderName(pShaderName);
	result = shaderPath + shaderName;
}

void Sky::CalculateLookupData()
{
	TextureLoadDesc SkyTransmittanceTextureDesc = {};
  SkyTransmittanceTextureDesc.mRoot = FSR_OtherFiles;
#if defined(_DURANGO)
  SkyTransmittanceTextureDesc.pFilename = "Textures/Transmittance.dds";
#else
  SkyTransmittanceTextureDesc.pFilename = "../../../../Ephemeris/Sky/resources/Textures/Transmittance.dds";
#endif	
	SkyTransmittanceTextureDesc.ppTexture = &pTransmittanceTexture;
	addResource(&SkyTransmittanceTextureDesc, false);

	TextureLoadDesc SkyIrradianceTextureDesc = {};
	SkyIrradianceTextureDesc.mRoot = FSR_OtherFiles;
#if defined(_DURANGO)
  SkyIrradianceTextureDesc.pFilename = "Textures/Irradiance.dds";
#else
  SkyIrradianceTextureDesc.pFilename = "../../../../Ephemeris/Sky/resources/Textures/Irradiance.dds";
#endif
	SkyIrradianceTextureDesc.ppTexture = &pIrradianceTexture;
	addResource(&SkyIrradianceTextureDesc, false);

	TextureLoadDesc SkyInscatterTextureDesc = {};
	SkyInscatterTextureDesc.mRoot = FSR_OtherFiles;
#if defined(_DURANGO)
  SkyInscatterTextureDesc.pFilename = "Textures/Inscatter.dds";
#else
  SkyInscatterTextureDesc.pFilename = "../../../../Ephemeris/Sky/resources/Textures/Inscatter.dds";
#endif
	SkyInscatterTextureDesc.ppTexture = &pInscatterTexture;
	addResource(&SkyInscatterTextureDesc, false);

	TextureLoadDesc SkyMoonTextureDesc = {};
	SkyMoonTextureDesc.mRoot = FSR_OtherFiles;
#if defined(_DURANGO)
  SkyMoonTextureDesc.pFilename = "Textures/Moon.dds";
#else
  SkyMoonTextureDesc.pFilename = "../../../../Ephemeris/Sky/resources/Textures/Moon.dds";
#endif
	SkyMoonTextureDesc.ppTexture = &pMoonTexture;
	addResource(&SkyMoonTextureDesc, false);
}

bool Sky::Init(Renderer* const renderer)
{
	pRenderer = renderer;

	RasterizerStateDesc rasterizerStateDesc = {};
	rasterizerStateDesc.mCullMode = CULL_MODE_NONE;
	addRasterizerState(pRenderer, &rasterizerStateDesc, &pRasterizerForSky);

	//////////////////////////////////// Samplers ///////////////////////////////////////////////////
	
	SamplerDesc samplerClampDesc = {
	FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
	ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE
	};
	addSampler(pRenderer, &samplerClampDesc, &pLinearClampSampler);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_DURANGO)
  eastl::string shaderPath("");
#elif defined(DIRECT3D12)
  eastl::string shaderPath("../../../../../Ephemeris/Sky/resources/Shaders/D3D12/");
#elif defined(VULKAN)
  eastl::string shaderPath("../../../../../Ephemeris/Sky/resources/Shaders/Vulkan/");
#elif defined(METAL)
  eastl::string shaderPath("../../../../../Ephemeris/Sky/resources/Shaders/Metal/");
#endif
  eastl::string shaderFullPath;

	ShaderLoadDesc skyShader = {};
	ShaderPath(shaderPath, (char*)"RenderSky.vert", shaderFullPath);
	skyShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"RenderSky.frag", shaderFullPath);
	skyShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };

	addShader(pRenderer, &skyShader, &pPAS_Shader);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc spaceShader = {};
	ShaderPath(shaderPath, (char*)"Space.vert", shaderFullPath);
	spaceShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Space.frag", shaderFullPath);
	spaceShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };

	addShader(pRenderer, &spaceShader, &pSpaceShader);
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc sunShader = {};
#if !defined(METAL)
	ShaderPath(shaderPath, (char*)"Sun.vert", shaderFullPath);
	sunShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Sun.geom", shaderFullPath);
	sunShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Sun.frag", shaderFullPath);
	sunShader.mStages[2] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
#else
	ShaderPath(shaderPath, (char*)"Sun.vert", shaderFullPath);
	sunShader.mStages[0] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"Sun.frag", shaderFullPath);
	sunShader.mStages[1] = { shaderFullPath, NULL, 0, FSR_SrcShaders };
#endif
	addShader(pRenderer, &sunShader, &pSunShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////
  Shader*           shaders[] = { pPAS_Shader, pSpaceShader, pSunShader };
  RootSignatureDesc rootDesc = {};
  rootDesc.mShaderCount = 3;
  rootDesc.ppShaders = shaders;

  addRootSignature(pRenderer, &rootDesc, &pSkyRootSignature);

  DescriptorBinderDesc SkyDescriptorBinderDesc[3] = { {pSkyRootSignature}, {pSkyRootSignature}, {pSkyRootSignature} };
  addDescriptorBinder(pRenderer, 0, 3, SkyDescriptorBinderDesc, &pSkyDescriptorBinder);
  

	BlendStateDesc blendStateSpaceDesc = {};
	blendStateSpaceDesc.mBlendModes[0] = BM_ADD;
	blendStateSpaceDesc.mBlendAlphaModes[0] = BM_ADD;

	blendStateSpaceDesc.mSrcFactors[0] = BC_SRC_ALPHA;
	blendStateSpaceDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;

	blendStateSpaceDesc.mSrcAlphaFactors[0] = BC_ONE;
	blendStateSpaceDesc.mDstAlphaFactors[0] = BC_ZERO;

	blendStateSpaceDesc.mMasks[0] = ALL;
	blendStateSpaceDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
	addBlendState(pRenderer, &blendStateSpaceDesc, &pBlendStateSpace);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	BlendStateDesc blendStateSunDesc = {};
	blendStateSunDesc.mBlendModes[0] = BM_ADD;
	blendStateSunDesc.mBlendAlphaModes[0] = BM_ADD;

	blendStateSunDesc.mSrcFactors[0] = BC_SRC_ALPHA;
	blendStateSunDesc.mDstFactors[0] = BC_ONE;

	blendStateSunDesc.mSrcAlphaFactors[0] = BC_ONE;
	blendStateSunDesc.mDstAlphaFactors[0] = BC_ZERO;

	blendStateSunDesc.mMasks[0] = ALL;
	blendStateSunDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
	addBlendState(pRenderer, &blendStateSpaceDesc, &pBlendStateSun);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	
	float screenQuadPoints[] = {
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f
	};

	BufferLoadDesc TriangularVbDesc = {};
	TriangularVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	TriangularVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	TriangularVbDesc.mDesc.mVertexStride = sizeof(float) * 5;
	TriangularVbDesc.mDesc.mSize = (uint64_t)(TriangularVbDesc.mDesc.mVertexStride * 4);
	TriangularVbDesc.pData = screenQuadPoints;
	TriangularVbDesc.ppBuffer = &pGlobalTriangularVertexBuffer;
	addResource(&TriangularVbDesc);	

	CalculateLookupData();

	// Generate sphere vertex buffer
	float* pSpherePoints;
	generateSpherePoints(&pSpherePoints, &gNumberOfSpherePoints, gSphereResolution, gSphereDiameter);

	uint64_t       sphereDataSize = gNumberOfSpherePoints * sizeof(float);
	BufferLoadDesc sphereVbDesc = {};
	sphereVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	sphereVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	sphereVbDesc.mDesc.mSize = sphereDataSize;
	sphereVbDesc.mDesc.mVertexStride = sizeof(float) * 6;
	sphereVbDesc.pData = pSpherePoints;
	sphereVbDesc.ppBuffer = &pSphereVertexBuffer;
	addResource(&sphereVbDesc);

	BufferLoadDesc renderSkybDesc = {};
	renderSkybDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	renderSkybDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	renderSkybDesc.mDesc.mSize = sizeof(RenderSkyUniformBuffer);
	renderSkybDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	renderSkybDesc.pData = NULL;
	
	for(uint i=0; i<gImageCount; i++)
	{
	  renderSkybDesc.ppBuffer = &pRenderSkyUniformBuffer[i];
	  addResource(&renderSkybDesc);
	}
	
	BufferLoadDesc spaceUniformDesc = {};
	spaceUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	spaceUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	spaceUniformDesc.mDesc.mSize = sizeof(SpaceUniformBuffer);
	spaceUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	spaceUniformDesc.pData = NULL;
	
	for(uint i=0; i<gImageCount; i++)
	{
		spaceUniformDesc.ppBuffer = &pSpaceUniformBuffer[i];
		addResource(&spaceUniformDesc);
	}

  BufferLoadDesc sunUniformDesc = {};
  sunUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sunUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
  sunUniformDesc.mDesc.mSize = sizeof(SunUniformBuffer);
  sunUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
  sunUniformDesc.pData = NULL;

  for (uint i = 0; i < gImageCount; i++)
  {
    sunUniformDesc.ppBuffer = &pSunUniformBuffer[i];
    addResource(&sunUniformDesc);
  }

  
	
	// Need to free memory;
	conf_free(pSpherePoints);

	///////////////////////////////////////////////////////////////////
	// UI
	///////////////////////////////////////////////////////////////////

	GuiDesc guiDesc = {};
	guiDesc.mStartPosition = vec2(1280.0f / getDpiScale().getX(), 700.0f / getDpiScale().getY());
	guiDesc.mStartSize = vec2(300.0f / getDpiScale().getX(), 250.0f / getDpiScale().getY());
	pGuiWindow = pGAppUI->AddGuiComponent("Sky", &guiDesc);

	gSkySettings.SkyInfo.x = 0.12f;
	gSkySettings.SkyInfo.y = 3.0f;
	gSkySettings.SkyInfo.z = 1.0f;
	gSkySettings.SkyInfo.w = 0.322f;

	gSkySettings.OriginLocation.x = -167.0f;
	gSkySettings.OriginLocation.y = -532.0f;
	gSkySettings.OriginLocation.z =  788.0f;
	gSkySettings.OriginLocation.w = 1.0f;

	SliderFloatWidget Exposure("Exposure", &gSkySettings.SkyInfo.x, float(0.0f), float(1.0f), float(0.001f));
	pGuiWindow->AddWidget(Exposure);
	SliderFloatWidget InscatterIntensity("InscatterIntensity", &gSkySettings.SkyInfo.y, float(0.0f), float(3.0f), float(0.001f));
	pGuiWindow->AddWidget(InscatterIntensity);
	SliderFloatWidget InscatterContrast("InscatterContrast", &gSkySettings.SkyInfo.z, float(0.0f), float(2.0f), float(0.001f));
	pGuiWindow->AddWidget(InscatterContrast);
	SliderFloatWidget UnitsToM("UnitsToM", &gSkySettings.SkyInfo.w, float(0.0f), float(2.0f), float(0.001f));
	pGuiWindow->AddWidget(UnitsToM);


	return true;
}

void Sky::Exit()
{
	removeBlendState(pBlendStateSpace);
	removeBlendState(pBlendStateSun);

	removeShader(pRenderer, pPAS_Shader);
	removeShader(pRenderer, pSpaceShader);
	removeShader(pRenderer, pSunShader);

  removeRootSignature(pRenderer, pSkyRootSignature);
  removeDescriptorBinder(pRenderer, pSkyDescriptorBinder);

	removeSampler(pRenderer, pLinearClampSampler);
	removeResource(pSphereVertexBuffer);
	
	for(uint i=0; i<gImageCount; i++)
	{
		removeResource(pRenderSkyUniformBuffer[i]);
		removeResource(pSpaceUniformBuffer[i]);
    removeResource(pSunUniformBuffer[i]);
	}

	removeResource(pTransmittanceTexture);
	removeResource(pIrradianceTexture);
	removeResource(pInscatterTexture);
	removeResource(pMoonTexture);

	removeResource(pGlobalTriangularVertexBuffer);

	removeRasterizerState(pRasterizerForSky);
}

bool Sky::Load(RenderTarget** rts)
{
	pPreStageRenderTarget = rts[0];

	mWidth = pPreStageRenderTarget->mDesc.mWidth;
	mHeight = pPreStageRenderTarget->mDesc.mHeight;

	float aspect = (float)mWidth / (float)mHeight;
	float aspectInverse = 1.0f / aspect;
	float horizontal_fov = PI / 3.0f;
	//float vertical_fov = 2.0f * atan(tan(horizontal_fov*0.5f) * aspectInverse);

	SkyProjectionMatrix = mat4::perspective(horizontal_fov, aspectInverse, SKY_NEAR, SKY_FAR);

	//layout and pipeline for ScreenQuad
	VertexLayout vertexLayout = {};
	vertexLayout.mAttribCount = 2;
	vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
	vertexLayout.mAttribs[0].mFormat = ImageFormat::RGB32F;
	vertexLayout.mAttribs[0].mBinding = 0;
	vertexLayout.mAttribs[0].mLocation = 0;
	vertexLayout.mAttribs[0].mOffset = 0;

	vertexLayout.mAttribs[1].mSemantic = SEMANTIC_TEXCOORD0;
	vertexLayout.mAttribs[1].mFormat = ImageFormat::RG32F;
	vertexLayout.mAttribs[1].mBinding = 0;
	vertexLayout.mAttribs[1].mLocation = 1;
	vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);

	RenderTargetDesc SkyRenderTarget = {};
	SkyRenderTarget.mArraySize = 1;
	SkyRenderTarget.mDepth = 1;
	SkyRenderTarget.mFormat = ImageFormat::RG11B10F;
	SkyRenderTarget.mSampleCount = SAMPLE_COUNT_1;
	SkyRenderTarget.mSampleQuality = 0;
	SkyRenderTarget.mSrgb = false;
	SkyRenderTarget.mWidth = pPreStageRenderTarget->mDesc.mWidth;
	SkyRenderTarget.mHeight = pPreStageRenderTarget->mDesc.mHeight;
	SkyRenderTarget.pDebugName = L"Sky RenderTarget";
	SkyRenderTarget.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
	addRenderTarget(pRenderer, &SkyRenderTarget, &pSkyRenderTarget);

#if defined(VULKAN)
  TransitionRenderTargets(pSkyRenderTarget, ResourceState::RESOURCE_STATE_RENDER_TARGET, pRenderer, ppTransCmds[0], pGraphicsQueue, pTransitionCompleteFences);
#endif

  PipelineDesc pipelineDescPAS;
  {
    pipelineDescPAS.mType = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipelineDesc &pipelineSettings = pipelineDescPAS.mGraphicsDesc;

	  pipelineSettings = { 0 };

	  pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
	  pipelineSettings.mRenderTargetCount = 1;

	  pipelineSettings.pColorFormats = &pSkyRenderTarget->mDesc.mFormat;
	  pipelineSettings.pSrgbValues = &pSkyRenderTarget->mDesc.mSrgb;
	  pipelineSettings.mSampleCount = pSkyRenderTarget->mDesc.mSampleCount;
	  pipelineSettings.mSampleQuality = pSkyRenderTarget->mDesc.mSampleQuality;

	  pipelineSettings.pRootSignature = pSkyRootSignature;
	  pipelineSettings.pShaderProgram = pPAS_Shader;
	  pipelineSettings.pVertexLayout = &vertexLayout;
	  pipelineSettings.pRasterizerState = pRasterizerForSky;

	  addPipeline(pRenderer, &pipelineDescPAS, &pPAS_Pipeline);
  }

	vertexLayout = {};
	vertexLayout.mAttribCount = 2;
	vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
	vertexLayout.mAttribs[0].mFormat = ImageFormat::RGB32F;
	vertexLayout.mAttribs[0].mBinding = 0;
	vertexLayout.mAttribs[0].mLocation = 0;
	vertexLayout.mAttribs[0].mOffset = 0;
	vertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
	vertexLayout.mAttribs[1].mFormat = ImageFormat::RGB32F;
	vertexLayout.mAttribs[1].mBinding = 0;
	vertexLayout.mAttribs[1].mLocation = 1;
	vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);

  PipelineDesc pipelineDescSpace;
  {
    pipelineDescSpace.mType = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipelineDesc &pipelineSettings = pipelineDescSpace.mGraphicsDesc;

	  pipelineSettings = { 0 };

	  pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
	  pipelineSettings.mRenderTargetCount = 1;

	  pipelineSettings.pColorFormats = &pSkyRenderTarget->mDesc.mFormat;
	  pipelineSettings.pSrgbValues = &pSkyRenderTarget->mDesc.mSrgb;
	  pipelineSettings.mSampleCount = pSkyRenderTarget->mDesc.mSampleCount;
	  pipelineSettings.mSampleQuality = pSkyRenderTarget->mDesc.mSampleQuality;

	  pipelineSettings.pRootSignature = pSkyRootSignature;
	  pipelineSettings.pShaderProgram = pSpaceShader;
	  pipelineSettings.pVertexLayout = &vertexLayout;
	  pipelineSettings.pRasterizerState = pRasterizerForSky;
	  pipelineSettings.pBlendState = pBlendStateSpace;

	  addPipeline(pRenderer, &pipelineDescSpace, &pSpacePipeline);

  }

  PipelineDesc pipelineDesSun;
  {
	  pipelineDesSun.mType = PIPELINE_TYPE_GRAPHICS;
	  GraphicsPipelineDesc &pipelineSettings = pipelineDesSun.mGraphicsDesc;

	  pipelineSettings = { 0 };

  #if !defined(METAL)
	  pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_POINT_LIST;
  #else
	  pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_STRIP;
  #endif
	  pipelineSettings.mRenderTargetCount = 1;

	  pipelineSettings.pColorFormats = &pSkyRenderTarget->mDesc.mFormat;
	  pipelineSettings.pSrgbValues = &pSkyRenderTarget->mDesc.mSrgb;
	  pipelineSettings.mSampleCount = pSkyRenderTarget->mDesc.mSampleCount;
	  pipelineSettings.mSampleQuality = pSkyRenderTarget->mDesc.mSampleQuality;

	  pipelineSettings.pRootSignature = pSkyRootSignature;
	  pipelineSettings.pShaderProgram = pSunShader;
	  pipelineSettings.pVertexLayout = &vertexLayout;
	  pipelineSettings.pRasterizerState = pRasterizerForSky;
	  pipelineSettings.pBlendState = pBlendStateSun;

	  addPipeline(pRenderer, &pipelineDesSun, &pSunPipeline);
  }

	return false;
}

void Sky::Unload()
{
	removeRenderTarget(pRenderer, pSkyRenderTarget);
	removePipeline(pRenderer, pPAS_Pipeline);
	removePipeline(pRenderer, pSpacePipeline);
	removePipeline(pRenderer, pSunPipeline);
}

void Sky::Update(float deltaTime)
{
	g_ElapsedTime += deltaTime;
}


void Sky::Draw(Cmd* cmd)
{
	cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Sky", true);

	vec4 lightDir = vec4(f3Tov3(LightDirection));
	
	{		
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Atmospheric Scattering", true);

		TextureBarrier barriersSky[] = {
				{ pSkyRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pPreStageRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pTransmittanceTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pIrradianceTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pInscatterTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

		cmdResourceBarrier(cmd, 0, NULL, 5, barriersSky, false);
		
		LoadActionsDesc loadActions = {};
		loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
		//loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
		loadActions.mClearColorValues[0].r = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].b = 0.0f;
		loadActions.mClearColorValues[0].a = 0.0f;
		//loadActions.mClearDepth = pDepthBuffer->mDesc.mClearValue;

		cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mDesc.mWidth, pSkyRenderTarget->mDesc.mHeight);

		cmdBindPipeline(cmd, pPAS_Pipeline);
		RenderSkyUniformBuffer _cbRootConstantStruct;

		_cbRootConstantStruct.InvViewMat = inverse(pCameraController->getViewMatrix());
		_cbRootConstantStruct.InvProjMat = inverse(SkyProjectionMatrix);
		_cbRootConstantStruct.LightDirection = float4(lightDir.getX(), lightDir.getY(), lightDir.getZ(), gSkySettings.SkyInfo.x);

		///////////////////////////////////////////////////////////////////////////////////////////////

		//	Update params
		float fUnitsToKM = gSkySettings.SkyInfo.w * 0.001f;

		float4	offsetScaleToLocalKM = -gSkySettings.OriginLocation * fUnitsToKM;
		offsetScaleToLocalKM.setW(fUnitsToKM);

		offsetScaleToLocalKM.y += Rg + 0.001f;

		float2	inscatterParams = float2(gSkySettings.SkyInfo.y * (1.0f - gSkySettings.SkyInfo.z), gSkySettings.SkyInfo.y * gSkySettings.SkyInfo.z);
			float3	localCamPosKM = (v3ToF3(pCameraController->getViewPosition()) * offsetScaleToLocalKM.w) + offsetScaleToLocalKM.getXYZ();

		float	Q = (float)(SKY_FAR / (SKY_FAR - SKY_NEAR));
		float4	QNNear = float4(Q, SKY_NEAR * fUnitsToKM, SKY_NEAR, SKY_FAR);

		//////////////////////////////////////////////////////////////////////////////////////////////

		_cbRootConstantStruct.CameraPosition = float4(localCamPosKM.getX(), localCamPosKM.getY(), localCamPosKM.getZ(), 1.0f);
		_cbRootConstantStruct.QNNear = QNNear;
		_cbRootConstantStruct.InScatterParams = float4(inscatterParams.x, inscatterParams.y, 0.0f, 0.0f);
		_cbRootConstantStruct.LightIntensity = LightColorAndIntensity;

		BufferUpdateDesc BufferUniformSettingDesc = { pRenderSkyUniformBuffer[gFrameIndex], &_cbRootConstantStruct };
		updateResource(&BufferUniformSettingDesc);

		DescriptorData ScParams[7] = {};

		ScParams[0].pName = "RenderSkyUniformBuffer";
		ScParams[0].ppBuffers = &pRenderSkyUniformBuffer[gFrameIndex];

		ScParams[1].pName = "g_LinearClamp";
		ScParams[1].ppSamplers = &pLinearClampSampler;

		ScParams[2].pName = "SceneColorTexture";
		ScParams[2].ppTextures = &pPreStageRenderTarget->pTexture;

		ScParams[3].pName = "Depth";
		ScParams[3].ppTextures = &pDepthBuffer->pTexture;

		ScParams[4].pName = "TransmittanceTexture";
		ScParams[4].ppTextures = &pTransmittanceTexture;

		ScParams[5].pName = "InscatterTexture";
		ScParams[5].ppTextures = &pInscatterTexture;

		ScParams[6].pName = "TransmittanceColor";
		ScParams[6].ppBuffers = &pTransmittanceBuffer;

		cmdBindDescriptors(cmd, pSkyDescriptorBinder, pSkyRootSignature, 7, ScParams);
		cmdBindVertexBuffer(cmd, 1, &pGlobalTriangularVertexBuffer, NULL);
		cmdDraw(cmd, 3, 0);
	
		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);
		
		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}

	/////////////////////////////////////////////////////////////////////////////////////

	if (lightDir.getY() < 0.2f)
	{			
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw Space", true);

		TextureBarrier barriersSky[] =
		{
			{ pSkyRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriersSky, false);
		
		cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mDesc.mWidth, pSkyRenderTarget->mDesc.mHeight);
		
		struct Data
		{
			mat4 viewProjMat;
			float4	LightDirection;
			float4 ScreenSize;
		} data;

		data.viewProjMat = SkyProjectionMatrix * pCameraController->getViewMatrix();
		data.LightDirection = float4(LightDirection, 0.0f);
		data.ScreenSize = float4((float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 0.0f);

		BufferUpdateDesc BufferUniformSettingDesc = { pSpaceUniformBuffer[gFrameIndex], &data };
		updateResource(&BufferUniformSettingDesc);
		
		DescriptorData SpaceParams[2] = {};

		cmdBindPipeline(cmd, pSpacePipeline);

		SpaceParams[0].pName = "SpaceUniform";
		SpaceParams[0].ppBuffers = &pSpaceUniformBuffer[gFrameIndex];
		SpaceParams[1].pName = "depthTexture";
		SpaceParams[1].ppTextures = &pDepthBuffer->pTexture;

		cmdBindDescriptors(cmd, pSkyDescriptorBinder, pSkyRootSignature, 2, SpaceParams);

		cmdBindVertexBuffer(cmd, 1, &pSphereVertexBuffer, NULL);
		cmdDrawInstanced(cmd, gNumberOfSpherePoints / 6, 0, 1, 0);
		
		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);
		
		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}
		
	////////////////////////////////////////////////////////////////////////
#if !defined(METAL)
	{
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw Sun and Moon", true);

		TextureBarrier barriersSky[] =
		{
			{ pSkyRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriersSky, false);
		
		cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mDesc.mWidth, pSkyRenderTarget->mDesc.mHeight);
		
		struct Data
		{
			mat4 ViewMat;
			mat4 ViewProjMat;
			float4 LightDirection;
			float4 Dx;
			float4 Dy;
		} data;

		data.ViewMat = pCameraController->getViewMatrix();
		data.ViewProjMat = SkyProjectionMatrix * pCameraController->getViewMatrix();
		data.LightDirection = float4(LightDirection * 1000000.0f, 0.0f);
		data.Dx = v4ToF4(pCameraController->getViewMatrix().getRow(0) * 20000.0f);
		data.Dy = v4ToF4(pCameraController->getViewMatrix().getRow(1) * 20000.0f);

    BufferUpdateDesc BufferUniformSettingDesc = { pSunUniformBuffer[gFrameIndex], &data };
    updateResource(&BufferUniformSettingDesc);
    

		DescriptorData SunParams[4] = {};

		cmdBindPipeline(cmd, pSunPipeline);

		SunParams[0].pName = "SunUniform";
  	SunParams[0].ppBuffers = &pSunUniformBuffer[gFrameIndex];
		SunParams[1].pName = "depthTexture";
		SunParams[1].ppTextures = &pDepthBuffer->pTexture;
		SunParams[2].pName = "moonAtlas";
		SunParams[2].ppTextures = &pMoonTexture;
		SunParams[3].pName = "g_LinearClampSampler";
		SunParams[3].ppSamplers = &pLinearClampSampler;

		cmdBindDescriptors(cmd, pSkyDescriptorBinder, pSkyRootSignature, 4, SunParams);

		cmdDraw(cmd, 1, 0);
		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);
		
		TextureBarrier barriersSkyEnd[] = {
					{ pSkyRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriersSkyEnd, false);
			
		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}
#else
	{		
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw Sun and Moon", true);
		
		TextureBarrier barriersSky[] =
		{
			{ pSkyRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
		};
		
		cmdResourceBarrier(cmd, 0, NULL, 1, barriersSky, false);
		
		cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mDesc.mWidth, (float)pSkyRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mDesc.mWidth, pSkyRenderTarget->mDesc.mHeight);
		
		struct Data
		{
			mat4 ViewMat;
			mat4 ViewProjMat;
			float4 LightDirection;
			float4 Dx;
			float4 Dy;
		} data;
		
		
		data.ViewMat = pCameraController->getViewMatrix();
		data.ViewProjMat = SkyProjectionMatrix * pCameraController->getViewMatrix();
		data.LightDirection = float4(LightDirection * 1000000.0f, 0.0f);
		data.Dx = v4ToF4(pCameraController->getViewMatrix().getRow(0) * 20000.0f);
		data.Dy = v4ToF4(pCameraController->getViewMatrix().getRow(1) * 20000.0f);

    BufferUpdateDesc BufferUniformSettingDesc = { pSunUniformBuffer[gFrameIndex], &data };
    updateResource(&BufferUniformSettingDesc);
		
		DescriptorData SunParams[4] = {};
		
		cmdBindPipeline(cmd, pSunPipeline);
		SunParams[0].pName = "SunUniform";
		SunParams[0].ppBuffers = &pSunUniformBuffer[gFrameIndex];
		SunParams[1].pName = "depthTexture";
		SunParams[1].ppTextures = &pDepthBuffer->pTexture;
		SunParams[2].pName = "moonAtlas";
		SunParams[2].ppTextures = &pMoonTexture;
		SunParams[3].pName = "g_LinearClampSampler";
		SunParams[3].ppSamplers = &pLinearClampSampler;
		
		cmdBindDescriptors(cmd, pSkyDescriptorBinder, pSkyRootSignature, 4, SunParams);
		
		cmdBindVertexBuffer(cmd, 1, &pGlobalTriangularVertexBuffer, NULL);
		
		cmdDraw(cmd, 4, 0);
		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);
		
		TextureBarrier barriersSkyEnd[] = {
			{ pSkyRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
		};
		
		cmdResourceBarrier(cmd, 0, NULL, 1, barriersSkyEnd, false);

		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}
#endif

	
	
	cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
}

void Sky::Initialize(uint InImageCount,
	ICameraController* InCameraController, Queue*	InGraphicsQueue,
	Cmd** InTransCmds, Fence* InTransitionCompleteFences, GpuProfiler*	InGraphicsGpuProfiler, UIApp* InGAppUI, Buffer*	InTransmittanceBuffer)
{
	gImageCount = InImageCount;

	pCameraController = InCameraController;
	pGraphicsQueue = InGraphicsQueue;
	ppTransCmds = InTransCmds;
	pTransitionCompleteFences = InTransitionCompleteFences;
	pGraphicsGpuProfiler = InGraphicsGpuProfiler;
	pGAppUI = InGAppUI;
	pTransmittanceBuffer = InTransmittanceBuffer;
}

void Sky::InitializeWithLoad(RenderTarget* InDepthRenderTarget, RenderTarget* InLinearDepthRenderTarget)
{
	pDepthBuffer = InDepthRenderTarget;
	pLinearDepthBuffer = InLinearDepthRenderTarget;
}

static const float AVERAGE_GROUND_REFLECTANCE = 0.1f;

// Rayleigh
static const float HR = 8.0f;
static const vec3 betaR = vec3(5.8e-3f, 1.35e-2f, 3.31e-2f);
/*
// Mie
// DEFAULT
static const float HM = 1.2;
static const float3 betaMSca = (4e-3).xxx;
static const float3 betaMEx = betaMSca / 0.9;
static const float mieG = 0.8;
*/

// CLEAR SKY
static const float HM = 1.2f;
static const vec3 betaMSca = vec3(20e-3f, 20e-3f, 20e-3f);
static const vec3 betaMEx = betaMSca / 0.9f;
static const float mieG = 0.76f;

// nearest intersection of ray r,mu with ground or top atmosphere boundary
// mu=cos(ray zenith angle at ray origin)
float limit(float r, float mu) {
	float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0f) + RL * RL);
	// 	float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg;
	// 	if (delta2 >= 0.0) {
	// 		float din = -r * mu - sqrt(delta2);
	// 		if (din >= 0.0) {
	// 			dout = min(dout, din);
	// 		}
	// 	}
	return dout;
}

// optical depth for ray (r,mu) of length d, using analytic formula
// (mu=cos(view zenith angle)), intersections with ground ignored
// H=height scale of exponential density function
float opticalDepth(float H, float r, float mu, float d) {

	float a = sqrt((0.5f / H)*r);
	vec2 a01 = a * vec2(mu, mu + d / r);
	vec2 a01s = vec2(sign(a01.getX()), sign(a01.getY()));
	vec2 a01sq = vec2(a01.getX() * a01.getX(), a01.getY() * a01.getY());
	float x = a01s.getY() > a01s.getX() ? exp(a01sq.getX()) : 0.0f;
	vec2 y = a01s;		 
	vec2 denom = (2.3193f*vec2(fabs(a01.getX()), fabs(a01.getY())) + vec2(sqrt(1.52f*a01sq.getX() + 4.0f), sqrt(1.52f*a01sq.getY() + 4.0f)));

	y = vec2(y.getX() / denom.getX(), y.getY() / denom.getY());

	vec2 denom2 = vec2(1.0f, exp(-d / H * (d / (2.0f*r) + mu)));

	y = vec2(y.getX() * denom2.getX(), y.getY() * denom2.getY());

	//denom = vec2(denom.getX() * denom2.getX(), denom.getY() * denom2.getY());

	

	return sqrt((6.2831f*H)*r) * exp((Rg - r) / H) * (x + dot(y, vec2(1.0f, -1.0f)));
}

// transmittance(=transparency) of atmosphere for ray (r,mu) of length d
// (mu=cos(view zenith angle)), intersections with ground ignored
// uses analytic formula instead of transmittance texture
vec3 analyticTransmittance(float r, float mu, float d) {
	vec3 arg = -betaR * opticalDepth(HR, r, mu, d) - betaMEx * opticalDepth(HM, r, mu, d);
	return vec3(exp(arg.getX()), exp(arg.getY()), exp(arg.getZ()));
}

vec3 analyticTransmittance(float r, float mu) {
	return analyticTransmittance(r, mu, limit(r, mu));
}


float SmoothStep(float edge0, float edge1, float x)
{
	// Scale, bias and saturate x to 0..1 range
	x = clamp((x - edge0) / (edge1 - edge0), 0, 1);
	// Evaluate polynomial
	return x * x*(3 - 2 * x);
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), or zero if ray intersects ground
vec3 transmittanceWithShadowSmooth(float r, float mu) {
	//	TODO: check if it is reasonably fast
	//	TODO: check if it is mathematically correct
	//    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? (0.0).xxx : transmittance(r, mu);

	float eps = 0.5f*PI / 180.0f;
	float eps1 = 0.1f*PI / 180.0f;
	float horizMu = -sqrt(1.0f - (Rg / r) * (Rg / r));
	float horizAlpha = acos(horizMu);
	float horizMuMin = cos(horizAlpha + eps);
	//float horizMuMax = cos(horizAlpha-eps);
	float horizMuMax = cos(horizAlpha + eps1);

	float t = SmoothStep(horizMuMin, horizMuMax, mu);
	vec3 analy = analyticTransmittance(r, mu);
	return lerp(t, vec3(0.0), analy);
}

float3 Sky::GetSunColor()
{
	//float4		SkyInfo; // x: fExposure, y: fInscatterIntencity, z: fInscatterContrast, w: fUnitsToM
	//float4		OriginLocation;

	const float	fUnitsToKM = gSkySettings.SkyInfo.w*0.001f;
	float4	offsetScaleToLocalKM = float4(-gSkySettings.OriginLocation.getXYZ()*fUnitsToKM, fUnitsToKM);
	offsetScaleToLocalKM.y += Rg + 0.001f;

	//float2	inscatterParams = float2(gSkySettings.SkyInfo.y*(1 - gSkySettings.SkyInfo.z), gSkySettings.SkyInfo.y*gSkySettings.SkyInfo.z);
	float3 localCamPosKM = (float3(0.0f, 0.0f, 0.0f)*offsetScaleToLocalKM.w) + offsetScaleToLocalKM.getXYZ();


	//	TODO: Igor: Simplify

	vec3 ray = f3Tov3(LightDirection);
	vec3 x = f3Tov3(localCamPosKM);
	vec3 v = normalize(ray);

	float r = length(x);
	float mu = dot(x, v) / r;

	return v3ToF3(transmittanceWithShadowSmooth(r, mu));
}
