//--------------------------------------------------------------------------------------------
//
// Coyright (C) 2009 - 2013 Confetti Special Effects Inc.
// All rights reserved.
//
// This source may not be distributed and/or modified without expressly written permission
// from Confetti Special Effects Inc.
//
//--------------------------------------------------------------------------------------------

#include "CloudsManager.h"

#include "../../../../The-Forge/Common_3/Renderer/IResourceLoader.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"

//#include "Singletons.h"
//#include "../Include/Renderer.h"
//#include "LogImpl.h"
//#include "../Include/FileSystem.h"

//#include <string>

//	TODO: Igor: remove it with the rand function
//#include "stdlib.h"

//	This is used for the single memcpy only
//#include <string.h>
//#include <stdio.h>

struct CloudDescriptor
{
	union
	{
		struct  
		{
			uint32	bCumulusCloud	: 1;
			uint32	uiCloudID		: 16;
		};
		CloudHandle handle;
	};
};

#define MaxParticles 100
#define QMaxParticles ((MaxParticles+3)/ 4)

struct CumulusCloudUniformBuffer
{
  mat4 model;
  vec4 OffsetScale[MaxParticles];
  vec4 ParticleProps[QMaxParticles];

  mat4 vp;
  mat4 v;
  vec3 dx, dy;
  float zNear;
  vec2 packDepthParams;
  float masterParticleRotation;
};


struct DistantCloudUniformBuffer
{
  mat4	mvp;
  mat4	model;
  vec4	offsetScale;
  vec4  camPos;
  vec4	localSun;
  vec4	s;

  float StepSize;
  float Attenuation;
  float AlphaSaturation;
  float padding00;
};

struct ImposterUniformBuffer
{
  mat4	mvp;
  mat4	model;
  vec4	offsetScale;

  vec3	localSun;

  float StepSize;

  float Attenuation;
  float AlphaSaturation;

  vec2 UnpackDepthParams;

  float CloudOpacity;
  float SaturationFactor;
  float ContrastFactor;
  float padding00;
};


float RandomValue()
{
	return ((float)rand() / RAND_MAX);
}


float RandomValueNormalized()
{
	return ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
}

float RandomValue( float2 a_Range )
{
	return a_Range.x + ( RandomValue() * ( a_Range.y - a_Range.x ) );
}

static void ShaderPath(const eastl::string &shaderPath, char* pShaderName, eastl::string &result)
{
  result.resize(0);
  eastl::string shaderName(pShaderName);
  result = shaderPath + shaderName;
}

Shader* pClDistanceCloudShader;
RootSignature* pClDistanceCloudRootSignature;
// #TODO
//DescriptorBinder* pClDistanceCloudDescriptorBinder = NULL;
//Pipeline* pClDistanceCloudPipeline = NULL;

Shader* pClCumulusCloudShader;
RootSignature*pClCumulusCloudRootSignature;
// #TODO
//DescriptorBinder* pClCumulusCloudDescriptorBinder = NULL;
//Pipeline* pClCumulusCloudPipeline = NULL;

Shader* pImposterCloudShader;
RootSignature*pImposterCloudRootSignature;
// #TODO
//DescriptorBinder* pImposterCloudDescriptorBinder = NULL;

CloudsManager::CloudsManager(void) :
	//m_pRenderer(0), m_pDevice(0),
	linearClamp(NULL), trilinearClamp(NULL),
	m_shDistantCloud(NULL),
	m_shCumulusCloud(NULL),
	m_tDistantCloud(NULL),
	m_tCumulusCloud(NULL)
{
	m_Params.fStepSize = 0.004f;
	m_Params.fAttenuation = 0.6f;
	m_Params.fAlphaSaturation = 2.0f;
	m_Params.fCumulusAlphaSaturation = 1.14f;
	//m_Params.bUpdateEveryFrame = true;
	m_Params.bUpdateEveryFrame = false;
	//m_Params.bDepthTestClouds = false;
	m_Params.bDepthTestClouds = true;
	m_Params.bMoveClouds = false;
	m_Params.fCloudSpeed = 10.f;
	//m_Params.fCloudSpeed = 1000.f;
	m_Params.fCumulusExistanceR = 20000.0;
	m_Params.fCumulusFadeStart = 19000.0;

	m_Params.fCumulusSunIntensity = 1.0f;
	m_Params.fCumulusAmbientIntensity = 1.0f;
	m_Params.fStratusSunIntensity = 1.0f;
	m_Params.fStratusAmbientIntensity = 1.0f;

	m_Params.fCloudsSaturation = 1.0f;
	m_Params.fCloudsContrast = 0.0f;
}

CloudsManager::~CloudsManager(void)
{
}

bool CloudsManager::load( int width, int height, const char* pszShaderDefines )
{
	// States
	//if ((noCull = pRenderer->addRasterizerState(CULL_NONE)) == RS_NONE) return false;

  RasterizerStateDesc rasterizerStateDesc = {};
  rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

	//if ((noDepthWrite  = pRenderer->addDepthState(true, false)) == DS_NONE) return false;

  DepthStateDesc depthStateNoWriteDesc = {};
  depthStateNoWriteDesc.mDepthTest = true;
  depthStateNoWriteDesc.mDepthWrite = false;
  depthStateNoWriteDesc.mDepthFunc = CMP_LEQUAL;

  DepthStateDesc depthStateDisableDesc = {};
  depthStateDisableDesc.mDepthTest = false;
  depthStateDisableDesc.mDepthWrite = false;
  depthStateDisableDesc.mDepthFunc = CMP_LEQUAL;

	//if ((linearClamp = pRenderer->addSamplerState(BILINEAR, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;

  SamplerDesc samplerClampDesc = {
  FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_NEAREST,
  ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE
  };
  addSampler(pRenderer, &samplerClampDesc, &linearClamp);

	//if ((trilinearClamp = pRenderer->addSamplerState(TRILINEAR, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;

  samplerClampDesc = {
    FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
    ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE
  };
  addSampler(pRenderer, &samplerClampDesc, &trilinearClamp);

	//if ((alphaBlend = pRenderer->addBlendState(SRC_ALPHA, ONE_MINUS_SRC_ALPHA, SRC_ALPHA, ONE_MINUS_SRC_ALPHA)) == BS_NONE) return false;

  BlendStateDesc blendStateAlphaDesc = {};
  blendStateAlphaDesc.mBlendModes[0] = BM_ADD;
  blendStateAlphaDesc.mBlendAlphaModes[0] = BM_ADD;

  blendStateAlphaDesc.mSrcFactors[0] = BC_SRC_ALPHA;
  blendStateAlphaDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;

  blendStateAlphaDesc.mSrcAlphaFactors[0] = BC_SRC_ALPHA;
  blendStateAlphaDesc.mDstAlphaFactors[0] = BC_ONE_MINUS_SRC_ALPHA;

  blendStateAlphaDesc.mMasks[0] = ALL;
  blendStateAlphaDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;

	//if ((impostorBlend = pRenderer->addBlendState(ONE, ZERO, ONE, ONE_MINUS_SRC_ALPHA)) == BS_NONE) return false;
	//if ((impostorBlend = pRenderer->addBlendState(SRC_ALPHA, ONE_MINUS_SRC_ALPHA, ONE, ONE_MINUS_SRC_ALPHA)) == BS_NONE) return false;
	//	TODO: B: Use this? Igor: this makes clouds look as without reconstructed position.
#ifdef	USE_MULTIPLICATIVE_DENSITY_ACCUMULTAION
	//if ((impostorBlend = pRenderer->addBlendState(SRC_ALPHA, ONE_MINUS_SRC_ALPHA, ZERO, ONE_MINUS_SRC_ALPHA)) == BS_NONE) return false;
  BlendStateDesc blendStateImposterDesc = {};
  blendStateImposterDesc.mBlendModes[0] = BM_ADD;
  blendStateImposterDesc.mBlendAlphaModes[0] = BM_ADD;

  blendStateImposterDesc.mSrcFactors[0] = BC_SRC_ALPHA;
  blendStateImposterDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;

  blendStateImposterDesc.mSrcAlphaFactors[0] = BC_ZERO;
  blendStateImposterDesc.mDstAlphaFactors[0] = BC_ONE_MINUS_SRC_ALPHA;

  blendStateImposterDesc.mMasks[0] = ALL;
  blendStateImposterDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;


#else	//	USE_MULTIPLICATIVE_DENSITY_ACCUMULTAION
	//if ((impostorBlend = pRenderer->addBlendState(SRC_ALPHA, ONE_MINUS_SRC_ALPHA, SRC_ALPHA, ONE_MINUS_SRC_ALPHA)) == BS_NONE) return false;
	
  //if ((impostorBlend = pRenderer->addBlendState(SRC_ALPHA, ONE_MINUS_SRC_ALPHA, ONE, ONE_MINUS_SRC_ALPHA)) == BS_NONE) return false;

  BlendStateDesc blendStateImposterDesc = {};
  blendStateImposterDesc.mBlendModes[0] = BM_ADD;
  blendStateImposterDesc.mBlendAlphaModes[0] = BM_ADD;

  blendStateImposterDesc.mSrcFactors[0] = BC_SRC_ALPHA;
  blendStateImposterDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;

  blendStateImposterDesc.mSrcAlphaFactors[0] = BC_ONE;
  blendStateImposterDesc.mDstAlphaFactors[0] = BC_ONE_MINUS_SRC_ALPHA;

  blendStateImposterDesc.mMasks[0] = ALL;
  blendStateImposterDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;


#endif	//	USE_MULTIPLICATIVE_DENSITY_ACCUMULTAION
	//if ((oneAlphaBlend = pRenderer->addBlendState(ONE, ONE_MINUS_SRC_ALPHA)) == BS_NONE) return false;
	//if ((addBlend = pRenderer->addBlendState(ONE, ONE, ONE, ONE)) == BS_NONE) return false;
	//if ((alphaBlend = m_pRenderer->addBlendState(ONE, ONE)) == BS_NONE) return false;

	//std::string ShaderIncludes = pszShaderDefines;

	//	Igor: just a workaround to handle shader includes
#ifdef	USE_NATIVE_INCLUDES
	char *szExtra = "#define USE_NATIVE_INCLUDES\n";
	//ShaderIncludes += szExtra;
#else	//	USE_NATIVE_INCLUDES
	//char *szExtra = prepareIncludes();
	//ShaderIncludes += szExtra;
	//cnf_free(szExtra);
  
  //conf_free(szExtra);
#endif

	//szExtra = (char*)ShaderIncludes.c_str();

	bool bShadersInited = false;

#if defined(_DURANGO) || defined(ORBIS)
  eastl::string shaderPath("");
#elif defined(DIRECT3D12)
  eastl::string shaderPath("../../../../../Ephemeris/Sky/resources/Shaders/D3D12/");
#elif defined(VULKAN)
  eastl::string shaderPath("../../../../../Ephemeris/Sky/resources/Shaders/Vulkan/");
#elif defined(METAL)
  eastl::string shaderPath("../../../../../Ephemeris/Sky/resources/Shaders/Metal/");
#endif
  //layout and pipeline for ScreenQuad
  VertexLayout vertexLayout = {};
  vertexLayout.mAttribCount = 1;
  vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
  vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32_SFLOAT;
  vertexLayout.mAttribs[0].mBinding = 0;
  vertexLayout.mAttribs[0].mLocation = 0;
  vertexLayout.mAttribs[0].mOffset = 0;

	do 
	{
		//if ((m_shDistantCloud = pRenderer->addShader("clDistanceCloud.shd", szExtra)) == SHADER_NONE) break;
		//if ((m_shCumulusCloud = pRenderer->addShader("clCumulusCloud.shd", szExtra)) == SHADER_NONE) break;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////


    ShaderLoadDesc skyShader = {};
	eastl::string skyShaderFullPath[2];
    ShaderPath(shaderPath, (char*)"clDistanceCloud.vert", skyShaderFullPath[0]);
    skyShader.mStages[0] = { skyShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
    ShaderPath(shaderPath, (char*)"clDistanceCloud.frag", skyShaderFullPath[1]);
    skyShader.mStages[1] = { skyShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

    addShader(pRenderer, &skyShader, &pClDistanceCloudShader);

    RootSignatureDesc rootDesc = {};
    rootDesc.mShaderCount = 1;
    rootDesc.ppShaders = &pClDistanceCloudShader;

    addRootSignature(pRenderer, &rootDesc, &pClDistanceCloudRootSignature);

	// #TODO
    //DescriptorBinderDesc SkyDescriptorBinderDesc[1] = { { pClDistanceCloudRootSignature } };
    //addDescriptorBinder(pRenderer, 0, 1, SkyDescriptorBinderDesc, &pClDistanceCloudDescriptorBinder);


    PipelineDesc pipelineDescClDistanceCloud;
    {
      pipelineDescClDistanceCloud.mType = PIPELINE_TYPE_GRAPHICS;
      GraphicsPipelineDesc &pipelineSettings = pipelineDescClDistanceCloud.mGraphicsDesc;

      pipelineSettings = { 0 };

      pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
      pipelineSettings.mRenderTargetCount = 1;

      pipelineSettings.pColorFormats = &pSkyRenderTarget->mFormat;
      //pipelineSettings.pSrgbValues = &pSkyRenderTarget->mSrgb;
      pipelineSettings.mSampleCount = pSkyRenderTarget->mSampleCount;
      pipelineSettings.mSampleQuality = pSkyRenderTarget->mSampleQuality;

      pipelineSettings.pRootSignature = pClDistanceCloudRootSignature;
      pipelineSettings.pShaderProgram = pClDistanceCloudShader;
      pipelineSettings.pVertexLayout = &vertexLayout;
      pipelineSettings.pDepthState = &depthStateNoWriteDesc;
      pipelineSettings.pBlendState = &blendStateAlphaDesc;
      pipelineSettings.pRasterizerState = &rasterizerStateDesc;

      addPipeline(pRenderer, &pipelineDescClDistanceCloud, &pDistantCloudPipeline);

    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ShaderLoadDesc spaceShader = {};
	eastl::string spaceShaderFullPath[3];
    ShaderPath(shaderPath, (char*)"clCumulusCloud.vert", spaceShaderFullPath[0]);
    spaceShader.mStages[0] = { spaceShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
    ShaderPath(shaderPath, (char*)"clCumulusCloud.geom", spaceShaderFullPath[1]);
    spaceShader.mStages[1] = { spaceShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };
    ShaderPath(shaderPath, (char*)"clCumulusCloud.frag", spaceShaderFullPath[2]);
    spaceShader.mStages[2] = { spaceShaderFullPath[2].c_str(), NULL, 0, RD_SHADER_SOURCES };

    addShader(pRenderer, &spaceShader, &pClCumulusCloudShader);

    rootDesc.mShaderCount = 1;
    rootDesc.ppShaders = &pClCumulusCloudShader;

    addRootSignature(pRenderer, &rootDesc, &pClCumulusCloudRootSignature);

	// #TODO
    //DescriptorBinderDesc SpaceDescriptorBinderDesc[1] = { { pClCumulusCloudRootSignature } };
    //addDescriptorBinder(pRenderer, 0, 1, SpaceDescriptorBinderDesc, &pClCumulusCloudDescriptorBinder);

    PipelineDesc pipelineDescClCumulusCloud;
    {
      pipelineDescClCumulusCloud.mType = PIPELINE_TYPE_GRAPHICS;
      GraphicsPipelineDesc &pipelineSettings = pipelineDescClCumulusCloud.mGraphicsDesc;

      pipelineSettings = { 0 };

      pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_POINT_LIST;
      pipelineSettings.mRenderTargetCount = 1;

      pipelineSettings.pColorFormats = &pSkyRenderTarget->mFormat;
      //pipelineSettings.pSrgbValues = &pSkyRenderTarget->mSrgb;
      pipelineSettings.mSampleCount = pSkyRenderTarget->mSampleCount;
      pipelineSettings.mSampleQuality = pSkyRenderTarget->mSampleQuality;

      pipelineSettings.pRootSignature = pClCumulusCloudRootSignature;
      pipelineSettings.pShaderProgram = pClCumulusCloudShader;
      pipelineSettings.pVertexLayout = &vertexLayout;
      pipelineSettings.pRasterizerState = &rasterizerStateDesc;

      addPipeline(pRenderer, &pipelineDescClCumulusCloud, &pCumulusCloudPipeline);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////

		{
#ifdef	USE_CLOUDS_DEPTH_RECONSTRUCTION
			//char* szFixedExtra = (char*)cnf_alloc(strlen(szExtra)+1);
			//strcpy_s(szFixedExtra, strlen(szExtra)+1, szExtra);

#ifdef	USE_MULTIPLICATIVE_DENSITY_ACCUMULTAION
			{
				const char* strInverseAlpha = "#define INVERSE_ALPHA\n";
				char* szTempExtra = (char*)conf_alloc(strlen(strInverseAlpha)+strlen(szFixedExtra)+1);
				strcpy(szTempExtra, szFixedExtra);
				strcat(szTempExtra, strInverseAlpha);
				conf_free(szFixedExtra);
				szFixedExtra = szTempExtra;
			}
#endif	//	USE_MULTIPLICATIVE_DENSITY_ACCUMULTAION

#ifdef	CLAMP_IMPOSTOR_PROJ
			{
				const char* strClampImpostorProj = "#define CLAMP_IMPOSTOR_PROJ\n";
				char* szTempExtra = (char*)conf_alloc(strlen(strClampImpostorProj)+strlen(szFixedExtra)+1);
				strcpy(szTempExtra, szFixedExtra);
				strcat(szTempExtra, strClampImpostorProj);
				conf_free(szFixedExtra);
				szFixedExtra = szTempExtra;
			}
#endif	//	CLAMP_IMPOSTOR_PROJ

			//if ((m_shImpostorCloud = pRenderer->addShader("clImpostorCloud.shd", szFixedExtra)) == SHADER_NONE) break;
			//cnf_free(szFixedExtra);

    ShaderLoadDesc impostorShader = {};
	eastl::string impostorShaderFullPath[3];
    ShaderPath(shaderPath, (char*)"clImpostorCloud.vert", impostorShaderFullPath[0]);
    impostorShader.mStages[0] = { impostorShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
    ShaderPath(shaderPath, (char*)"clImpostorCloud.frag", impostorShaderFullPath[1]);
    impostorShader.mStages[1] = { impostorShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

      addShader(pRenderer, &impostorShader, &pImposterCloudShader);

      rootDesc.mShaderCount = 1;
      rootDesc.ppShaders = &pImposterCloudShader;

      addRootSignature(pRenderer, &rootDesc, &pImposterCloudRootSignature);

	  // #TODO
      //DescriptorBinderDesc ImpostorDescriptorBinderDesc[1] = { { pImposterCloudRootSignature } };
      //addDescriptorBinder(pRenderer, 0, 1, ImpostorDescriptorBinderDesc, &pImposterCloudDescriptorBinder);

      PipelineDesc pipelineDescImpostor;
      {
        pipelineDescImpostor.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc &pipelineSettings = pipelineDescImpostor.mGraphicsDesc;

        pipelineSettings = { 0 };

        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_POINT_LIST;
        pipelineSettings.mRenderTargetCount = 1;

        pipelineSettings.pColorFormats = &pSkyRenderTarget->mFormat;
        //pipelineSettings.pSrgbValues = &pSkyRenderTarget->mSrgb;
        pipelineSettings.mSampleCount = pSkyRenderTarget->mSampleCount;
        pipelineSettings.mSampleQuality = pSkyRenderTarget->mSampleQuality;

        pipelineSettings.pRootSignature = pImposterCloudRootSignature;
        pipelineSettings.pShaderProgram = pImposterCloudShader;
        pipelineSettings.pVertexLayout = &vertexLayout;
        pipelineSettings.pRasterizerState = &rasterizerStateDesc;

        addPipeline(pRenderer, &pipelineDescImpostor, &pImposterCloudPipeline);
      }

#endif	//	USE_CLOUDS_DEPTH_RECONSTRUCTION

		}

		bShadersInited = true;
	} while (false);

#ifndef	USE_NATIVE_INCLUDES
	//cnf_free(szExtra);
#endif
	if (!bShadersInited) return false;

	SyncToken token = {};
	
  TextureLoadDesc CloudFlatTextureDesc = {};
#if defined(_DURANGO) || defined(ORBIS)
	PathHandle CloudFlatTextureFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, "Textures/flat");
#else
	PathHandle CloudFlatTextureFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, "../../../Ephemeris/Sky/resources/Textures/flat");
#endif
	CloudFlatTextureDesc.pFilePath = CloudFlatTextureFilePath;
  CloudFlatTextureDesc.ppTexture = &m_tDistantCloud;
  addResource(&CloudFlatTextureDesc, &token, LOAD_PRIORITY_NORMAL);

  TextureLoadDesc CloudCumulusTextureDesc = {};
#if defined(_DURANGO) || defined(ORBIS)
	PathHandle CloudCumulusTextureFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, "Textures/cumulus_particles");
#else
	PathHandle CloudCumulusTextureFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, "../../../Ephemeris/Sky/resources/Textures/cumulus_particles");
#endif
	CloudCumulusTextureDesc.pFilePath = CloudCumulusTextureFilePath;
  CloudCumulusTextureDesc.ppTexture = &m_tCumulusCloud;
  addResource(&CloudCumulusTextureDesc, &token, LOAD_PRIORITY_NORMAL);


  BufferLoadDesc CumulusUniformDesc = {};
  CumulusUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  CumulusUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
  CumulusUniformDesc.mDesc.mSize = sizeof(CumulusCloudUniformBuffer);
  CumulusUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
  CumulusUniformDesc.pData = NULL;

  for (uint i = 0; i < gImageCount; i++)
  {
    CumulusUniformDesc.ppBuffer = &pCumulusUniformBuffer[i];
    addResource(&CumulusUniformDesc, &token, LOAD_PRIORITY_NORMAL);
  }

  BufferLoadDesc DistantUniformDesc = {};
  DistantUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  DistantUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
  DistantUniformDesc.mDesc.mSize = sizeof(DistantCloudUniformBuffer);
  DistantUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
  DistantUniformDesc.pData = NULL;

  for (uint i = 0; i < gImageCount; i++)
  {
    DistantUniformDesc.ppBuffer = &pDistantUniformBuffer[i];
    addResource(&DistantUniformDesc, &token, LOAD_PRIORITY_NORMAL);
  }

  BufferLoadDesc imposterUniformDesc = {};
  imposterUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  imposterUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
  imposterUniformDesc.mDesc.mSize = sizeof(ImposterUniformBuffer);
  imposterUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
  imposterUniformDesc.pData = NULL;

  for (uint i = 0; i < gImageCount; i++)
  {
    imposterUniformDesc.ppBuffer = &pImposterUniformBuffer[i];
    addResource(&imposterUniformDesc, &token, LOAD_PRIORITY_NORMAL);
  }

  waitForToken(&token);
	
	return true;
}

void CloudsManager::unload()
{
//	TODO: C: implement unload!

  removeShader(pRenderer, pClDistanceCloudShader);
  removeShader(pRenderer, pClCumulusCloudShader);
  removeShader(pRenderer, pImposterCloudShader);

  removeRootSignature(pRenderer, pClDistanceCloudRootSignature);
  removeRootSignature(pRenderer, pClCumulusCloudRootSignature);
  removeRootSignature(pRenderer, pImposterCloudRootSignature);

  // #TODO
  //removeDescriptorBinder(pRenderer, pClDistanceCloudDescriptorBinder);
  //removeDescriptorBinder(pRenderer, pClCumulusCloudDescriptorBinder);
  //removeDescriptorBinder(pRenderer, pImposterCloudDescriptorBinder);

  removeResource(m_tDistantCloud);
  removeResource(m_tCumulusCloud);

  removePipeline(pRenderer, pDistantCloudPipeline);
  removePipeline(pRenderer, pCumulusCloudPipeline);
  removePipeline(pRenderer, pImposterCloudPipeline);

  removeSampler(pRenderer, linearClamp);
  removeSampler(pRenderer, trilinearClamp);

  for (uint i = 0; i < gImageCount; i++)
  {
    removeResource(pCumulusUniformBuffer[i]);
    removeResource(pDistantUniformBuffer[i]);
    removeResource(pImposterUniformBuffer[i]);
  }
}

mat4 rotateY(const float angle) {
  float cosA = cosf(angle), sinA = sinf(angle);

  return mat4(
    vec4(cosA, 0, -sinA, 0),
    vec4(0, 1, 0, 0),
    vec4(sinA, 0, cosA, 0),
    vec4(0, 0, 0, 1));
}

mat4 scale(const float x, const float y, const float z) {
  return mat4(vec4(x, 0, 0, 0), vec4(0, y, 0, 0), vec4(0, 0, z, 0), vec4(0, 0, 0, 1));
}

void CloudsManager::loadDistantClouds()
{
	mat4	transform;

/*
	//	Scale Y too in order to correctly transform sun dir to local space
	transform = scale(2000,2000,2000);
	transform.rows[0].w += 0;
	transform.rows[1].w += 2000;
	transform.rows[2].w += 5000;
	m_DistantClouds.push_back(DistantCloud(transform));

	transform = rotateY(1.7f)*scale(3000,3000,3000);
	transform.rows[0].w += 0;
	transform.rows[1].w += 2500;
	transform.rows[2].w += 20000;
	m_DistantClouds.push_back(DistantCloud(transform));
*/
	return;

	// Add some more clouds at random
	for (int i = 0; i < 15; i++)
	{
		float Orientation = RandomValue() * PI * 2.0f;
		float CloudSize = RandomValue() * 2000.0f + 2000.0f;
		vec3 CloudPosition = vec3(0.0f, 2500.0f, 0.0f);
		//CloudPosition.x += RandomValueNormalized() * 20000.0f;
		//CloudPosition.y += RandomValue() * 1000.0f;
		//CloudPosition.z += RandomValueNormalized() * 20000.0f;

    CloudPosition[0] += RandomValueNormalized() * 20000.0f;
    CloudPosition[1] += RandomValue() * 1000.0f;
    CloudPosition[2] += RandomValueNormalized() * 20000.0f;


		transform = rotateY(Orientation) * scale(CloudSize, CloudSize, CloudSize);
		//transform.rows[0].w += CloudPosition.x;
		//transform.rows[1].w += CloudPosition.y;
		//transform.rows[2].w += CloudPosition.z;

    transform[3][0] += CloudPosition.getX();
    transform[3][1] += CloudPosition.getY();
    transform[3][2] += CloudPosition.getZ();

		createDistantCloud(transform, m_tDistantCloud);
	}
}

void CloudsManager::drawFrame(Cmd *cmd, const mat4 &vp, const mat4 &view, const vec3 &camPos, const vec3 &camPosLocalKM, const vec4 &offsetScale, vec3 &sunDir,
 Texture* Transmittance, Texture* Irradiance, Texture* Inscatter, Texture* shaftsMask, float exposure, vec2 inscatterParams, vec4 QNnear, const Texture* rtDepth, bool bSoftClouds )
{
	sortClouds(camPos);

	size_t cloudsCount = m_SortedClouds.size(); 

	vec3 camDir = view.getRow(2).getXYZ();

	for (uint i=0; i<cloudsCount; ++i)
	{
		switch (m_SortedClouds[i].type)
		{
		case CloudSortData::CT_Cumulus:
			//renderCumulusCloud(Transmittance, Irradiance, Inscatter, shaftsMask, exposure, camPos, sunDir, inscatterParams, m_SortedClouds[i].index, vp);
			{
				int CumulusIndex = (int)m_SortedClouds[i].index;
				vec2 XZCloudPos = vec2(m_CumulusClouds[CumulusIndex].Transform().getRow(0).getW(), m_CumulusClouds[CumulusIndex].Transform().getRow(2).getW());
				vec2 XZCamPos = vec2(camPos.getX(), camPos.getZ());

				vec2 offset = XZCamPos - XZCloudPos;

				float distanceToCloud = length(offset);


				//float distanceToCloud = sqrtf(m_SortedClouds[i].distanceSQR);

        float cloudOpacity = 1;
        if ( distanceToCloud > m_Params.fCumulusExistanceR )
        {
            cloudOpacity = 0;
        }
        else if ( distanceToCloud > m_Params.fCumulusFadeStart )
        {
            cloudOpacity = 1 - ( distanceToCloud - m_Params.fCumulusFadeStart ) / ( m_Params.fCumulusExistanceR - m_Params.fCumulusFadeStart );
        }

				cloudOpacity = 1.0f;

				renderCumulusCloud(cmd, Transmittance, Irradiance, Inscatter, shaftsMask, 
					exposure, camPosLocalKM, offsetScale, sunDir, inscatterParams,
					CumulusIndex, vp, cloudOpacity,
					camDir, QNnear, rtDepth, bSoftClouds);
			}
			break;
		case CloudSortData::CT_Distant:
			//renderDistantCloud(Transmittance, Irradiance, Inscatter, shaftsMask, exposure, camPos, sunDir, inscatterParams, m_SortedClouds[i].index, vp);
			renderDistantCloud(cmd, Transmittance, Irradiance, Inscatter, shaftsMask, exposure, camPosLocalKM, offsetScale, sunDir, inscatterParams, m_SortedClouds[i].index, vp);
			break;
		}
	}
}

//void CloudsManager::drawFrame( const float4x4 &vp, const float4x4 &view, const float3 &camPos, float3 &sunDir,TextureID Transmittance, TextureID Irradiance, TextureID Inscatter, TextureID shaftsMask, float exposure, float2 inscatterParams )
//{
//	m_pRenderer->reset();
//	m_pRenderer->setRasterizerState(noCull);
//	m_pRenderer->setDepthState(noDepthWrite);
//	m_pRenderer->setBlendState(alphaBlend);
//
//	m_pRenderer->setShader(m_shDistantCloud);
//
//	m_pRenderer->setTexture("transmittanceSampler", Transmittance);
//	m_pRenderer->setTexture("irradianceSampler", Irradiance);
//	m_pRenderer->setTexture("inscatterSampler", Inscatter);
//	m_pRenderer->setTexture("base", m_tDistantCloud);
//	m_pRenderer->setTexture("SunShafts", shaftsMask);
//
//	m_pRenderer->setShaderConstant1f("exposure", exposure);
//	m_pRenderer->setShaderConstant3f("camPos", camPos);
//	m_pRenderer->setShaderConstant3f("s", sunDir);
//	m_pRenderer->setShaderConstant2f("inscatterParams", inscatterParams);
//	m_pRenderer->setShaderConstant1f("StepSize", m_Params.fStepSize);
//	m_pRenderer->setShaderConstant1f("Attenuation", m_Params.fAttenuation);
//	m_pRenderer->setShaderConstant1f("AlphaSaturation", m_Params.fAlphaSaturation);
//
//
//	for (unsigned int i=0; i<m_DistantClouds.size(); ++i)
//	{
//		m_pRenderer->setShaderConstant4x4f("mvp", vp*m_DistantClouds[i].Transform());
//		m_pRenderer->setShaderConstant4x4f("model", m_DistantClouds[i].Transform());
//		mat4 invTransform = !m_DistantClouds[i].Transform();
//		vec4 localSun = invTransform*vec4(sunDir, 0);
//		m_pRenderer->setShaderConstant3f("localSun", normalize(localSun.xyz()));
//
//		m_pRenderer->apply();
//
//		m_pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
//		m_pDevice->Draw(4, 0);
//	}
//
//	///////////////////////////////////////
//	//	Draw cumulus clouds. Basic shading. No lighting.
//	#if 0
//	m_pRenderer->reset();
//	m_pRenderer->setRasterizerState(noCull);
//	m_pRenderer->setDepthState(noDepthWrite);
//	m_pRenderer->setBlendState(alphaBlend);
//
//	m_pRenderer->setShader(m_shCumulusCloud);
//
//	m_pRenderer->setShaderConstant3f("camPos", camPos);
//	m_pRenderer->setShaderConstant4x4f("vp", vp);
//
//	m_pRenderer->setTexture("base", m_tCumulusCloud);
//	
//	
//	for (unsigned int i=0; i<m_CumulusClouds.size(); ++i)
//	{
//		const float particleSize = m_CumulusClouds[i].ParticlesScale();
//		//m_pRenderer->setShaderConstant4x4f("mvp", vp*m_CumulusClouds[i].Transform());
//		m_pRenderer->setShaderConstant4x4f("model", m_CumulusClouds[i].Transform());
//		m_pRenderer->setShaderConstant3f("dx", view.rows[0].xyz() * particleSize);
//		m_pRenderer->setShaderConstant3f("dy", view.rows[1].xyz() * particleSize);
//		m_CumulusClouds[i].setupConstants(camPos, *m_pRenderer, "OffsetScale","TexIDs");
//		m_pRenderer->apply();
//
//		m_pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
//		m_pDevice->Draw((UINT)m_CumulusClouds[i].getParticlesCount(), 0);
//	}
//#endif
//
//	////////////////////////////////////////
//	//	Draw impostors
//	m_pRenderer->reset();
//	m_pRenderer->setRasterizerState(noCull);
//	m_pRenderer->setDepthState(noDepthWrite);
//	m_pRenderer->setBlendState(alphaBlend);
//	//m_pRenderer->setBlendState(oneAlphaBlend);
//
//	m_pRenderer->setShader(m_shDistantCloud);
//
//	m_pRenderer->setTexture("transmittanceSampler", Transmittance);
//	m_pRenderer->setTexture("irradianceSampler", Irradiance);
//	m_pRenderer->setTexture("inscatterSampler", Inscatter);
//	m_pRenderer->setTexture("SunShafts", shaftsMask);
//
//	m_pRenderer->setShaderConstant1f("exposure", exposure);
//	m_pRenderer->setShaderConstant3f("camPos", camPos);
//	m_pRenderer->setShaderConstant3f("s", sunDir);
//	m_pRenderer->setShaderConstant2f("inscatterParams", inscatterParams);
//	m_pRenderer->setShaderConstant1f("StepSize", m_Params.fStepSize);
//	m_pRenderer->setShaderConstant1f("Attenuation", m_Params.fAttenuation);
//	m_pRenderer->setShaderConstant1f("AlphaSaturation", m_Params.fCumulusAlphaSaturation);
//
//	for (unsigned int i=0; i<m_Impostors.size(); ++i)
//	{
//		m_pRenderer->setTexture("base", m_Impostors[i].getImpostorTexture());
//		m_pRenderer->setShaderConstant4x4f("mvp", vp*m_Impostors[i].Transform());
//		m_pRenderer->setShaderConstant4x4f("model", m_Impostors[i].Transform());
//		mat4 invTransform = !m_Impostors[i].Transform();
//		vec4 localSun = invTransform*vec4(sunDir, 0);
//		m_pRenderer->setShaderConstant3f("localSun", normalize(localSun.xyz()));
//
//		m_pRenderer->apply();
//
//		m_pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
//		m_pDevice->Draw(4, 0);
//	}
//}

/*
char* CloudsManager::prepareIncludes()
{
	const int iIncludeNumber = 3;
	char	*szIncludeNames[iIncludeNumber] = {"SkyDomeCommon.h","sdCommon.h","sdFinal.h"};
	FileHandle	file[iIncludeNumber];
	size_t		length[iIncludeNumber];
	size_t		iTotalLength = 0;	

	const int iExtraDataPerFile = 260*2;
	const char szLineReset[] = "#line 1 \"%s\"\n";

	for (int i=0; i<iIncludeNumber; ++i)
	{
		file[i] = pFS->OpenFile(szIncludeNames[i], FM_ReadOnly, FSR_Shaders);

		if (file[i] == NULL)
		{
			//ErrorMsg(String("Couldn't load \"") + szIncludeNames[i] + "\"");
			length[i] = 0;
			cnf_Error("Couldn't load \"");
			cnf_Error(szIncludeNames[i], false);
			cnf_Error("\"\n", false);
		}
		else
		{
			length[i] = pFS->FileSize(file[i]);
		}

		iTotalLength += length[i];
		iTotalLength += iExtraDataPerFile;
	}

	int iCurrentPos = 0;
	char *shaderText = (char*)cnf_alloc(iTotalLength+1);
	for (int i=0; i<iIncludeNumber; ++i)
	{
		if (length[i])
		{
			char buf[iExtraDataPerFile];
			sprintf_s(buf, szLineReset, szIncludeNames[i]);
			const int strLen = strlen(buf);
			memcpy(shaderText+iCurrentPos, buf, strLen);
			iCurrentPos += strLen;

			pFS->FileRead(shaderText+iCurrentPos, length[i], file[i]);
			pFS->FileClose(file[i]);
			iCurrentPos += length[i];
		}
	}


	shaderText[iCurrentPos] = '\0';

	return shaderText;
}
*/

void CloudsManager::loadCumulusClouds()
{
	mat4	transform;
	eastl::vector<vec4> particlePosScale;
  eastl::vector<ParticleProps>	particleProps;

	//	Igor: advanced impostor pos reconstruction test  
	//transform = scale(2000,2000,2000);
	//transform.rows[0].w += 4000;
	//transform.rows[1].w += 4000;
	//transform.rows[2].w += 4000;
	//m_CumulusClouds.push_back(CumulusCloud(transform,1300));
	//generateCumulusCloud(m_CumulusClouds.back());

/*
	transform = scale(1000,1000,1000);
	transform.rows[0].w += 4000;
	transform.rows[1].w += 4000;
	transform.rows[2].w += 4000;
	m_CumulusClouds.push_back(CumulusCloud(transform,450));
	//m_CumulusClouds.push_back(CumulusCloud(transform,900));
	generateCumulusCloud(m_CumulusClouds.back());

	transform = scale(2000,2000,2000);
	transform.rows[0].w -= 4000;
	transform.rows[1].w += 4000;
	transform.rows[2].w += 30000;
	m_CumulusClouds.push_back(CumulusCloud(transform,950));
	generateCumulusCloud(m_CumulusClouds.back());

	transform = scale(1500,1500,1500);
	transform.rows[0].w -= 10000;
	transform.rows[1].w += 5000;
	transform.rows[2].w += 28000;
	m_CumulusClouds.push_back(CumulusCloud(transform,950));
	generateCumulusCloud(m_CumulusClouds.back());

	transform = scale(1500,1500,1500);
	transform.rows[0].w += 8000;
	transform.rows[1].w += 4500;
	transform.rows[2].w += 1000;
	m_CumulusClouds.push_back(CumulusCloud(transform,650));
	generateCumulusCloud(m_CumulusClouds.back());
	*/

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
	{
		//float CloudSize = RandomValue() * 1500.0f + 500.0f;
		float CloudSize = 1000.0f;
		float ParticleScale = CloudSize * 0.9f;
		vec3 CloudPosition = vec3(0.0f, 3500.0f, 0.0f);
		CloudPosition[0] += i * 9000.0f + RandomValue()*1000.0f;
		CloudPosition[1] += RandomValue() * 1500.0f;
		CloudPosition[2] += j * 9000.0f + RandomValue()*2000.0f;

		transform = scale(CloudSize, CloudSize, CloudSize);
		//transform.rows[0].w += CloudPosition.x;
		//transform.rows[1].w += CloudPosition.y;
		//transform.rows[2].w += CloudPosition.z;
		
    transform[3][0] = transform[3][0] + CloudPosition[0];
    transform[3][1] = transform[3][1] + CloudPosition[1];
    transform[3][2] = transform[3][2] + CloudPosition[2];

//m_CumulusClouds.push_back(CumulusCloud(*transform, ParticleScale));
		//generateCumulusCloud(m_CumulusClouds.back());
		generateCumulusCloudParticles( particlePosScale, particleProps);
		
    //assert( particlePosScale.size() == particleProps.size() );
    if(particlePosScale.size() == particleProps.size())
      return;

    if ( ! particlePosScale.empty() )
		{
			createCumulusCloud(transform, m_tCumulusCloud, ParticleScale, 
				&particlePosScale[ 0 ], &particleProps[ 0 ], (uint32_t)particlePosScale.size(), true );
		}
	}

	return;

	// Add some more clouds at random
	for (int i = 0; i < 15; i++)
	//for (int i = 0; i < 1; i++)
	{
		float CloudSize = RandomValue() * 1500.0f + 500.0f;
		float ParticleScale = CloudSize * 0.9f;
		vec3 CloudPosition = vec3(0.0f, 2500.0f, 0.0f);
		CloudPosition[0] += RandomValueNormalized() * 20000.0f;
		CloudPosition[1] += RandomValue() * 1500.0f;
		CloudPosition[2] += RandomValueNormalized() * 20000.0f;

		transform = scale(CloudSize, CloudSize, CloudSize);
		//transform.rows[0].w += CloudPosition.x;
		//transform.rows[1].w += CloudPosition.y;
		//transform.rows[2].w += CloudPosition.z;

		transform[3][0] = transform[3][0] + CloudPosition[0];
		transform[3][1] = transform[3][1] + CloudPosition[1];
		transform[3][2] = transform[3][2] + CloudPosition[2];

		generateCumulusCloudParticles( particlePosScale, particleProps);
		//assert( particlePosScale.size() == particleProps.size() );

    if (particlePosScale.size() == particleProps.size())
      return;

		if ( ! particlePosScale.empty() )
		{
			createCumulusCloud(transform, m_tCumulusCloud, ParticleScale, 
				&particlePosScale[ 0 ], &particleProps[ 0 ], (uint32_t)particlePosScale.size(), true );
		}
		//m_CumulusClouds.push_back(CumulusCloud(transform, ParticleScale));
		//generateCumulusCloud(m_CumulusClouds.back());
	}
	
}

void CloudsManager::drawDebug()
{
	return;
	/*
	float width = 400;
	float height = 200;

	size_t numImpostors = m_Impostors.size();
	numImpostors = min(numImpostors, (size_t)6);
	float fitWidth = (float)max(numImpostors, (size_t)3);
	float fitHeight = fitWidth / 3.0f * 2.0f;

	// calculate size of render target previews
	float WidthX = width / fitWidth;
	float WidthY = height / fitHeight;
	float PosX = width / 100.0f;
	float PosY = height - WidthY;

	m_pRenderer->setup2DMode(0, (float) width, 0, (float) height);
	
	for (size_t i=0; i<numImpostors; ++i)
		m_pRenderer->drawRenderTargetToScreen(m_Impostors[i].getImpostorTexture(), SS_NONE, PosX+WidthX*i, PosY, WidthX, WidthY, 1, eRGB);
		*/
}

void CloudsManager::update(float frameTime)
{
	vec3 motionDir = vec3(m_Params.fCloudSpeed, 0.0f, 0.0f) * frameTime;

	if (m_Params.bMoveClouds)
	{
		for (unsigned int i=0; i<m_CumulusClouds.size(); ++i)
		{
			m_CumulusClouds[i].moveCloud(motionDir);
		}

// 		for (unsigned int i=0; i<m_DistantClouds.size(); ++i)
// 		{
// 			m_DistantClouds[i].moveCloud(motionDir);
// 		}
	}
}

#ifdef	CLAMP_IMPOSTOR_PROJ

void getCameraCornerDirs(const float4x4 &vp, float camNear, float3 cameraCornerDirs[4])
{
//	TODO: Igor: we can pass inversed VP from the game
	float4x4 invVP = !vp;

	for (int VertexID=0; VertexID<4; ++VertexID)
	{
		float4 position;
		position.x = (VertexID & 1)? 1.0f : -1.0f;
		position.y = (VertexID & 2)? 1.0f : -1.0f;
		position.z = 0.0f;
		position.w = camNear;
		position.x *= position.w;
		position.y *= position.w;

		position = invVP * position;

		cameraCornerDirs[VertexID] = normalize(position.xyz());
	}

}
#endif	//	CLAMP_IMPOSTOR_PROJ

void CloudsManager::prepareImpostors(Cmd *cmd, const vec3 & camPos, const mat4 & view, const mat4 &vp, float camNear )
{
	//ImpostorUpdateMode updateMode = m_Params.bUpdateEveryFrame?IUM_EveryFrame:IUM_PositionBased;
	//	TODO: A: Igor: this is a workaround for moving clouds, since need to update impostor if the cloud is moving.
	ImpostorUpdateMode updateMode = (m_Params.bUpdateEveryFrame||m_Params.bMoveClouds)?IUM_EveryFrame:IUM_PositionBased;

#ifdef	CLAMP_IMPOSTOR_PROJ
	float3 cameraCornerDirs[4];
	getCameraCornerDirs(vp, camNear, cameraCornerDirs);
#endif	//	CLAMP_IMPOSTOR_PROJ

	//for (unsigned int i=0; i<m_CumulusClouds.size(); ++i)
	//	Igor: use the sorted clouds list since m_CumulusClouds might have unused clouds.
	size_t cloudsCount = m_SortedClouds.size();
	for (uint i=0; i<cloudsCount; ++i)
	{
		if (m_SortedClouds[i].type == CloudSortData::CT_Cumulus)
		{
			uint32 cumulusID = (uint32_t)m_SortedClouds[i].index;

      mat4 v;
      mat4 vp;
      vec3 dx;
      vec3 dy;
      vec2 packDepthParams;
      float masterParticleRotation;

			bool needUpdate;
			m_Impostors[cumulusID].setupRenderer(cmd, &m_CumulusClouds[cumulusID], camPos, camNear, view,
      v, vp, dx, dy,
      updateMode, needUpdate
#ifdef	CLAMP_IMPOSTOR_PROJ
				, cameraCornerDirs
#endif	//	CLAMP_IMPOSTOR_PROJ	
#ifdef	USE_CLOUDS_DEPTH_RECONSTRUCTION
        , packDepthParams
#endif
#ifdef STABLISE_PARTICLE_ROTATION
        , masterParticleRotation
#endif
				);
			if (!needUpdate)
            {
                continue;
            }

			//pRenderer->setShaderConstant4x4f("model", m_CumulusClouds[cumulusID].Transform());
#ifdef SN_TARGET_PS3
			m_CumulusClouds[cumulusID].setupConstants(camPos, "OffsetScale","TexIDs");
#else
			//m_CumulusClouds[cumulusID].setupConstants(camPos, "OffsetScale","ParticleProps");
#endif

      cmdBindPipeline(cmd, pCumulusCloudPipeline);

	  BufferUpdateDesc BufferUniformSettingDesc = { pCumulusUniformBuffer[gFrameIndex] };
	  beginUpdateResource(&BufferUniformSettingDesc);
      CumulusCloudUniformBuffer& tempBuffer = *(CumulusCloudUniformBuffer*)BufferUniformSettingDesc.pMappedData;
      //tempBuffer.model;
      memcpy(tempBuffer.OffsetScale, m_CumulusClouds[cumulusID].m_OffsetScales.data(), sizeof(vec4) *  MaxParticles);
      memcpy(tempBuffer.ParticleProps, m_CumulusClouds[cumulusID].m_particleProps.data(), sizeof(ParticleProps) * QMaxParticles);
      tempBuffer.vp = vp;
      tempBuffer.v = v;
      tempBuffer.dx = dx;
      tempBuffer.dy = dy;
      tempBuffer.packDepthParams = packDepthParams;
      tempBuffer.masterParticleRotation = masterParticleRotation;
      //tempBuffer.zNear;
      //tempBuffer.packDepthParams;
      //tempBuffer.masterParticleRotation;
	  endUpdateResource(&BufferUniformSettingDesc, NULL);


			//	TODO: Igor: fix it
			//	Have to be here since we change rt in m_Impostors[cumulusID].setupRenderer
			//pRenderer->setTexture("base", m_CumulusClouds[cumulusID].Texture(), linearClamp);	

      DescriptorData preImpParams[3] = {};

      preImpParams[0].pName = "CumulusUniformBuffer";
      preImpParams[0].ppBuffers = &pCumulusUniformBuffer[gFrameIndex];

      preImpParams[1].pName = "linearClamp";
      preImpParams[1].ppSamplers = &linearClamp;

      preImpParams[2].pName = "base";
      preImpParams[2].ppTextures = &m_tCumulusCloud;

	  // #TODO
      //cmdBindDescriptors(cmd, pClCumulusCloudDescriptorBinder, pClCumulusCloudRootSignature, 3, preImpParams);
      //cmdBindVertexBuffer(cmd, 1, &pGlobalTriangularVertexBuffer, NULL);
      
      cmdDraw(cmd, (uint32_t)m_CumulusClouds[cumulusID].getParticlesCount(), 0);

      cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);



			//	Igor: since it's not generic apply be very careful with what you set in this loop
			//pRenderer->applyTextures();
			//pRenderer->applyConstants();
			//pRenderer->apply();

			//pRenderer->drawArrays(PRIM_POINTS, 0, m_CumulusClouds[cumulusID].getParticlesCount());
			m_Impostors[cumulusID].resolve();
		}
	}
}

void CloudsManager::generateCumulusCloudParticles( eastl::vector<vec4> & particlePosScale, eastl::vector<ParticleProps> & particleProps)
{
	particlePosScale.clear();
	particleProps.clear();

	int dimX = 10;
	int dimY = 10;

	// Create the puffs that make up our cumulus cloud
	for (int i=0; i<dimX; ++i)
		for (int j=0; j<dimY; ++j)
		{
			vec4 offset;
			offset[0] = -1.0f + i * 2.0f / (dimX-1) + 64.0f * (0.5f - (((float)rand()) / RAND_MAX)) / (dimX - 1);
			offset[1] = 2.0f * (0.5f - (((float)rand()) / RAND_MAX)) / 5;
			offset[2] = -1.0f + j * 2.0f / (dimY-1) + 64.0f * (0.5f - (((float)rand()) / RAND_MAX)) / (dimY - 1);

			// use w component to hold fade factor for this particle
			offset[3] = 1.0f;

			if (sqrt((i-4.5f)*(i-4.5) + (j-4.5f)*(j-4.5f)) < 4.5)
			{
				//if (rand() % 10 < 8)
				{
					particlePosScale.push_back(offset);
					particleProps.push_back(ParticleProps());

					particleProps.back().texID = float((i + j * dimY) % 16);
					particleProps.back().angle = float((i + j * dimY) % 16);
					//particleProps.back().transparency = 0.5*float(i % 16)/15.0;
					//cloud.pushParticle(offset, 1, (i + j * dimY) % 16);
					//cloud.pushParticle(offset, 1, rand() % 16);
				}
			}
		}
		/**/

		/*
		for (int i=0; i<8; ++i)
		{
		vec3	offset;
		offset.x = i&1?-1.0f:1.0f;
		offset.y = i&2?-1.0f:1.0f;
		offset.z = i&4?-1.0f:1.0f;
		cloud.pushParticle(offset, 1, i);
		}
		*/
}

// void CloudsManager::prepareSortData()
// {
// 	m_SortedClouds.clear();
// 
// 	for (size_t i=0; i<m_DistantClouds.size(); ++i)
// 	{
// 		m_SortedClouds.push_back(CloudSortData());
// 		m_SortedClouds.back().type = CloudSortData::CT_Distant;
// 		m_SortedClouds.back().index = i;
// 	}
// 
// 	for (size_t i=0; i<m_CumulusClouds.size(); ++i)
// 	{
// 		m_SortedClouds.push_back(CloudSortData());
// 		m_SortedClouds.back().type = CloudSortData::CT_Cumulus;
// 		m_SortedClouds.back().index = i;
// 	}
// }

void CloudsManager::sortClouds( const vec3 & camPos )
{
	size_t a = 0;
	size_t b = m_SortedClouds.size(); 

	if (!b) return;

	for (uint i=0; i<b; ++i)
	{
		const mat4 *transform = 0;

		switch (m_SortedClouds[i].type)
		{
		case CloudSortData::CT_Cumulus:
			{
				//transform = &m_Impostors[m_SortedClouds[i].index].Transform();
				float distance = m_Impostors[m_SortedClouds[i].index].SortDepth();
				m_SortedClouds[i].distanceSQR = distance*distance;
			}
			break;
		case CloudSortData::CT_Distant:
			{
				transform = &m_DistantClouds[m_SortedClouds[i].index].Transform();
				vec3 delta = camPos - vec3( transform->getRow(0).getW(), transform->getRow(1).getW(), transform->getRow(2).getW());

				m_SortedClouds[i].distanceSQR = dot(delta,delta);
			}
			break;
		}

// 		float3 delta = camPos - float3( transform->rows[0].w, transform->rows[1].w, transform->rows[2].w);
// 
// 		m_SortedClouds[i].distanceSQR = dot(delta,delta);
	}

	//	Use bubble sort since cloud particles should have
	//	high time coherency for the distances from camera
	//	Igor: if there's only one cloud in the queue we will experience b underflow
	--b;
	while (a<b)
	{
		size_t lastChanged = b;
		for (size_t i=b; i>a; --i)
		{
			if (m_SortedClouds[i].distanceSQR>m_SortedClouds[i-1].distanceSQR)
			{
				CloudSortData fTmp = m_SortedClouds[i-1];
				m_SortedClouds[i-1] = m_SortedClouds[i];
				m_SortedClouds[i] = fTmp;

				lastChanged = i;
			}
		}

		a = lastChanged;
	}
}

void CloudsManager::renderDistantCloud(Cmd* cmd,
	Texture* Transmittance, Texture* Irradiance, Texture* Inscatter, Texture* shaftsMask,
	float exposure, const vec3 & localCamPosKM, const vec4 &offsetScaleToLocalKM, vec3 & sunDir, vec2 inscatterParams, size_t i, const mat4 & vp)
{
/*
	pRenderer->reset();
	pRenderer->setRasterizerState(noCull);

	pRenderer->setDepthState(noDepthWrite);

	// PSS: until we fix impostor clipping...
	if (!m_Params.bDepthTestClouds)
	{
		pRenderer->setDepthState(noDepthAtAll);
	}


	pRenderer->setBlendState(alphaBlend);

	pRenderer->setShader(m_shDistantCloud);

	pRenderer->setTexture("transmittanceSampler", Transmittance, linearClamp);
	pRenderer->setTexture("irradianceSampler", Irradiance, linearClamp);
	pRenderer->setTexture("inscatterSampler", Inscatter, linearClamp);
	pRenderer->setTexture("base", m_tDistantCloud, linearClamp);
	pRenderer->setTexture("SunShafts", shaftsMask, linearClamp);

	pRenderer->setSamplerState("linearClamp", linearClamp);

	pRenderer->setShaderConstant1f("exposure", exposure);
	pRenderer->setShaderConstant3f("camPos", localCamPosKM);
	pRenderer->setShaderConstant4f("offsetScale", offsetScaleToLocalKM);
	pRenderer->setShaderConstant3f("s", sunDir);
	pRenderer->setShaderConstant2f("inscatterParams", inscatterParams);
	pRenderer->setShaderConstant1f("StepSize", m_Params.fStepSize);
	pRenderer->setShaderConstant1f("Attenuation", m_Params.fAttenuation);
	pRenderer->setShaderConstant1f("AlphaSaturation", m_Params.fAlphaSaturation);
	pRenderer->setShaderConstant1f("DirectLightIntensity", m_Params.fStratusSunIntensity);
	pRenderer->setShaderConstant1f("AmbientLightIntensity", m_Params.fStratusAmbientIntensity);

	//	Instance data!!!
	pRenderer->setShaderConstant4x4f("mvp", vp*m_DistantClouds[i].Transform());
	pRenderer->setShaderConstant4x4f("model", m_DistantClouds[i].Transform());
	mat4 invTransform = !m_DistantClouds[i].Transform();
	vec4 localSun = invTransform*vec4(sunDir, 0);
	pRenderer->setShaderConstant3f("localSun", normalize(localSun.xyz()));

	pRenderer->apply();

	//m_pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	//m_pDevice->Draw(4, 0);

	pRenderer->drawArrays(PRIM_TRIANGLE_STRIP, 0, 4);
*/

  cmdBindPipeline(cmd, pDistantCloudPipeline);

  BufferUpdateDesc BufferUniformSettingDesc = { pDistantUniformBuffer[gFrameIndex] };
  beginUpdateResource(&BufferUniformSettingDesc);
  DistantCloudUniformBuffer& tempBuffer = *(DistantCloudUniformBuffer*)BufferUniformSettingDesc.pMappedData;
  tempBuffer.AlphaSaturation = 0.0f;
  tempBuffer.Attenuation = 0.0f;

  vec4 localSun = inverse(m_DistantClouds[i].Transform()) * vec4(sunDir, 0);

  tempBuffer.localSun = localSun;
  tempBuffer.s = vec4(sunDir, 0);
  tempBuffer.model = m_DistantClouds[i].Transform();
  tempBuffer.mvp = vp * m_DistantClouds[i].Transform();
  tempBuffer.offsetScale = offsetScaleToLocalKM;
  tempBuffer.StepSize = 0.0f;

  /*
  pRenderer->setTexture("transmittanceSampler", Transmittance, linearClamp);
  pRenderer->setTexture("irradianceSampler", Irradiance, linearClamp);
  pRenderer->setTexture("inscatterSampler", Inscatter, linearClamp);
  pRenderer->setTexture("base", m_tDistantCloud, linearClamp);
  pRenderer->setTexture("SunShafts", shaftsMask, linearClamp);

  pRenderer->setSamplerState("linearClamp", linearClamp);

  pRenderer->setShaderConstant1f("exposure", exposure);
  pRenderer->setShaderConstant3f("camPos", localCamPosKM);
  pRenderer->setShaderConstant4f("offsetScale", offsetScaleToLocalKM);
  pRenderer->setShaderConstant3f("s", sunDir);
  pRenderer->setShaderConstant2f("inscatterParams", inscatterParams);
  pRenderer->setShaderConstant1f("StepSize", m_Params.fStepSize);
  pRenderer->setShaderConstant1f("Attenuation", m_Params.fAttenuation);
  pRenderer->setShaderConstant1f("AlphaSaturation", m_Params.fAlphaSaturation);
  pRenderer->setShaderConstant1f("DirectLightIntensity", m_Params.fStratusSunIntensity);
  pRenderer->setShaderConstant1f("AmbientLightIntensity", m_Params.fStratusAmbientIntensity);

  //	Instance data!!!
  pRenderer->setShaderConstant4x4f("mvp", vp*m_DistantClouds[i].Transform());
  pRenderer->setShaderConstant4x4f("model", m_DistantClouds[i].Transform());
  mat4 invTransform = !m_DistantClouds[i].Transform();
 
  pRenderer->setShaderConstant3f("localSun", normalize(localSun.xyz()));
  */ 
  endUpdateResource(&BufferUniformSettingDesc, NULL);


  //	TODO: Igor: fix it
  //	Have to be here since we change rt in m_Impostors[cumulusID].setupRenderer
  //pRenderer->setTexture("base", m_CumulusClouds[cumulusID].Texture(), linearClamp);	

  cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
  cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mWidth, (float)pSkyRenderTarget->mHeight, 0.0f, 1.0f);
  cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mWidth, pSkyRenderTarget->mHeight);

  DescriptorData preImpParams[4] = {};

  preImpParams[0].pName = "RenderSkyUniformBuffer";
  preImpParams[0].ppBuffers = &pRenderSkyUniformBuffer;

  preImpParams[1].pName = "DistantUniformBuffer";
  preImpParams[1].ppBuffers = &pDistantUniformBuffer[gFrameIndex];

  preImpParams[2].pName = "linearClamp";
  preImpParams[2].ppSamplers = &linearClamp;

  preImpParams[3].pName = "base";
  preImpParams[3].ppTextures = &m_tCumulusCloud;

  // #TODO
  //cmdBindDescriptors(cmd, pClCumulusCloudDescriptorBinder, pClCumulusCloudRootSignature, 4, preImpParams);
  //cmdBindVertexBuffer(cmd, 1, &pGlobalTriangularVertexBuffer, NULL);

  cmdDraw(cmd, 4, 0);

  cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);


}

void CloudsManager::renderCumulusCloud(Cmd *cmd, Texture* Transmittance, Texture* Irradiance, Texture* Inscatter, Texture* shaftsMask, float exposure,
 const vec3 & localCamPosKM, const vec4 &offsetScaleToLocalKM, vec3 & sunDir, vec2 inscatterParams, size_t i, const mat4 & vp, float cloudOpacity,
 const vec3& camDir, vec4 QNnear, const Texture* rtDepth, bool bSoftClouds )
{
  /*
	////////////////////////////////////////
	//	Draw impostors
	pRenderer->reset();
	pRenderer->setRasterizerState(noCull);
	pRenderer->setDepthState(noDepthWrite);

	// PSS: until we fix impostor clipping...
	if (!m_Params.bDepthTestClouds || bSoftClouds)
	{
		pRenderer->setDepthState(noDepthAtAll);
	}

	pRenderer->setBlendState(alphaBlend);
	//m_pRenderer->setBlendState(oneAlphaBlend);

#ifdef	USE_CLOUDS_DEPTH_RECONSTRUCTION
	pRenderer->setShader(m_shImpostorCloud);
#else	//	USE_CLOUDS_DEPTH_RECONSTRUCTION
	pRenderer->setShader(m_shDistantCloud);
#endif	//	USE_CLOUDS_DEPTH_RECONSTRUCTION

	pRenderer->setTexture("transmittanceSampler", Transmittance, linearClamp);
	pRenderer->setTexture("irradianceSampler", Irradiance, linearClamp);
	pRenderer->setTexture("inscatterSampler", Inscatter, linearClamp);
	pRenderer->setTexture("SunShafts", shaftsMask, linearClamp);

	pRenderer->setSamplerState("linearClamp", linearClamp);

	pRenderer->setShaderConstant1f("exposure", exposure);
	pRenderer->setShaderConstant3f("camPos", localCamPosKM);
	pRenderer->setShaderConstant4f("offsetScale", offsetScaleToLocalKM);
	pRenderer->setShaderConstant3f("s", sunDir);
	pRenderer->setShaderConstant2f("inscatterParams", inscatterParams);
	pRenderer->setShaderConstant1f("StepSize", m_Params.fStepSize);
	pRenderer->setShaderConstant1f("Attenuation", m_Params.fAttenuation);
	pRenderer->setShaderConstant1f("AlphaSaturation", m_Params.fCumulusAlphaSaturation);
	pRenderer->setShaderConstant1f("CloudOpacity", cloudOpacity);
	pRenderer->setShaderConstant1f("DirectLightIntensity", m_Params.fCumulusSunIntensity);
	pRenderer->setShaderConstant1f("AmbientLightIntensity", m_Params.fCumulusAmbientIntensity);
	pRenderer->setShaderConstant1f("SaturationFactor", m_Params.fCloudsSaturation);
	pRenderer->setShaderConstant1f("ContrastFactor", m_Params.fCloudsContrast);

	if (bSoftClouds)
	{
		pRenderer->setTexture("Depth", rtDepth, linearClamp);
		pRenderer->setShaderConstant4f("QNNear", QNnear);
		pRenderer->setShaderConstant3f("camDir", camDir);
	}

	//	Instance data
	pRenderer->setTexture("base", m_Impostors[i].getImpostorTexture(), linearClamp);
	pRenderer->setShaderConstant4x4f("mvp", vp*m_Impostors[i].Transform());
	pRenderer->setShaderConstant4x4f("model", m_Impostors[i].Transform());
	mat4 invTransform = !m_Impostors[i].Transform();
	vec3 localSun = (invTransform*vec4(sunDir, 0)).xyz();
	localSun = normalize(localSun);
#ifdef	CLAMP_IMPOSTOR_PROJ
	const float4 & ClampWindow = m_Impostors[i].ClampWindow();
	localSun.x /= ClampWindow.y-ClampWindow.x;
	localSun.z /= ClampWindow.w-ClampWindow.z;
#endif	//	CLAMP_IMPOSTOR_PROJ

	pRenderer->setShaderConstant3f("localSun", localSun);
#ifdef	USE_CLOUDS_DEPTH_RECONSTRUCTION
	pRenderer->setShaderConstant2f("UnpackDepthParams", m_Impostors[i].UnpackDepthParams());
#endif	//	USE_CLOUDS_DEPTH_RECONSTRUCTION
#ifdef	CLAMP_IMPOSTOR_PROJ
	pRenderer->setShaderConstant4f("ClampWindow", m_Impostors[i].ClampWindow());
#endif	//	CLAMP_IMPOSTOR_PROJ

	pRenderer->apply();

	//m_pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	//m_pDevice->Draw(4, 0);
	pRenderer->drawArrays(PRIM_TRIANGLE_STRIP, 0, 4);
*/

  LoadActionsDesc loadActions = {};
  loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
  //loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
  loadActions.mClearColorValues[0].r = 0.0f;
  loadActions.mClearColorValues[0].g = 0.0f;
  loadActions.mClearColorValues[0].b = 0.0f;
  loadActions.mClearColorValues[0].a = 0.0f;
  //loadActions.mClearDepth = pDepthBuffer->mClearValue;

  cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
  cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mWidth, (float)pSkyRenderTarget->mHeight, 0.0f, 1.0f);
  cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mWidth, pSkyRenderTarget->mHeight);

  cmdBindPipeline(cmd, pDistantCloudPipeline);

  BufferUpdateDesc BufferUniformSettingDesc = { pImposterUniformBuffer[gFrameIndex] };
  beginUpdateResource(&BufferUniformSettingDesc);
  ImposterUniformBuffer& tempBuffer = *(ImposterUniformBuffer*)BufferUniformSettingDesc.pMappedData;
  tempBuffer.AlphaSaturation = 0.0f;
  tempBuffer.Attenuation = 0.0f;

  vec4 localSun = inverse(m_DistantClouds[i].Transform()) * vec4(sunDir, 0);

  tempBuffer.localSun = localSun.getXYZ();
  tempBuffer.model = m_Impostors[i].Transform();
  tempBuffer.mvp = vp * m_Impostors[i].Transform();
  tempBuffer.offsetScale = offsetScaleToLocalKM;
  tempBuffer.SaturationFactor = 1.0f;
  endUpdateResource(&BufferUniformSettingDesc, NULL);


  //	TODO: Igor: fix it
  //	Have to be here since we change rt in m_Impostors[cumulusID].setupRenderer
  //pRenderer->setTexture("base", m_CumulusClouds[cumulusID].Texture(), linearClamp);	

  RenderTargetBarrier barriersSky[] = {
          { m_Impostors[i].getImpostorTexture(), RESOURCE_STATE_SHADER_RESOURCE }
  };

  cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersSky);

  DescriptorData preImpParams[4] = {};

  preImpParams[0].pName = "RenderSkyUniformBuffer";
  preImpParams[0].ppBuffers = &pRenderSkyUniformBuffer;

  preImpParams[1].pName = "ImposterUniform";
  preImpParams[1].ppBuffers = &pImposterUniformBuffer[gFrameIndex];

  preImpParams[2].pName = "linearClamp";
  preImpParams[2].ppSamplers = &linearClamp;

  preImpParams[3].pName = "base";
  preImpParams[3].ppTextures = &m_Impostors[i].getImpostorTexture()->pTexture;

  // #TODO
  //cmdBindDescriptors(cmd, pClCumulusCloudDescriptorBinder, pClCumulusCloudRootSignature, 4, preImpParams);
  //cmdBindVertexBuffer(cmd, 1, &pGlobalTriangularVertexBuffer, NULL);

  cmdDraw(cmd, 4, 0);

  cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);
}

void CloudsManager::setParams( const CloudsParams& params )
{
	m_Params = params;
}

void CloudsManager::getParams( CloudsParams& params )
{
	params = m_Params;
}

void CloudsManager::clipClouds( const vec3 & camPos )
{
	clipCumulusClouds(camPos);
}

void CloudsManager::clipCumulusClouds( const vec3 & camPos )
{
	for (unsigned int i=0; i<m_CumulusClouds.size(); ++i)
	{
		m_CumulusClouds[i].clipCloud(camPos, m_Params.fCumulusExistanceR);
	}
}

CloudHandle CloudsManager::createDistantCloud( const mat4 & transform, Texture* texID )
{
	//assert(sizeof(CloudHandle)==sizeof(CloudDescriptor));

	if (texID == NULL)
		texID = m_tDistantCloud;

	uint32 cloudIndex = m_DistantCloudsHandles.GetFreeIndex();

	CloudDescriptor cloud;
	cloud.handle = 0;
	cloud.bCumulusCloud = false;
	cloud.uiCloudID = cloudIndex;

	//	Check if there's enough bits for the index
	//assert(cloud.uiCloudID == cloudIndex);

	if (cloudIndex<m_DistantClouds.size())
	{
		m_DistantClouds[cloudIndex] = DistantCloud(transform, texID);
	}
	else
	{
		m_DistantClouds.push_back(DistantCloud(transform, texID));
	}

	m_SortedClouds.push_back(CloudSortData());
	m_SortedClouds.back().type = CloudSortData::CT_Distant;
	m_SortedClouds.back().index = cloudIndex;

	return cloud.handle;
}

CloudHandle CloudsManager::createCumulusCloud( const mat4 & transform, Texture* texID, float particleScale,
 vec4 * particleOffsetScale, ParticleProps * particleProps, uint32 particleCount, bool centerParticles/*=true*/ )
{
	//assert(sizeof(CloudHandle)==sizeof(CloudDescriptor));

	if (texID == NULL)
		texID = m_tCumulusCloud;

	uint32 cloudIndex = m_CumulusCloudsHandles.GetFreeIndex();

	CloudDescriptor cloud;
	cloud.handle = 0;
	cloud.bCumulusCloud = true;
	cloud.uiCloudID = cloudIndex;

	//	Check if there's enough bits for the index
	//assert(cloud.uiCloudID == cloudIndex);

	if (cloudIndex<m_CumulusClouds.size())
	{
		m_CumulusClouds[cloudIndex] = CumulusCloud(transform, texID, particleScale);
	}
	else
	{
		//For RTU
		m_CumulusClouds.push_back(CumulusCloud(transform, texID, particleScale));
		m_Impostors.push_back(CloudImpostor());
	}

	m_CumulusClouds[cloudIndex].setParticles(particleOffsetScale, particleProps, particleCount);

// if we do this it will wiggle when new points are added, the points are already generated in
// a distribution that is centered on the origin
	if (centerParticles)
		m_CumulusClouds[cloudIndex].centerParticles();

	m_Impostors[cloudIndex].InitFromCloud(&m_CumulusClouds[cloudIndex]);

	m_SortedClouds.push_back(CloudSortData());
	m_SortedClouds.back().type = CloudSortData::CT_Cumulus;
	m_SortedClouds.back().index = cloudIndex;

	return cloud.handle;
}

void CloudsManager::removeCloud( CloudHandle handle )
{
	//assert(sizeof(CloudHandle)==sizeof(CloudDescriptor));

	CloudDescriptor cloud;
	cloud.handle = handle;

	size_t cloudsCount = m_SortedClouds.size();
	CloudSortData data;
	data.type = cloud.bCumulusCloud ? CloudSortData::CT_Cumulus : CloudSortData::CT_Distant;
	size_t i=0;

	for (; i<cloudsCount; ++i)
	{
		if ( (m_SortedClouds[i].type == data.type) && (m_SortedClouds[i].index == cloud.uiCloudID) )
		{
			m_SortedClouds.erase(m_SortedClouds.begin()+i);
			break;
		}
	}

	if (cloud.bCumulusCloud)
		m_CumulusCloudsHandles.ReleaseIndex(cloud.uiCloudID);
	else
		m_DistantCloudsHandles.ReleaseIndex(cloud.uiCloudID);

	//assert(i<cloudsCount);
}

void CloudsManager::setCloudTramsform( CloudHandle handle, const mat4 & transform )
{
	//assert(sizeof(CloudHandle)==sizeof(CloudDescriptor));

	CloudDescriptor cloud;
	cloud.handle = handle;

	if (cloud.bCumulusCloud)
	{
		//assert(cloud.uiCloudID<m_CumulusClouds.size());
		m_CumulusClouds[cloud.uiCloudID].setTransform(transform);
	}
	else
	{
		//assert(cloud.uiCloudID<m_DistantClouds.size());
		m_DistantClouds[cloud.uiCloudID].setTransform(transform);
	}
}
