/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "VolumetricClouds.h"

#include "../../../../The-Forge/Common_3/Renderer/IResourceLoader.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"

#define TERRAIN_NEAR            50.0f
#define TERRAIN_FAR             100000000.0f
#define _CLOUDS_LAYER_START     15000.0f                                        // The height where the clouds' layer get started
#define _CLOUDS_LAYER_THICKNESS 20000.0f                                        // The height where the clouds' layer covers
#define _CLOUDS_LAYER_END       (_CLOUDS_LAYER_START + _CLOUDS_LAYER_THICKNESS) // The height where the clouds' layer get ended (End = Start + Thickness)
#define	DRAW_SHADOW             0
#define	USE_RP_FRAGMENTSHADER   1
#define	USE_DEPTH_CULLING       1
#define	USE_LOD_DEPTH           1
#define USE_VC_FRAGMENTSHADER   0                                               // 0: compute shaders 1: fragment shaders

static uint32_t						DownSampling = 1;
static uint								prevDownSampling = 1;

const uint32_t            glowResBufferSize = 4;
const uint32_t            godRayBufferSize = 8;

Texture*                  pHiZDepthBuffer = NULL;
Texture*                  pHiZDepthBuffer2 = NULL;

Texture*                  pHiZDepthBufferX = NULL;

RenderTarget*             plowResCloudRT = NULL;
Texture*                  plowResCloudTexture = NULL;
RenderTarget*             phighResCloudRT = NULL;
Texture*                  phighResCloudTexture = NULL;

Buffer*                   pTriangularScreenVertexBuffer = NULL;
Buffer*                   pTriangularScreenVertexWithMiscBuffer = NULL;

RenderTarget*             pPostProcessRT = NULL;
RenderTarget*             pRenderTargetsPostProcess[2] = { nullptr };

Shader*                   pGenHiZMipmapShader = NULL;
Pipeline*                 pGenHiZMipmapPipeline = NULL;
RootSignature*            pGenHiZMipmapRootSignature = NULL;

Shader*                   pCopyTextureShader = NULL;
Pipeline*                 pCopyTexturePipeline = NULL;
RootSignature*            pCopyTextureRootSignature = NULL;

//Shader*                   pCopyWeatherTextureShader = NULL;
//Pipeline*                 pCopyWeatherTexturePipeline = NULL;
//RootSignature*            pCopyWeatherTextureRootSignature = NULL;

// Draw stage

// Graphics
Shader*                   pVolumetricCloudShader = NULL;
Pipeline*                 pVolumetricCloudPipeline = NULL;

Shader*                   pPostProcessShader = NULL;
Pipeline*                 pPostProcessPipeline = NULL;

Shader*                   pPostProcessWithBlurShader = NULL;
Pipeline*                 pPostProcessWithBlurPipeline = NULL;

Shader*                   pGodrayShader = NULL;
Pipeline*                 pGodrayPipeline = NULL;

Shader*                   pGodrayAddShader = NULL;
Pipeline*                 pGodrayAddPipeline = NULL;

/*
Shader*                   pCastShadowShader = NULL;
Pipeline*                 pCastShadowPipeline = NULL;
RootSignature*            pCastShadowRootSignature = NULL;
DescriptorBinder*         pCastShadowDescriptorBinder = NULL;
*/

Shader*                   pCompositeShader = NULL;
Pipeline*                 pCompositePipeline = NULL;

Shader*                   pCompositeOverlayShader = NULL;
Pipeline*                 pCompositeOverlayPipeline = NULL;

RootSignature*            pVolumetricCloudsRootSignatureGraphics = NULL;
DescriptorSet*            pVolumetricCloudsDescriptorSetGraphics[2] = { NULL };

#if USE_VC_FRAGMENTSHADER
/*********Volumetric cloud generation fragment shaders pipeline**********/
Shader*                   pVolumetricCloudWithDepthShader = NULL;
Pipeline*                 pVolumetricCloudWithDepthPipeline = NULL;

Shader*                   pVolumetricCloud2ndShader = NULL;
Pipeline*                 pVolumetricCloud2ndPipeline = NULL;

Shader*                   pVolumetricCloud2ndWithDepthShader = NULL;
Pipeline*                 pVolumetricCloud2ndWithDepthPipeline = NULL;
/************************************************************************/
#else
/**********Volumetric cloud generation compute shaders pipeline**********/
Shader*                   pVolumetricCloudCompShader = NULL;
Pipeline*                 pVolumetricCloudCompPipeline = NULL;

Shader*                   pVolumetricCloud2ndCompShader = NULL;
Pipeline*                 pVolumetricCloud2ndCompPipeline = NULL;

Shader*                   pVolumetricCloudWithDepthCompShader = NULL;
Pipeline*                 pVolumetricCloudWithDepthCompPipeline = NULL;

Shader*                   pVolumetricCloud2ndWithDepthCompShader = NULL;
Pipeline*                 pVolumetricCloud2ndWithDepthCompPipeline = NULL;
/*************************************************************************/
#endif

// Comp
Shader*                   pGenHiZMipmapPRShader = NULL;
Pipeline*                 pGenHiZMipmapPRPipeline = NULL;

Shader*                   pReprojectionShader = NULL;
Pipeline*                 pReprojectionPipeline = NULL;

Shader*                   pCopyRTShader = NULL;
Pipeline*                 pCopyRTPipeline = NULL;

Shader*                   pHorizontalBlurShader = NULL;
Pipeline*                 pHorizontalBlurPipeline = NULL;

Shader*                   pVerticalBlurShader = NULL;
Pipeline*                 pVerticalBlurPipeline = NULL;

RootSignature*            pVolumetricCloudsRootSignatureCompute = NULL;
DescriptorSet*            pVolumetricCloudsDescriptorSetCompute[2] = { NULL };

/*
Shader*                   pReprojectionCompShader = NULL;
Pipeline*                 pReprojectionCompPipeline = NULL;
RootSignature*            pReprojectionCompRootSignature = NULL;
DescriptorBinder*         pReprojectionCompDescriptorBinder = NULL;
*/

Texture*                  pHBlurTex;
Texture*                  pVBlurTex;

RenderTarget*             pGodrayRT = NULL;

DescriptorSet*            pVolumetricCloudsDescriptorSet = NULL;

Sampler*                  pBilinearSampler = NULL;
Sampler*                  pPointSampler = NULL;

Sampler*                  pBiClampSampler = NULL;
Sampler*                  pPointClampSampler = NULL;

Sampler*                  pLinearBorderSampler = NULL;

const uint32_t            gHighFreq3DTextureSize = 32;
const uint32_t            gLowFreq3DTextureSize = 128;

Texture*                  gHighFrequencyOriginTextureStorage = NULL;
Texture*                  gLowFrequencyOriginTextureStorage = NULL;

eastl::vector<eastl::string> gHighFrequencyTextureNames;
eastl::vector<eastl::string> gLowFrequencyTextureNames;

eastl::vector<Texture*>		gHighFrequencyOriginTexture;
eastl::vector<Texture*>		gLowFrequencyOriginTexture;

eastl::vector<Texture*>		gHighFrequencyOriginTexturePacked;
eastl::vector<Texture*>		gLowFrequencyOriginTexturePacked;

Texture*                  pHighFrequency3DTextureOrigin;
Texture*                  pLowFrequency3DTextureOrigin;

Texture*                  pHighFrequency3DTexture;
Texture*                  pLowFrequency3DTexture;

Texture*                  pWeatherTexture;
Texture*                  pWeatherCompactTexture;
Texture*                  pCurlNoiseTexture;

const uint32_t		HighFreqDimension = 32;
const uint32_t		LowFreqDimension  = 128;
const uint32_t		HighFreqMipCount  = (uint32_t)log2(HighFreqDimension) + 1;
const uint32_t		LowFreqMipCount   = (uint32_t)log2(LowFreqDimension)  + 1;

const uint32_t      gTriangleVbStride = sizeof(float) * 5;

static TextureDesc HiZDepthDesc = {};
static TextureDesc lowResTextureDesc = {};
static TextureDesc blurTextureDesc = {};

static mat4				prevView;
static mat4				projMat;

static float4			g_ProjectionExtents;
static float			g_currentTime = 0.0f;
typedef struct AppSettings
{
	bool				m_Enabled = true;

	float				m_EarthRadiusScale = 10.0f;
	float				m_MaxSampleDistance = 200000.0f;

	float				m_CloudsLayerStart = 15000.0f;
	float				m_CloudsLayerStart_2nd = 66100.0f;

	// VolumetricClouds raymarching
	uint32_t		m_MinSampleCount = 64;
	uint32_t		m_MaxSampleCount = 192;
	float				m_MinStepSize = 256.0;
	float				m_MaxStepSize = 1024.0;


	float				m_LayerThickness = 178000.0f;
	float				m_LayerThickness_2nd = 90900.0f;

	bool				m_EnabledTemporalRayOffset = false;

	// VolumetricClouds modeling
	float				m_BaseTile = 0.621f;

	float				m_DetailTile = 5.496f;
	float				m_DetailStrength = 0.25f;
	float				m_CurlTile = 0.1f;
	float				m_CurlStrength = 2000.0f;
	float				m_CloudTopOffset = 500.0f;
	float				m_CloudSize = 103305.805f;

	float				m_CloudDensity = 3.0f;
	float				m_CloudCoverageModifier = 0.0f;

	float				m_CloudTypeModifier = 0.0f;
	float				m_AnvilBias = 1.0f;
	float				m_WeatherTexSize = 867770.0f;

	float				WeatherTextureAzimuth = 0.0f;
	float				WeatherTextureDistance = 0.0f;

	// Wind factors
	float				m_WindAzimuth = 0.0f;
	float				m_WindIntensity = 20.0f;

	bool				m_EnabledRotation = false;

	float				m_RotationPivotAzimuth = 0.0f;
	float				m_RotationPivotDistance = 0.0f;
	float				m_RotationIntensity = 1.0f;
	float				m_RisingVaporScale = 1.0f;
	float				m_RisingVaporUpDirection = -1.0f;
	float				m_RisingVaporIntensity = 5.0f;

	float				m_NoiseFlowAzimuth = 180.0f;
	float				m_NoiseFlowIntensity = 7.5f;
	

	bool				m_Enabled2ndLayer = false;

	float				m_BaseTile_2nd = 0.455f;

	float				m_DetailTile_2nd = 3.141f;
	float				m_DetailStrength_2nd = 0.298f;
	float				m_CurlTile_2nd = 0.1f;
	float				m_CurlStrength_2nd = 2000.0f;
	float				m_CloudTopOffset_2nd = 500.0f;
	float				m_CloudSize_2nd = 57438.0f;

	float				m_CloudDensity_2nd = 3.0f;
	float				m_CloudCoverageModifier_2nd = 0.0f;

	float				m_CloudTypeModifier_2nd = 0.521f;
	float				m_AnvilBias_2nd = 1.0f;
	float				m_WeatherTexSize_2nd = 1000000.0f;

	float				WeatherTextureAzimuth_2nd = 0.0f;
	float				WeatherTextureDistance_2nd = 0.0f;

	// Wind factors
	float				m_WindAzimuth_2nd = 180.0f;
	float				m_WindIntensity_2nd = 0.0f;

	bool				m_EnabledRotation_2nd = true;

	float				m_RotationPivotAzimuth_2nd = 0.0f;
	float				m_RotationPivotDistance_2nd = 0.0f;
	float				m_RotationIntensity_2nd = 1.0f;
	float				m_RisingVaporScale_2nd = 0.5f;
	float				m_RisingVaporUpDirection_2nd = 0.0f;
	float				m_RisingVaporIntensity_2nd = 1.0f;

	float				m_NoiseFlowAzimuth_2nd = 180.0f;
	float				m_NoiseFlowIntensity_2nd = 10.0f;

	// VolumetricClouds lighting	
	uint32_t		m_CustomColor = 0xFFFFFFFF;

	float				m_CustomColorIntensity = 1.0f;
	float				m_CustomColorBlendFactor = 0.0f;
	float				m_Contrast = 1.3f;
	float				m_TransStepSize = 768.0f * 2.0f;

	float				m_BackgroundBlendFactor = 1.0f;
	float				m_Precipitation = 1.0f;
	float				m_SilverIntensity = 0.175f;
	float				m_SilverSpread = 0.29f;

	float				m_Eccentricity = 0.65f;
	float				m_CloudBrightness = 1.15f;
	//Culling
	bool				m_EnabledDepthCulling = true;
	bool				m_EnabledLodDepth = true;
	//Shadow

	bool				m_EnabledShadow = true;


	float				m_ShadowBrightness = 0.5f;
	float				m_ShadowTiling = 20.0f;
	float				m_ShadowSpeed = 1.0f;

	// VolumetricClouds' Light shaft

	bool				m_EnabledGodray = true;
	uint32_t		m_GodNumSamples = 80;
	float				m_Exposure = 0.0025f;
	float				m_Decay = 0.995f;
	float				m_Density = 0.3f;
	float				m_Weight = 0.85f;

	float				m_Test00 = 0.5f;
	float				m_Test01 = 0.0f;
	float				m_Test02 = 0.23f;
	float				m_Test03 = 0.0f;

#if defined(USE_CHEAP_SETTINGS)
	bool				m_EnableBlur = true;
#else
	bool				m_EnableBlur = false;
#endif
} AppSettings;

AppSettings	gAppSettings;

float4 calcSkyBetaR(float sunlocationY, float rayleigh)
{
	float3 totalRayleigh = float3(5.804542996261093E-6f, 1.3562911419845635E-5f, 3.0265902468824876E-5f);

	float sunFade = 1.0f - clamp(1.0f - exp(sunlocationY), 0.0f, 1.0f);
	return float4(totalRayleigh * (rayleigh - 1.f + sunFade), sunFade);
}

float4 calcSkyBetaV(float turbidity)
{
	float3 MIE_CONST = float3(1.8399918514433978E14f, 2.7798023919660528E14f, 4.0790479543861094E14f);
	float c = (0.2f * turbidity) * 10E-18f;
	return float4(0.434f * c * MIE_CONST, 0.0f);
}

//static float elapsedTime = 0.0f;
static uint g_LowResFrameIndex = 0;
//static uint haltonSequenceIndex = 0;
//static uint gFrameIndex = 0;

const uint32_t		GImageCount = 3;

Buffer* VolumetricCloudsCBuffer[GImageCount];

static int2 offset[] = {
					{2,1}, {1,2}, {2,0}, {0,1},
					{2,3}, {3,2}, {3,1}, {0,3},
					{1,0}, {1,1}, {3,3}, {0,0},
					{2,2}, {1,3}, {3,0}, {0,2}
};

//static int4 bayerOffsets[] =
//{
//	{0,8,2,10 },
//	{12,4,14,6 },
//	{3,11,1,9 },
//	{15,7,13,5 }
//};

/*
static int haltonSequence[] =
{
	8, 4, 12, 2, 10, 6, 14, 1
};
*/

static void reverse_str(char *s, int len)
{
	int i, j;
	char c;

	for (i = 0, j = len - 1; i < j; ++i, --j)
	{
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

static char* _itoa(int n, char *s)
{
	int i = 0, sign = n;

	do {
		s[i++] = (char)(abs(n % 10) + '0');
	} while ((n /= 10) != 0);

	if (sign < 0)
		s[i++] = '-';

	s[i] = '\0';
	reverse_str(s, i);
	return s;
}

bool VolumetricClouds::Init(Renderer* renderer, PipelineCache* pCache)
{
	pRenderer = renderer;
	pPipelineCache = pCache;

	g_StandardPosition			= vec4(0.0f, 0.0f, 0.0f, 0.0f);
	g_StandardPosition_2nd	= vec4(0.0f, 0.0f, 0.0f, 0.0f);
	g_ShadowInfo						= vec4(0.0f, 0.0f, 1.0f, 0.0f);
	srand((uint)time(NULL));	

	SamplerDesc samplerDesc = {
		FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
		ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT
	};
	addSampler(pRenderer, &samplerDesc, &pBilinearSampler);


	SamplerDesc PointSamplerDesc = {
		FILTER_NEAREST, FILTER_NEAREST, MIPMAP_MODE_LINEAR,
		ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT
	};
	addSampler(pRenderer, &PointSamplerDesc, &pPointSampler);

	SamplerDesc samplerClampDesc = {
		FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
		ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE
	};
	addSampler(pRenderer, &samplerClampDesc, &pBiClampSampler);


	SamplerDesc PointSamplerClampDesc = {
		FILTER_NEAREST, FILTER_NEAREST, MIPMAP_MODE_LINEAR,
		ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE
	};
	addSampler(pRenderer, &PointSamplerClampDesc, &pPointClampSampler);

	SamplerDesc LinearBorderSamplerClampDesc = {
		FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
		ADDRESS_MODE_CLAMP_TO_BORDER, ADDRESS_MODE_CLAMP_TO_BORDER, ADDRESS_MODE_CLAMP_TO_BORDER
	};
	addSampler(pRenderer, &LinearBorderSamplerClampDesc, &pLinearBorderSampler);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc basicShader = {};
	basicShader.mStages[0] = { "VolumetricCloud.vert", NULL, 0, NULL};
	basicShader.mStages[1] = { "PostProcess.frag", NULL, 0, NULL};
	addShader(pRenderer, &basicShader, &pPostProcessShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc postBlurShader = {};
	postBlurShader.mStages[0] = { "VolumetricCloud.vert", NULL, 0, NULL };
	postBlurShader.mStages[1] = { "PostProcessWithBlur.frag", NULL, 0, NULL };
	addShader(pRenderer, &postBlurShader, &pPostProcessWithBlurShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc reprojectionShader = {};
	reprojectionShader.mStages[0] = { "VolumetricCloud.vert", NULL, 0, NULL };
	reprojectionShader.mStages[1] = { "Reprojection.frag", NULL, 0, NULL };
	addShader(pRenderer, &reprojectionShader, &pReprojectionShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc godrayShader = {};
	godrayShader.mStages[0] = { "Triangular.vert", NULL, 0, NULL };
	godrayShader.mStages[1] = { "Godray.frag", NULL, 0, NULL };
	addShader(pRenderer, &godrayShader, &pGodrayShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc godrayAddShader = {};
	godrayAddShader.mStages[0] = { "Triangular.vert", NULL, 0, NULL };
	godrayAddShader.mStages[1] = { "GodrayAdd.frag", NULL, 0, NULL };
	addShader(pRenderer, &godrayAddShader, &pGodrayAddShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	ShaderLoadDesc castShadowShader = {};
	ShaderPath(shaderPath, (char*)"basic.vert", shaderFullPath);
	castShadowShader.mStages[0] = { shaderFullPath.c_str(), NULL, 0, FSR_SrcShaders };
	ShaderPath(shaderPath, (char*)"castShadow.frag", shaderFullPath);
	castShadowShader.mStages[1] = { shaderFullPath.c_str(), NULL, 0, FSR_SrcShaders };

	addShader(pRenderer, &castShadowShader, &pCastShadowShader);

	rootDesc = { 0 };
	rootDesc.mShaderCount = 1;
	rootDesc.ppShaders = &pCastShadowShader;

	addRootSignature(pRenderer, &rootDesc, &pCastShadowRootSignature);
	DescriptorBinderDesc CastShadowDescriptorBinderDesc[1] = { { pCastShadowRootSignature } };
	addDescriptorBinder(pRenderer, 0, 1, CastShadowDescriptorBinderDesc, &pCastShadowDescriptorBinder);
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc compositeShader = {};
	compositeShader.mStages[0] = { "VolumetricCloud.vert", NULL, 0, NULL };
	compositeShader.mStages[1] = { "Composite.frag", NULL, 0, NULL };
	addShader(pRenderer, &compositeShader, &pCompositeShader);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc compositeOverlayShader = {};
	compositeOverlayShader.mStages[0] = { "VolumetricCloud.vert", NULL, 0, NULL };
	compositeOverlayShader.mStages[1] = { "CompositeOverlay.frag", NULL, 0, NULL };
	addShader(pRenderer, &compositeOverlayShader, &pCompositeOverlayShader);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc GenHiZMipmapShader = {};
	GenHiZMipmapShader.mStages[0] = { "HiZdownSampling.comp", NULL, 0,NULL };
	addShader(pRenderer, &GenHiZMipmapShader, &pGenHiZMipmapShader);

	Shader* GenHiZMipmapShaders[] = { pGenHiZMipmapShader };

	RootSignatureDesc rootGenHiZMipmapDesc = {};
	rootGenHiZMipmapDesc.mShaderCount = 1;
	rootGenHiZMipmapDesc.ppShaders = GenHiZMipmapShaders;

	addRootSignature(pRenderer, &rootGenHiZMipmapDesc, &pGenHiZMipmapRootSignature);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc GenHiZMipmapPRShader = {};
	GenHiZMipmapPRShader.mStages[0] = { "HiZdownSamplingPR.comp", NULL, 0, NULL };
	addShader(pRenderer, &GenHiZMipmapPRShader, &pGenHiZMipmapPRShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc CopyTextureShader = {};
	CopyTextureShader.mStages[0] = { "CopyTexture.comp", NULL, 0, NULL};
	addShader(pRenderer, &CopyTextureShader, &pCopyTextureShader);

	Shader* CopyTextureShaders[] = { pCopyTextureShader };

	RootSignatureDesc rootCopyTextureDesc = {};
	rootCopyTextureDesc.mShaderCount = 1;
	rootCopyTextureDesc.ppShaders = CopyTextureShaders;

	addRootSignature(pRenderer, &rootCopyTextureDesc, &pCopyTextureRootSignature);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*ShaderLoadDesc CopyWeatherTextureShader = {};
	CopyWeatherTextureShader.mStages[0] = { "VolumetricClouds/copyWeatherTexture.comp", NULL, 0, NULL };
	addShader(pRenderer, &CopyWeatherTextureShader, &pCopyWeatherTextureShader);

	Shader* CopyWeatherTextureShaders[] = { pCopyWeatherTextureShader };

	RootSignatureDesc rootCopyWeatherTextureDesc = {};
	rootCopyWeatherTextureDesc.mShaderCount = 1;
	rootCopyWeatherTextureDesc.ppShaders = CopyWeatherTextureShaders;

	addRootSignature(pRenderer, &rootCopyWeatherTextureDesc, &pCopyWeatherTextureRootSignature);*/

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc BlurShader = {};
	BlurShader.mStages[0] = { "BlurHorizontal.comp", NULL, 0, NULL };
	addShader(pRenderer, &BlurShader, &pHorizontalBlurShader);

	BlurShader.mStages[0] = { "BlurVertical.comp", NULL, 0, NULL };
	addShader(pRenderer, &BlurShader, &pVerticalBlurShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc CopyRTShader = {};
	CopyRTShader.mStages[0] = { "CopyRT.comp", NULL, 0, NULL };
	addShader(pRenderer, &CopyRTShader, &pCopyRTShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc VolumetricCloudShaderDesc = {};
	VolumetricCloudShaderDesc.mStages[0] = { "VolumetricCloud.vert", NULL, 0, NULL };
	VolumetricCloudShaderDesc.mStages[1] = { "VolumetricCloud.frag", NULL, 0, NULL };
	addShader(pRenderer, &VolumetricCloudShaderDesc, &pVolumetricCloudShader);

#if USE_VC_FRAGMENTSHADER
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VolumetricCloudShaderDesc = {};
	VolumetricCloudShaderDesc.mStages[0] = { "VolumetricCloud.vert", NULL, 0, NULL };
	VolumetricCloudShaderDesc.mStages[1] = { "VolumetricCloudWithDepth.frag", NULL, 0, NULL };
	addShader(pRenderer, &VolumetricCloudShaderDesc, &pVolumetricCloudWithDepthShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VolumetricCloudShaderDesc = {};
	VolumetricCloudShaderDesc.mStages[0] = { "VolumetricCloud.vert", NULL, 0, NULL };
	VolumetricCloudShaderDesc.mStages[1] = { "VolumetricCloud_2nd.frag", NULL, 0, NULL };
	addShader(pRenderer, &VolumetricCloudShaderDesc, &pVolumetricCloud2ndShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VolumetricCloudShaderDesc = {};
	VolumetricCloudShaderDesc.mStages[0] = { "VolumetricCloud.vert", NULL, 0, NULL };
	VolumetricCloudShaderDesc.mStages[1] = { "VolumetricCloud_2ndWithDepth.frag", NULL, 0, NULL };
	addShader(pRenderer, &VolumetricCloudShaderDesc, &pVolumetricCloud2ndWithDepthShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
#else
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc VolumetricCloudCompShaderDesc = {};
	VolumetricCloudCompShaderDesc.mStages[0] = { "VolumetricCloud.comp", NULL, 0, NULL };
	addShader(pRenderer, &VolumetricCloudCompShaderDesc, &pVolumetricCloudCompShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc VolumetricCloud2ndCompShaderDesc = {};
	VolumetricCloud2ndCompShaderDesc.mStages[0] = { "VolumetricCloud_2nd.comp", NULL, 0, NULL};
	addShader(pRenderer, &VolumetricCloud2ndCompShaderDesc, &pVolumetricCloud2ndCompShader);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc VolumetricCloudWithDepthCompShaderDesc = {};
	VolumetricCloudWithDepthCompShaderDesc.mStages[0] = { "VolumetricCloudWithDepth.comp", NULL, 0, NULL};
	addShader(pRenderer, &VolumetricCloudWithDepthCompShaderDesc, &pVolumetricCloudWithDepthCompShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc VolumetricCloud2ndWithDepthCompShaderDesc = {};
	VolumetricCloud2ndWithDepthCompShaderDesc.mStages[0] = { "VolumetricCloud_2ndWithDepth.comp", NULL, 0, NULL };
	addShader(pRenderer, &VolumetricCloud2ndWithDepthCompShaderDesc, &pVolumetricCloud2ndWithDepthCompShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

	/*
	ShaderLoadDesc reprojectionCompShaderDesc = {};
	ShaderPath(shaderPath, (char*)"reprojection.comp", shaderFullPath);
	reprojectionCompShaderDesc.mStages[0] = { shaderFullPath.c_str(), NULL, 0, FSR_SrcShaders };

	addShader(pRenderer, &reprojectionCompShaderDesc, &pReprojectionCompShader);

	RootSignatureDesc rootRCDesc = {};
	rootRCDesc.mShaderCount = 1;
	rootRCDesc.ppShaders = &pReprojectionCompShader;

	addRootSignature(pRenderer, &rootRCDesc, &pReprojectionCompRootSignature);

	DescriptorBinderDesc ReprojectionCompDescriptorBinderDesc[1] = { { pReprojectionCompRootSignature } };
	addDescriptorBinder(pRenderer, 0, 1, ReprojectionCompDescriptorBinderDesc, &pReprojectionCompDescriptorBinder);
	*/

	////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	const char* pVCSamplerNames[] = { "g_LinearWrapSampler", "g_LinearClampSampler", "g_PointClampSampler", "g_LinearBorderSampler" };
	Sampler* pVCSamplers[] = { pBilinearSampler, pBiClampSampler, pPointClampSampler, pLinearBorderSampler };

#if USE_VC_FRAGMENTSHADER
	Shader* shaders[]     = { pReprojectionShader, pPostProcessShader, pPostProcessWithBlurShader, pGodrayShader, pGodrayAddShader, pCompositeShader, pCompositeOverlayShader, pVolumetricCloudShader, pVolumetricCloudWithDepthShader, pVolumetricCloud2ndShader, pVolumetricCloud2ndWithDepthShader };
	Shader* shaderComps[] = { pGenHiZMipmapPRShader, pCopyRTShader, pHorizontalBlurShader, pVerticalBlurShader };
#else
	Shader* shaders[]     = { pReprojectionShader, pPostProcessShader, pPostProcessWithBlurShader, pGodrayShader, pGodrayAddShader, pCompositeShader, pCompositeOverlayShader, pVolumetricCloudShader };
	Shader* shaderComps[] = { pGenHiZMipmapPRShader, pCopyRTShader, pHorizontalBlurShader, pVerticalBlurShader, pVolumetricCloudCompShader, pVolumetricCloud2ndCompShader, pVolumetricCloudWithDepthCompShader, pVolumetricCloud2ndWithDepthCompShader, };
#endif

	uint32_t shaderCount = sizeof(shaders) / sizeof(Shader*);

	RootSignatureDesc rootDesc = {};
	rootDesc.mShaderCount = shaderCount;
	rootDesc.ppShaders = shaders;
	rootDesc.mStaticSamplerCount = 4;
	rootDesc.ppStaticSamplerNames = pVCSamplerNames;
	rootDesc.ppStaticSamplers = pVCSamplers;
	addRootSignature(pRenderer, &rootDesc, &pVolumetricCloudsRootSignatureGraphics);

	uint32_t shaderCountComp = sizeof(shaderComps) / sizeof(Shader*);
	
	RootSignatureDesc rootCompDesc = {};
	rootCompDesc.mShaderCount = shaderCountComp;
	rootCompDesc.ppShaders = shaderComps;
	rootCompDesc.mStaticSamplerCount = 4;
	rootCompDesc.ppStaticSamplerNames = pVCSamplerNames;
	rootCompDesc.ppStaticSamplers = pVCSamplers;
	addRootSignature(pRenderer, &rootCompDesc, &pVolumetricCloudsRootSignatureCompute);

	DescriptorSetDesc setDesc = { pVolumetricCloudsRootSignatureCompute, DESCRIPTOR_UPDATE_FREQ_NONE, rootCompDesc.mShaderCount };
	addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetCompute[0]);
#if !USE_VC_FRAGMENTSHADER
	setDesc = { pVolumetricCloudsRootSignatureCompute, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
	addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetCompute[1]);
#endif
	setDesc = { pVolumetricCloudsRootSignatureGraphics, DESCRIPTOR_UPDATE_FREQ_NONE, rootDesc.mShaderCount };
	addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetGraphics[0]);
	setDesc = { pVolumetricCloudsRootSignatureGraphics, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
	addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetGraphics[1]);

	float screenQuadPoints[] = {
			-1.0f,  3.0f, 0.5f, 0.0f, -1.0f,
			-1.0f, -1.0f, 0.5f, 0.0f, 1.0f,
			3.0f, -1.0f, 0.5f, 2.0f, 1.0f,
	};

	SyncToken token = {};

	uint64_t screenQuadDataSize = 5 * 3 * sizeof(float);
	BufferLoadDesc screenQuadVbDesc = {};
	screenQuadVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	screenQuadVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	screenQuadVbDesc.mDesc.mSize = screenQuadDataSize;
	screenQuadVbDesc.pData = screenQuadPoints;
	screenQuadVbDesc.ppBuffer = &pTriangularScreenVertexBuffer;
	addResource(&screenQuadVbDesc, &token);

	gHighFrequencyOriginTexture = eastl::vector<Texture*>(gHighFreq3DTextureSize);


	eastl::string gHighFrequencyOriginTextureName("VolumetricClouds/hiResCloudShape/hiResClouds (");
	for (uint32_t i = 0; i < gHighFreq3DTextureSize; ++i)
	{
		char stringInt[4];
		_itoa(i, stringInt);

		eastl::string gHighFrequencyNameLocal = gHighFrequencyOriginTextureName;

		gHighFrequencyNameLocal += eastl::string(stringInt);
		gHighFrequencyNameLocal += eastl::string(")");

		TextureLoadDesc highFrequencyOrigin3DTextureDesc = {};
		gHighFrequencyTextureNames.push_back(gHighFrequencyNameLocal);

		highFrequencyOrigin3DTextureDesc.pFileName = gHighFrequencyTextureNames[i].c_str();
		highFrequencyOrigin3DTextureDesc.ppTexture = &gHighFrequencyOriginTexture[i];
		addResource(&highFrequencyOrigin3DTextureDesc, &token);
	}

	//////////////////////////////////////////////////////////////////////////////////////////

	gLowFrequencyOriginTexture = eastl::vector<Texture*>(gLowFreq3DTextureSize);

	eastl::string gLowFrequencyOriginTextureName("VolumetricClouds/lowResCloudShape/lowResCloud(");
	for (uint32_t i = 0; i < gLowFreq3DTextureSize; ++i)
	{
		char stringInt[4];
		_itoa(i, stringInt);

		eastl::string gLowFrequencyNameLocal = gLowFrequencyOriginTextureName;

		gLowFrequencyNameLocal += eastl::string(stringInt);
		gLowFrequencyNameLocal += eastl::string(")");
		gLowFrequencyTextureNames.push_back(gLowFrequencyNameLocal);
		TextureLoadDesc lowFrequencyOrigin3DTextureDesc = {};

		lowFrequencyOrigin3DTextureDesc.pFileName = gLowFrequencyTextureNames[i].c_str();
		lowFrequencyOrigin3DTextureDesc.ppTexture = &gLowFrequencyOriginTexture[i];
		addResource(&lowFrequencyOrigin3DTextureDesc, &token);
	}

	waitForToken(&token);

	gHighFrequencyOriginTextureStorage = (Texture*)tf_malloc(sizeof(Texture) * gHighFrequencyOriginTexture.size());
	gLowFrequencyOriginTextureStorage = (Texture*)tf_malloc(sizeof(Texture) * gLowFrequencyOriginTexture.size());

	for (uint32_t i = 0; i < (uint32_t)gHighFrequencyOriginTexture.size(); ++i)
	{
		memcpy(&gHighFrequencyOriginTextureStorage[i], gHighFrequencyOriginTexture[i], sizeof(Texture));
		gHighFrequencyOriginTexturePacked.push_back(&gHighFrequencyOriginTextureStorage[i]);
	}

	for (uint32_t i = 0; i < (uint32_t)gLowFrequencyOriginTexture.size(); ++i)
	{
		memcpy(&gLowFrequencyOriginTextureStorage[i], gLowFrequencyOriginTexture[i], sizeof(Texture));
		gLowFrequencyOriginTexturePacked.push_back(&gLowFrequencyOriginTextureStorage[i]);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	TextureDesc lowFreqImgDesc = {};
	lowFreqImgDesc.mArraySize = 1;
	lowFreqImgDesc.mFormat = TinyImageFormat_R8G8B8A8_UNORM;

	lowFreqImgDesc.mWidth = gLowFreq3DTextureSize;
	lowFreqImgDesc.mHeight = gLowFreq3DTextureSize;
	lowFreqImgDesc.mDepth = gLowFreq3DTextureSize;

	lowFreqImgDesc.mMipLevels = LowFreqMipCount;
	lowFreqImgDesc.mSampleCount = SAMPLE_COUNT_1;
	//lowFreqImgDesc.mSrgb = false;

	lowFreqImgDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
	lowFreqImgDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	lowFreqImgDesc.pName = "LowFrequency3DTexture";

	TextureLoadDesc lowFrequency3DTextureDesc = {};
	lowFrequency3DTextureDesc.ppTexture = &pLowFrequency3DTexture;
	lowFrequency3DTextureDesc.pDesc = &lowFreqImgDesc;
	addResource(&lowFrequency3DTextureDesc, &token);

	lowFreqImgDesc.mMipLevels = 1;
	lowFrequency3DTextureDesc.ppTexture = &pLowFrequency3DTextureOrigin;
	addResource(&lowFrequency3DTextureDesc, &token);

	//////////////////////////////////////////////////////////////////////////////////////////////

	TextureDesc highFreqImgDesc = {};
	highFreqImgDesc.mArraySize = 1;
	highFreqImgDesc.mFormat = TinyImageFormat_R8G8B8A8_UNORM;

	highFreqImgDesc.mWidth = gHighFreq3DTextureSize;
	highFreqImgDesc.mHeight = gHighFreq3DTextureSize;
	highFreqImgDesc.mDepth = gHighFreq3DTextureSize;

	highFreqImgDesc.mMipLevels = HighFreqMipCount;
	highFreqImgDesc.mSampleCount = SAMPLE_COUNT_1;
	//highFreqImgDesc.mSrgb = false;

	highFreqImgDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
	highFreqImgDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	highFreqImgDesc.pName = "HighFrequency3DTexture";

	TextureLoadDesc highFrequency3DTextureDesc = {};
	highFrequency3DTextureDesc.ppTexture = &pHighFrequency3DTexture;
	highFrequency3DTextureDesc.pDesc = &highFreqImgDesc;
	addResource(&highFrequency3DTextureDesc, &token);

	highFreqImgDesc.mMipLevels = 1;
	highFrequency3DTextureDesc.ppTexture = &pHighFrequency3DTextureOrigin;
	addResource(&highFrequency3DTextureDesc, &token);

	//////////////////////////////////////////////////////////////////////////////////////////////

	TextureLoadDesc curlNoiseTextureDesc = {};
	curlNoiseTextureDesc.pFileName = "VolumetricClouds/CurlNoiseFBM";
	curlNoiseTextureDesc.ppTexture = &pCurlNoiseTexture;
	addResource(&curlNoiseTextureDesc, &token);

	//////////////////////////////////////////////////////////////////////////////////////////////

	TextureLoadDesc weathereTextureDesc = {};
	weathereTextureDesc.pFileName = "VolumetricClouds/WeatherMap";
	weathereTextureDesc.ppTexture = &pWeatherTexture;
	addResource(&weathereTextureDesc, &token);

	/*
	{
	  PipelineDesc CopyWeatherTexturePipelineDesc;
	  CopyWeatherTexturePipelineDesc.mType = PIPELINE_TYPE_COMPUTE;

	  ComputePipelineDesc &comPipelineSettings = CopyWeatherTexturePipelineDesc.mComputeDesc;
	  comPipelineSettings = {0};
	  comPipelineSettings.pShaderProgram = pCopyWeatherTextureShader;
	  comPipelineSettings.pRootSignature = pCopyWeatherTextureRootSignature;
	  addPipeline(pRenderer, &CopyWeatherTexturePipelineDesc, &pCopyWeatherTexturePipeline);

	  Cmd* cmd = ppTransCmds[0];
	  beginCmd(cmd);

	  cmdBindPipeline(cmd, pCopyWeatherTexturePipeline);

	  TextureBarrier barrier[] = {
		   { pWeatherCompactTexture, RESOURCE_STATE_UNORDERED_ACCESS }
	  };

	  cmdResourceBarrier(cmd, 0, NULL, 1, barrier, false);

	  DescriptorData mipParams[2] = {};
	  mipParams[0].pName = "SrcTexture";
	  mipParams[0].ppTextures = &pWeatherTexture;
	  mipParams[1].pName = "DstTexture";
	  mipParams[1].ppTextures = &pWeatherCompactTexture;
	  mipParams[1].mUAVMipSlice = 0;
	  cmdBindDescriptors(cmd, pVolumetricCloudsDescriptorBinder, pCopyWeatherTextureRootSignature, 2, mipParams);

	  uint32_t* pThreadGroupSize = pCopyWeatherTextureShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
	  cmdDispatch(cmd, (uint32_t)ceilf((float)pWeatherTexture->mWidth / (float)pThreadGroupSize[0]), (uint32_t)ceilf((float)pWeatherTexture->mHeight / (float)pThreadGroupSize[1]), 1);

	  TextureBarrier barriers[] = {
		{ pWeatherCompactTexture, RESOURCE_STATE_SHADER_RESOURCE }
	  };

	  cmdResourceBarrier(cmd, 0, NULL, 1, barriers, false);

	  endCmd(cmd);
	  queueSubmit(pGraphicsQueue, 1, &cmd, pTransitionCompleteFences, 0, 0, 0, 0);
	  waitForFences(pRenderer, 1, &pTransitionCompleteFences);
	}
	*/

	AddUniformBuffers();


	///////////////////////////////////////////////////////////////////
	// UI
	///////////////////////////////////////////////////////////////////

	GuiDesc guiDesc = {};
	guiDesc.mStartPosition = vec2(1600.0f / getDpiScale().getX(), 0.0f / getDpiScale().getY());
	guiDesc.mStartSize = vec2(300.0f / getDpiScale().getX(), 250.0f / getDpiScale().getY());
	pGuiWindow = addAppUIGuiComponent(pGAppUI, "Volumetric Cloud", &guiDesc);

	SliderUintWidget sliderUint;
	CheckboxWidget checkbox;
	SliderFloatWidget sliderFloat;
	SeparatorWidget separator;

	sliderUint.pData = &DownSampling;
	sliderUint.mMin = 1;
	sliderUint.mMax = 3;
	sliderUint.mStep = 1;
	addWidgetLua(addGuiWidget(pGuiWindow, "Downsampling", &sliderUint, WIDGET_TYPE_SLIDER_UINT));

	addWidgetLua(addGuiWidget(pGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

	ButtonWidget UseLowQuality;
	IWidget* pUseLowQuality = addGuiWidget(pGuiWindow, "Use  Low Quality Settings", &UseLowQuality, WIDGET_TYPE_BUTTON);
	pUseLowQuality->pOnEdited = VolumetricClouds::UseLowQualitySettings;
	addWidgetLua(pUseLowQuality);

	ButtonWidget UseMiddleQuality;
	IWidget* pUseMiddleQuality = addGuiWidget(pGuiWindow, "Use  Middle Quality Settings", &UseMiddleQuality, WIDGET_TYPE_BUTTON);
	pUseMiddleQuality->pOnEdited = VolumetricClouds::UseMiddleQualitySettings;
	addWidgetLua(pUseMiddleQuality);

	ButtonWidget UseHighQuality;
	IWidget* pUseHighQuality = addGuiWidget(pGuiWindow, "Use  High Quality Settings", &UseHighQuality, WIDGET_TYPE_BUTTON);
	pUseHighQuality->pOnEdited = VolumetricClouds::UseHighQualitySettings;
	addWidgetLua(pUseHighQuality);

	ButtonWidget UseUltraQuality;
	IWidget* pUseUltraQuality = addGuiWidget(pGuiWindow, "Use  Ultra Quality Settings", &UseUltraQuality, WIDGET_TYPE_BUTTON);
	pUseUltraQuality->pOnEdited = VolumetricClouds::UseUltraQualitySettings;
	addWidgetLua(pUseUltraQuality);

	addWidgetLua(addGuiWidget(pGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

	checkbox.pData = &gAppSettings.m_EnableBlur;
	addWidgetLua(addGuiWidget(pGuiWindow, "Enabled Edge Blur", &checkbox, WIDGET_TYPE_CHECKBOX));

	addWidgetLua(addGuiWidget(pGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

	sliderFloat.pData = &gAppSettings.m_EarthRadiusScale;
	sliderFloat.mMin = 0.01f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.01f;
	addWidgetLua(addGuiWidget(pGuiWindow, "Earth Radius Scale", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT));

	addWidgetLua(addGuiWidget(pGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

	CollapsingHeaderWidget CollapsingRayMarching;

	sliderUint.pData = &gAppSettings.m_MinSampleCount;
	sliderUint.mMin = 1;
	sliderUint.mMax = 256;
	sliderUint.mStep = 1;
	addCollapsingHeaderSubWidget(&CollapsingRayMarching, "Min Sample Iteration", &sliderUint, WIDGET_TYPE_SLIDER_UINT);

	sliderUint.pData = &gAppSettings.m_MaxSampleCount;
	sliderUint.mMin = 1;
	sliderUint.mMax = 1024;
	sliderUint.mStep = 1;
	addCollapsingHeaderSubWidget(&CollapsingRayMarching, "Max Sample Iteration", &sliderUint, WIDGET_TYPE_SLIDER_UINT);

	sliderFloat.pData = &gAppSettings.m_MinStepSize;
	sliderFloat.mMin = 1.0f;
	sliderFloat.mMax = 2048.0f;
	sliderFloat.mStep = 32.0f;
	addCollapsingHeaderSubWidget(&CollapsingRayMarching, "Min Step Size", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_MaxStepSize;
	sliderFloat.mMin = 1.0f;
	sliderFloat.mMax = 4096.0f;
	sliderFloat.mStep = 32.0f;
	addCollapsingHeaderSubWidget(&CollapsingRayMarching, "Max Step Size", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_MaxSampleDistance;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 250000.0f;
	sliderFloat.mStep = 32.0f;
	addCollapsingHeaderSubWidget(&CollapsingRayMarching, "Max Sample Distance", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);
	
	checkbox.pData = &gAppSettings.m_EnabledTemporalRayOffset;
	addCollapsingHeaderSubWidget(&CollapsingRayMarching, "Enabled Temporal RayOffset", &checkbox, WIDGET_TYPE_CHECKBOX);

	addWidgetLua(addGuiWidget(pGuiWindow, "Ray Marching", &CollapsingRayMarching, WIDGET_TYPE_COLLAPSING_HEADER));

	addWidgetLua(addGuiWidget(pGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

	CollapsingHeaderWidget CollapsingVC01;

	CollapsingHeaderWidget CollapsingSkyDome;
	
	sliderFloat.pData = &gAppSettings.m_CloudsLayerStart;
	sliderFloat.mMin = -1000000.0f;
	sliderFloat.mMax = 1000000.0f;
	sliderFloat.mStep = 100.0f;
	addCollapsingHeaderSubWidget(&CollapsingSkyDome, "Clouds Layer Start Height", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_LayerThickness;
	sliderFloat.mMin = 1.0f;
	sliderFloat.mMax = 1000000.0f;
	sliderFloat.mStep = 100.0f;
	addCollapsingHeaderSubWidget(&CollapsingSkyDome, "Layer Thickness", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	CollapsingHeaderWidget CollapsingCloud;

	sliderFloat.pData = &gAppSettings.m_BaseTile;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud, "Base Cloud Tiling", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_DetailTile;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud, "Detail Cloud Tiling", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_DetailStrength;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 2.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud, "Detail Strength", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CloudTopOffset;
	sliderFloat.mMin = 0.01f;
	sliderFloat.mMax = 2000.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloud, "Cloud Top Offset", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CloudSize;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 1000000.0f;
	sliderFloat.mStep = 0.1f;
	addCollapsingHeaderSubWidget(&CollapsingCloud, "Cloud Size", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CloudDensity;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloud, "Cloud Density", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CloudCoverageModifier;
	sliderFloat.mMin = -1.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud, "Cloud Coverage Modifier", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CloudTypeModifier;
	sliderFloat.mMin = -1.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud, "Cloud Type Modifier", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_AnvilBias;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud, "Anvil Bias", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	CollapsingHeaderWidget CollapsingCloudCurl;

	sliderFloat.pData = &gAppSettings.m_CurlTile;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 4.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloudCurl, "Curl Tiling", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CurlStrength;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 10000.0f;
	sliderFloat.mStep = 0.1f;
	addCollapsingHeaderSubWidget(&CollapsingCloudCurl, "Curl Strength", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	CollapsingHeaderWidget CollapsingCloudWeather;

	sliderFloat.pData = &gAppSettings.m_WeatherTexSize;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 10000000.0f;
	sliderFloat.mStep = 0.1f;
	addCollapsingHeaderSubWidget(&CollapsingCloudWeather, "Weather Texture Size", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.WeatherTextureAzimuth;
	sliderFloat.mMin = -180.0f;
	sliderFloat.mMax = 180.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloudWeather, "Weather Texture Direction", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.WeatherTextureDistance;
	sliderFloat.mMin = -1000000.0f;
	sliderFloat.mMax = 1000000.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloudWeather, "Weather Texture Distance", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	CollapsingHeaderWidget CollapsingWind;

	sliderFloat.pData = &gAppSettings.m_WindAzimuth;
	sliderFloat.mMin = -180.0f;
	sliderFloat.mMax = 180.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind, "Wind Direction", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_WindIntensity;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 100.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind, "Wind Intensity", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	checkbox.pData = &gAppSettings.m_EnabledRotation;
	addCollapsingHeaderSubWidget(&CollapsingWind, "Enabled Rotation", &checkbox, WIDGET_TYPE_CHECKBOX);

	sliderFloat.pData = &gAppSettings.m_RotationPivotAzimuth;
	sliderFloat.mMin = -180.0f;
	sliderFloat.mMax = 180.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind, "Rotation Pivot Azimuth", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_RotationPivotDistance;
	sliderFloat.mMin = -10.0f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingWind, "Rotation Pivot Distance", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_RotationIntensity;
	sliderFloat.mMin = -20.0f;
	sliderFloat.mMax = 20.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingWind, "Rotation Intensity", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_RisingVaporScale;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind, "RisingVapor Scale", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_RisingVaporUpDirection;
	sliderFloat.mMin = -1.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingWind, "RisingVapor UpDirection", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_RisingVaporIntensity;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingWind, "RisingVapor Intensity", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);
	
	sliderFloat.pData = &gAppSettings.m_NoiseFlowAzimuth;
	sliderFloat.mMin = -180.0f;
	sliderFloat.mMax = 180.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind, "Noise Flow Azimuth", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);
	
	sliderFloat.pData = &gAppSettings.m_NoiseFlowIntensity;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 100.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind, "Noise Flow Intensity", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	addCollapsingHeaderSubWidget(&CollapsingVC01, "Sky Dome", &CollapsingSkyDome, WIDGET_TYPE_COLLAPSING_HEADER);
	addCollapsingHeaderSubWidget(&CollapsingVC01, "Cloud Modeling", &CollapsingCloud, WIDGET_TYPE_COLLAPSING_HEADER);
	addCollapsingHeaderSubWidget(&CollapsingVC01, "Cloud Curl", &CollapsingCloudCurl, WIDGET_TYPE_COLLAPSING_HEADER);
	addCollapsingHeaderSubWidget(&CollapsingVC01, "Weather", &CollapsingCloudWeather, WIDGET_TYPE_COLLAPSING_HEADER);
	addCollapsingHeaderSubWidget(&CollapsingVC01, "Wind", &CollapsingWind, WIDGET_TYPE_COLLAPSING_HEADER);

	addWidgetLua(addGuiWidget(pGuiWindow, "Volumetric Clouds", &CollapsingVC01, WIDGET_TYPE_COLLAPSING_HEADER));

	addWidgetLua(addGuiWidget(pGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

	checkbox.pData = &gAppSettings.m_Enabled2ndLayer;
	addWidgetLua(addGuiWidget(pGuiWindow, "Enabled 2nd Layer", &checkbox, WIDGET_TYPE_CHECKBOX));

	CollapsingHeaderWidget CollapsingVC02;

	CollapsingHeaderWidget CollapsingSkyDome2;

	sliderFloat.pData = &gAppSettings.m_CloudsLayerStart_2nd;
	sliderFloat.mMin = -1000000.0f;
	sliderFloat.mMax = 1000000.0f;
	sliderFloat.mStep = 100.0f;
	addCollapsingHeaderSubWidget(&CollapsingSkyDome2, "Clouds Layer Start Height 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_LayerThickness_2nd;
	sliderFloat.mMin = 1.0f;
	sliderFloat.mMax = 1000000.0f;
	sliderFloat.mStep = 100.0f;
	addCollapsingHeaderSubWidget(&CollapsingSkyDome2, "Layer Thickness 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	CollapsingHeaderWidget CollapsingCloud2;

	sliderFloat.pData = &gAppSettings.m_BaseTile_2nd;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud2, "Base Cloud Tiling 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_DetailTile_2nd;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud2, "Detail Cloud Tiling 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_DetailStrength_2nd;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 2.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud2, "Detail Strength 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CloudTopOffset_2nd;
	sliderFloat.mMin = 0.01f;
	sliderFloat.mMax = 2000.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloud2, "Cloud Top Offset 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CloudSize_2nd;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 1000000.0f;
	sliderFloat.mStep = 0.1f;
	addCollapsingHeaderSubWidget(&CollapsingCloud2, "Cloud Size 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CloudDensity_2nd;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloud2, "Cloud Density 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CloudCoverageModifier_2nd;
	sliderFloat.mMin = -1.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud2, "Cloud Coverage Modifier 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CloudTypeModifier_2nd;
	sliderFloat.mMin = -1.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud2, "Cloud Type Modifier 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_AnvilBias_2nd;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloud2, "Anvil Bias 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	CollapsingHeaderWidget CollapsingCloudCurl2;

	sliderFloat.pData = &gAppSettings.m_CurlTile_2nd;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 4.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloudCurl2, "Curl Tiling 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CurlStrength_2nd;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 10000.0f;
	sliderFloat.mStep = 0.1f;
	addCollapsingHeaderSubWidget(&CollapsingCloudCurl2, "Curl Strength 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	CollapsingHeaderWidget CollapsingCloudWeather2;

	sliderFloat.pData = &gAppSettings.m_WeatherTexSize_2nd;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 10000000.0f;
	sliderFloat.mStep = 0.1f;
	addCollapsingHeaderSubWidget(&CollapsingCloudWeather2, "Weather Texture Size 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.WeatherTextureAzimuth_2nd;
	sliderFloat.mMin = -180.0f;
	sliderFloat.mMax = 180.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloudWeather2, "Weather Texture Direction 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.WeatherTextureDistance_2nd;
	sliderFloat.mMin = -1000000.0f;
	sliderFloat.mMax = 1000000.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloudWeather2, "Weather Texture Distance 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	CollapsingHeaderWidget CollapsingWind2;

	sliderFloat.pData = &gAppSettings.m_WindAzimuth_2nd;
	sliderFloat.mMin = -180.0f;
	sliderFloat.mMax = 180.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind2, "Wind Direction 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_WindIntensity_2nd;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 100.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind2, "Wind Intensity 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	checkbox.pData = &gAppSettings.m_EnabledRotation_2nd;
	addCollapsingHeaderSubWidget(&CollapsingWind2, "Enabled Rotation 2nd", &checkbox, WIDGET_TYPE_CHECKBOX);

	sliderFloat.pData = &gAppSettings.m_RotationPivotAzimuth_2nd;
	sliderFloat.mMin = -180.0f;
	sliderFloat.mMax = 180.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind2, "Rotation Pivot Azimuth 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_RotationPivotDistance_2nd;
	sliderFloat.mMin = -10.0f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingWind2, "Rotation Pivot Distance 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_RotationIntensity_2nd;
	sliderFloat.mMin = -20.0f;
	sliderFloat.mMax = 20.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingWind2, "Rotation Intensity 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_RisingVaporScale_2nd;
	sliderFloat.mMin = 0.001f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind2, "RisingVapor Scale 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_RisingVaporUpDirection_2nd;
	sliderFloat.mMin = -1.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingWind2, "RisingVapor UpDirection 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_RisingVaporIntensity_2nd;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingWind2, "RisingVapor Intensity 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_NoiseFlowAzimuth_2nd;
	sliderFloat.mMin = -180.0f;
	sliderFloat.mMax = 180.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind2, "Noise Flow Azimuth 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_NoiseFlowIntensity_2nd;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 100.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingWind2, "Noise Flow Intensity 2nd", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	addCollapsingHeaderSubWidget(&CollapsingVC02, "Sky Dome 2nd", &CollapsingSkyDome2, WIDGET_TYPE_COLLAPSING_HEADER);
	addCollapsingHeaderSubWidget(&CollapsingVC02, "Cloud Modeling 2nd", &CollapsingCloud2, WIDGET_TYPE_COLLAPSING_HEADER);
	addCollapsingHeaderSubWidget(&CollapsingVC02, "Cloud Curl 2nd", &CollapsingCloudCurl2, WIDGET_TYPE_COLLAPSING_HEADER);
	addCollapsingHeaderSubWidget(&CollapsingVC02, "Weather 2nd", &CollapsingCloudWeather2, WIDGET_TYPE_COLLAPSING_HEADER);
	addCollapsingHeaderSubWidget(&CollapsingVC02, "Wind 2nd", &CollapsingWind2, WIDGET_TYPE_COLLAPSING_HEADER);

	addWidgetLua(addGuiWidget(pGuiWindow, "Volumetric Clouds 2nd", &CollapsingVC02, WIDGET_TYPE_COLLAPSING_HEADER));

	addWidgetLua(addGuiWidget(pGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

	CollapsingHeaderWidget CollapsingCloudLighting;

	ColorPickerWidget colorPicker;
	colorPicker.pData = &gAppSettings.m_CustomColor;
	addCollapsingHeaderSubWidget(&CollapsingCloudLighting, "Custom Color", &colorPicker, WIDGET_TYPE_COLOR_PICKER);

	sliderFloat.pData = &gAppSettings.m_CustomColorIntensity;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloudLighting, "Custom Color Intensity", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CustomColorBlendFactor;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloudLighting, "Custom Color Blend", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_TransStepSize;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 2048.0f;
	sliderFloat.mStep = 1.0f;
	addCollapsingHeaderSubWidget(&CollapsingCloudLighting, "Trans Step Size", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_Contrast;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 2.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloudLighting, "Contrast", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_BackgroundBlendFactor;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloudLighting, "BackgroundBlendFactor", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_Precipitation;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 10.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloudLighting, "Precipitation", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);
	
	sliderFloat.pData = &gAppSettings.m_SilverIntensity;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 2.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloudLighting, "Silver Intensity", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_SilverSpread;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 0.99f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloudLighting, "Silver Spread", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_Eccentricity;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingCloudLighting, "Eccentricity", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_CloudBrightness;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 2.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingCloudLighting, "Cloud Brightness", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	addWidgetLua(addGuiWidget(pGuiWindow, "Cloud Lighting", &CollapsingCloudLighting, WIDGET_TYPE_COLLAPSING_HEADER));

	addWidgetLua(addGuiWidget(pGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));


	CollapsingHeaderWidget CollapsingCulling;

	checkbox.pData = &gAppSettings.m_EnabledDepthCulling;
	addCollapsingHeaderSubWidget(&CollapsingCulling, "Enabled Depth Culling", &checkbox, WIDGET_TYPE_CHECKBOX);

	checkbox.pData = &gAppSettings.m_EnabledLodDepth;
	addCollapsingHeaderSubWidget(&CollapsingCulling, "Enabled Lod Depth", &checkbox, WIDGET_TYPE_CHECKBOX);

	addWidgetLua(addGuiWidget(pGuiWindow, "Depth Culling", &CollapsingCulling, WIDGET_TYPE_COLLAPSING_HEADER));

	addWidgetLua(addGuiWidget(pGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

	CollapsingHeaderWidget CollapsingShadow;

	checkbox.pData = &gAppSettings.m_EnabledShadow;
	addCollapsingHeaderSubWidget(&CollapsingShadow, "Enabled Shadow", &checkbox, WIDGET_TYPE_CHECKBOX);

	sliderFloat.pData = &gAppSettings.m_ShadowBrightness;
	sliderFloat.mMin = -1.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingShadow, "Shadow Brightness", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_ShadowTiling;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 100.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingShadow, "Shadow Tiling", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_ShadowSpeed;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 100.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingShadow, "Shadow Speed", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	addWidgetLua(addGuiWidget(pGuiWindow, "Clouds Shadow", &CollapsingShadow, WIDGET_TYPE_COLLAPSING_HEADER));

	addWidgetLua(addGuiWidget(pGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

	CollapsingHeaderWidget CollapsingGodray;

	checkbox.pData = &gAppSettings.m_EnabledGodray;
	addCollapsingHeaderSubWidget(&CollapsingGodray, "Enabled Godray", &checkbox, WIDGET_TYPE_CHECKBOX);

	sliderUint.pData = &gAppSettings.m_GodNumSamples;
	sliderUint.mMin = 1;
	sliderUint.mMax = 256;
	sliderUint.mStep = 1;
	addCollapsingHeaderSubWidget(&CollapsingGodray, "Number of Samples", &sliderUint, WIDGET_TYPE_SLIDER_UINT);

	sliderFloat.pData = &gAppSettings.m_Exposure;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 0.1f;
	sliderFloat.mStep = 0.0001f;
	addCollapsingHeaderSubWidget(&CollapsingGodray, "Exposure", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_Decay;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 2.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingGodray, "Decay", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);
	
	sliderFloat.pData = &gAppSettings.m_Density;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 2.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingGodray, "Density", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_Weight;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 2.0f;
	sliderFloat.mStep = 0.01f;
	addCollapsingHeaderSubWidget(&CollapsingGodray, "Weight", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	addWidgetLua(addGuiWidget(pGuiWindow, "Clouds Godray", &CollapsingGodray, WIDGET_TYPE_COLLAPSING_HEADER));

	addWidgetLua(addGuiWidget(pGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));
	
	CollapsingHeaderWidget CollapsingTest;

	sliderFloat.pData = &gAppSettings.m_Test00;
	sliderFloat.mMin = -1.0f;
	sliderFloat.mMax = 1.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingTest, "Test00", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_Test01;
	sliderFloat.mMin = 0.0f;
	sliderFloat.mMax = 2.0f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingTest, "Test01", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	sliderFloat.pData = &gAppSettings.m_Test02;
	sliderFloat.mMin = 1.0f;
	sliderFloat.mMax = 200000.0f;
	sliderFloat.mStep = 100.0f;
	addCollapsingHeaderSubWidget(&CollapsingTest, "Test02", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);
	
	sliderFloat.pData = &gAppSettings.m_Test03;
	sliderFloat.mMin = -0.999f;
	sliderFloat.mMax = 0.999f;
	sliderFloat.mStep = 0.001f;
	addCollapsingHeaderSubWidget(&CollapsingTest, "Test03", &sliderFloat, WIDGET_TYPE_SLIDER_FLOAT);

	addWidgetLua(addGuiWidget(pGuiWindow, "Clouds Test", &CollapsingTest, WIDGET_TYPE_COLLAPSING_HEADER));
	
	///////////////////////////

#if defined(XBOX) && !defined(SCARLETT)
	UseMiddleQualitySettings();
#else
	UseHighQualitySettings();
#endif

	waitForToken(&token);

	TextureDesc WeatherCompactTextureDesc = {};
	WeatherCompactTextureDesc.mArraySize = 1;
	WeatherCompactTextureDesc.mFormat = TinyImageFormat_R8G8_UNORM;

	WeatherCompactTextureDesc.mWidth = pWeatherTexture->mWidth;
	WeatherCompactTextureDesc.mHeight = pWeatherTexture->mHeight;
	WeatherCompactTextureDesc.mDepth = pWeatherTexture->mDepth;

	WeatherCompactTextureDesc.mMipLevels = 1;
	WeatherCompactTextureDesc.mSampleCount = SAMPLE_COUNT_1;

	WeatherCompactTextureDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
	WeatherCompactTextureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	WeatherCompactTextureDesc.pName = "WeatherCompactTexture";

	TextureLoadDesc WeatherCompactTextureLoadDesc = {};
	WeatherCompactTextureLoadDesc.ppTexture = &pWeatherCompactTexture;
	WeatherCompactTextureLoadDesc.pDesc = &WeatherCompactTextureDesc;
	addResource(&WeatherCompactTextureLoadDesc, &token);
	
	GenerateCloudTextures();

	return true;
}


void VolumetricClouds::Exit()
{
	removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetCompute[0]);
#if !USE_VC_FRAGMENTSHADER
	removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetCompute[1]);
#endif
	removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetGraphics[0]);
	removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetGraphics[1]);

	RemoveUniformBuffers();

	removeResource(pHighFrequency3DTexture);
	removeResource(pLowFrequency3DTexture);

	removeResource(pHighFrequency3DTextureOrigin);
	removeResource(pLowFrequency3DTextureOrigin);

	removeResource(pWeatherTexture);
	removeResource(pWeatherCompactTexture);
	removeResource(pCurlNoiseTexture);

	// Remove Textures
	for (uint32_t i = 0; i < gHighFreq3DTextureSize; ++i)
	{
		removeResource(gHighFrequencyOriginTexture[i]);
	}

	// Remove Textures
	for (uint32_t i = 0; i < gLowFreq3DTextureSize; ++i)
	{
		removeResource(gLowFrequencyOriginTexture[i]);
	}

	tf_free(gHighFrequencyOriginTextureStorage);
	tf_free(gLowFrequencyOriginTextureStorage);

	gHighFrequencyTextureNames.set_capacity(0);
	gHighFrequencyTextureNames.clear();
	gLowFrequencyTextureNames.set_capacity(0);
	gLowFrequencyTextureNames.clear();

	gHighFrequencyOriginTexturePacked.set_capacity(0);
	gHighFrequencyOriginTexturePacked.clear();
	gLowFrequencyOriginTexturePacked.set_capacity(0);
	gLowFrequencyOriginTexturePacked.clear();

	gHighFrequencyOriginTexture.set_capacity(0);
	gHighFrequencyOriginTexture.clear();
	gLowFrequencyOriginTexture.set_capacity(0);
	gLowFrequencyOriginTexture.clear();

	removeResource(pTriangularScreenVertexBuffer);

	removeShader(pRenderer, pCompositeOverlayShader);
	//removeRootSignature(pRenderer, pCompositeOverlayRootSignature);
	//removeDescriptorBinder(pRenderer, pCompositeOverlayDescriptorBinder);



	removeSampler(pRenderer, pPointSampler);
	removeSampler(pRenderer, pBilinearSampler);
	removeSampler(pRenderer, pPointClampSampler);
	removeSampler(pRenderer, pBiClampSampler);
	removeSampler(pRenderer, pLinearBorderSampler);
	//removeShader(pRenderer, pSaveCurrentShader);	
	removeShader(pRenderer, pPostProcessShader);
	removeShader(pRenderer, pVolumetricCloudShader);

#if USE_VC_FRAGMENTSHADER
	removeShader(pRenderer, pVolumetricCloudWithDepthShader);
	removeShader(pRenderer, pVolumetricCloud2ndShader);
	removeShader(pRenderer, pVolumetricCloud2ndWithDepthShader);
#else
	removeShader(pRenderer, pVolumetricCloudCompShader);
	removeShader(pRenderer, pVolumetricCloud2ndCompShader);
	removeShader(pRenderer, pVolumetricCloudWithDepthCompShader);
	removeShader(pRenderer, pVolumetricCloud2ndWithDepthCompShader);
#endif
	removeShader(pRenderer, pReprojectionShader);
	removeShader(pRenderer, pGodrayShader);
	removeShader(pRenderer, pGodrayAddShader);
	//removeShader(pRenderer, pCastShadowShader);

	removeShader(pRenderer, pGenHiZMipmapShader);
	removeShader(pRenderer, pCopyTextureShader);
	//removeShader(pRenderer, pCopyWeatherTextureShader);
	removeShader(pRenderer, pCopyRTShader);
	removeShader(pRenderer, pCompositeShader);
	removeShader(pRenderer, pGenHiZMipmapPRShader);

	removeShader(pRenderer, pHorizontalBlurShader);
	removeShader(pRenderer, pVerticalBlurShader);

	removeShader(pRenderer, pPostProcessWithBlurShader);

	removeRootSignature(pRenderer, pCopyTextureRootSignature);
	//removeRootSignature(pRenderer, pCopyWeatherTextureRootSignature);

	removeRootSignature(pRenderer, pGenHiZMipmapRootSignature);
	removeRootSignature(pRenderer, pVolumetricCloudsRootSignatureGraphics);
	removeRootSignature(pRenderer, pVolumetricCloudsRootSignatureCompute);
}

Texture* VolumetricClouds::GetWeatherMap()
{
	return pWeatherTexture;
};

bool VolumetricClouds::Load(RenderTarget** rts, uint32_t count)
{
	pFinalRT = rts;

	gDownsampledCloudSize = (uint32_t)pow(2, (double)DownSampling);

	mWidth = (*rts)->mWidth;
	mHeight = (*rts)->mHeight;

	float aspect = (float)mWidth / (float)mHeight;
	float aspectInverse = 1.0f / aspect;
	float horizontal_fov = PI / 3.0f;
	float vertical_fov = 2.0f * atan(tan(horizontal_fov*0.5f) * aspectInverse);
	//projMat = mat4::perspective(horizontal_fov, aspectInverse, _CLOUDS_LAYER_START * 0.01, _CLOUDS_LAYER_END * 10.0);
	projMat = mat4::perspective(horizontal_fov, aspectInverse, TERRAIN_NEAR, TERRAIN_FAR);

	if (!AddVolumetricCloudsRenderTargets())
		return false;

	if (!AddHiZDepthBuffer())
		return false;

	g_ProjectionExtents = GetProjectionExtents(vertical_fov, aspect,
		(float)((mWidth / gDownsampledCloudSize) & (~31)), (float)((mHeight / gDownsampledCloudSize) & (~31)), 0.0f, 0.0f);

	float screenMiscPoints[] = {
			g_ProjectionExtents.getX(), g_ProjectionExtents.getY(), g_ProjectionExtents.getZ(), g_ProjectionExtents.getW(),
			g_ProjectionExtents.getX(), g_ProjectionExtents.getY(), g_ProjectionExtents.getZ(), g_ProjectionExtents.getW(),
			g_ProjectionExtents.getX(), g_ProjectionExtents.getY(), g_ProjectionExtents.getZ(), g_ProjectionExtents.getW(),
	};

	SyncToken token = {};

	uint64_t screenDataSize = 4 * 3 * sizeof(float);
	BufferLoadDesc screenMiscVbDesc = {};
	screenMiscVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	screenMiscVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	screenMiscVbDesc.mDesc.mSize = screenDataSize;
	screenMiscVbDesc.pData = screenMiscPoints;
	screenMiscVbDesc.ppBuffer = &pTriangularScreenVertexWithMiscBuffer;
	addResource(&screenMiscVbDesc, &token);

	volumetricCloudsCB = VolumetricCloudsCB();

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

	VertexLayout vertexLayoutForVC = {};
	vertexLayoutForVC.mAttribCount = 1;
	vertexLayoutForVC.mAttribs[0].mSemantic = SEMANTIC_TEXCOORD0;
	vertexLayoutForVC.mAttribs[0].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
	vertexLayoutForVC.mAttribs[0].mBinding = 0;
	vertexLayoutForVC.mAttribs[0].mLocation = 0;
	vertexLayoutForVC.mAttribs[0].mOffset = 0;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	BlendStateDesc blendStateSkyBoxDesc = {};
	blendStateSkyBoxDesc.mBlendModes[0] = BM_ADD;
	blendStateSkyBoxDesc.mBlendAlphaModes[0] = BM_ADD;

	blendStateSkyBoxDesc.mSrcFactors[0] = BC_SRC_ALPHA;// BC_ONE;// BC_ONE_MINUS_DST_ALPHA;
	blendStateSkyBoxDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;// BC_ZERO;// BC_DST_ALPHA;

	blendStateSkyBoxDesc.mSrcAlphaFactors[0] = BC_ONE;
	blendStateSkyBoxDesc.mDstAlphaFactors[0] = BC_ZERO;

	blendStateSkyBoxDesc.mMasks[0] = ALL;
	blendStateSkyBoxDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	BlendStateDesc blendStateGodrayDesc = {};
	blendStateGodrayDesc.mBlendModes[0] = BM_ADD;
	blendStateGodrayDesc.mBlendAlphaModes[0] = BM_ADD;

	blendStateGodrayDesc.mSrcFactors[0] = BC_ONE;
	blendStateGodrayDesc.mDstFactors[0] = BC_ONE;

	blendStateGodrayDesc.mSrcAlphaFactors[0] = BC_ZERO;
	blendStateGodrayDesc.mDstAlphaFactors[0] = BC_ONE;

	blendStateGodrayDesc.mMasks[0] = ALL;
	blendStateGodrayDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;

	RasterizerStateDesc rasterizerStateDesc = {};
	rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

	PipelineDesc pipelineDescVolumetricCloud = {};
	pipelineDescVolumetricCloud.pCache = pPipelineCache;
	{
		pipelineDescVolumetricCloud.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescVolumetricCloud.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &plowResCloudRT->mFormat;
		pipelineSettings.mSampleCount = (*rts)->mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;

		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pVolumetricCloudShader;
		pipelineSettings.pVertexLayout = &vertexLayoutForVC;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;

		addPipeline(pRenderer, &pipelineDescVolumetricCloud, &pVolumetricCloudPipeline);
	}

#if USE_VC_FRAGMENTSHADER
	PipelineDesc pipelineDescVolumetricCloudWithDepthShader = {};
	pipelineDescVolumetricCloudWithDepthShader.pCache = pPipelineCache;
	{
		pipelineDescVolumetricCloudWithDepthShader.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescVolumetricCloudWithDepthShader.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &plowResCloudRT->mFormat;
		pipelineSettings.mSampleCount = (*rts)->mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;

		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pVolumetricCloudWithDepthShader;
		pipelineSettings.pVertexLayout = &vertexLayoutForVC;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;

		addPipeline(pRenderer, &pipelineDescVolumetricCloudWithDepthShader, &pVolumetricCloudWithDepthPipeline);
	}

	PipelineDesc pipelineDescVolumetricCloud2nd = {};
	pipelineDescVolumetricCloud2nd.pCache = pPipelineCache;
	{
		pipelineDescVolumetricCloud2nd.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescVolumetricCloud2nd.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &plowResCloudRT->mFormat;
		pipelineSettings.mSampleCount = (*rts)->mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;

		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pVolumetricCloud2ndShader;
		pipelineSettings.pVertexLayout = &vertexLayoutForVC;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;

		addPipeline(pRenderer, &pipelineDescVolumetricCloud2nd, &pVolumetricCloud2ndPipeline);
	}

	PipelineDesc pipelineDescVolumetricCloud2ndWithDepth = {};
	pipelineDescVolumetricCloud2ndWithDepth.pCache = pPipelineCache;
	{
		pipelineDescVolumetricCloud2ndWithDepth.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescVolumetricCloud2ndWithDepth.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &plowResCloudRT->mFormat;
		pipelineSettings.mSampleCount = (*rts)->mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;

		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pVolumetricCloud2ndWithDepthShader;
		pipelineSettings.pVertexLayout = &vertexLayoutForVC;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;

		addPipeline(pRenderer, &pipelineDescVolumetricCloud2ndWithDepth, &pVolumetricCloud2ndWithDepthPipeline);
	}
#endif

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PipelineDesc pipelineDescReprojection = {};
	pipelineDescReprojection.pCache = pPipelineCache;
	{
		pipelineDescReprojection.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescReprojection.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &phighResCloudRT->mFormat;
		//pipelineSettings.pSrgbValues = &phighResCloudRT->mSrgb;
		pipelineSettings.mSampleCount = (*rts)->mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;

		// pipelineSettings.pRootSignature = pReprojectionRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pReprojectionShader;
		pipelineSettings.pVertexLayout = &vertexLayoutForVC;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;

		addPipeline(pRenderer, &pipelineDescReprojection, &pReprojectionPipeline);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	pipelineSettings = { 0 };
	pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
	pipelineSettings.mRenderTargetCount = 1;
	pipelineSettings.pDepthState = NULL;
	pipelineSettings.pColorFormats = &pSaveCurrentRT->mFormat;
	pipelineSettings.pSrgbValues = &pSaveCurrentRT->mSrgb;
	pipelineSettings.mSampleCount = (*rts)->mSampleCount;
	pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;
	pipelineSettings.pRootSignature = pSaveCurrentRootSignature;
	pipelineSettings.pShaderProgram = pSaveCurrentShader;
	pipelineSettings.pVertexLayout = &vertexLayout;
	pipelineSettings.pRasterizerState = pPostProcessRast;
	addPipeline(pRenderer, &pipelineSettings, &pSaveCurrentPipeline);
	*/
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PipelineDesc pipelineDescGodray = {};
	pipelineDescGodray.pCache = pPipelineCache;
	{
		pipelineDescGodray.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescGodray.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &pGodrayRT->mFormat;
		//pipelineSettings.pSrgbValues = &pGodrayRT->mSrgb;
		pipelineSettings.mSampleCount = (*rts)->mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;

		//pipelineSettings.pRootSignature = pGodrayRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pGodrayShader;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;

		addPipeline(pRenderer, &pipelineDescGodray, &pGodrayPipeline);
	}



	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PipelineDesc pipelineDescPostProcess = {};
	pipelineDescPostProcess.pCache = pPipelineCache;
	{
		pipelineDescPostProcess.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescPostProcess.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;

		TinyImageFormat mrtFormats[1] = {};
		for (uint32_t i = 0; i < 1; ++i)
		{
			mrtFormats[i] = pRenderTargetsPostProcess[i]->mFormat;
			//mrtSrgb[i] = pRenderTargetsPostProcess[i]->mSrgb;
		}

		pipelineSettings.pColorFormats = mrtFormats;
		//pipelineSettings.pSrgbValues = mrtSrgb;
		pipelineSettings.mSampleCount = (*rts)->mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;
		//pipelineSettings.pRootSignature = pPostProcessRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pPostProcessShader;
		pipelineSettings.pVertexLayout = &vertexLayoutForVC;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;

		addPipeline(pRenderer, &pipelineDescPostProcess, &pPostProcessPipeline);

		//pipelineSettings.pRootSignature = pPostProcessWithBlurRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pPostProcessWithBlurShader;

		addPipeline(pRenderer, &pipelineDescPostProcess, &pPostProcessWithBlurPipeline);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
  PipelineDesc pipelineDescCastShadow;
  {
	pipelineDescCastShadow.mType = PIPELINE_TYPE_GRAPHICS;
	GraphicsPipelineDesc &pipelineSettings = pipelineDescCastShadow.mGraphicsDesc;

	  pipelineSettings = { 0 };
	  pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
	  pipelineSettings.mRenderTargetCount = 1;
	  pipelineSettings.pDepthState = NULL;
	  pipelineSettings.pColorFormats = &pCastShadowRT->mFormat;
	  pipelineSettings.pSrgbValues = &pCastShadowRT->mSrgb;
	  pipelineSettings.mSampleCount = (*rts)->mSampleCount;
	  pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;
	  pipelineSettings.pRootSignature = pCastShadowRootSignature;
	  pipelineSettings.pShaderProgram = pCastShadowShader;
	  pipelineSettings.pVertexLayout = &vertexLayout;
	  pipelineSettings.pRasterizerState = &rasterizerStateDesc;

	  addPipeline(pRenderer, &pipelineDescCastShadow, &pCastShadowPipeline);
  }
*/

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	PipelineDesc pipelineDescGodrayAdd = {};
	pipelineDescGodrayAdd.pCache = pPipelineCache;
	{
		pipelineDescGodrayAdd.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescGodrayAdd.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &(*rts)->mFormat;
		//pipelineSettings.pSrgbValues = &(*rts)->mSrgb;
		pipelineSettings.mSampleCount = (*rts)->mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;
		//pipelineSettings.pRootSignature = pGodrayAddRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pGodrayAddShader;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;

		pipelineSettings.pBlendState = &blendStateGodrayDesc;

		addPipeline(pRenderer, &pipelineDescGodrayAdd, &pGodrayAddPipeline);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PipelineDesc pipelineDescComposite = {};
	pipelineDescComposite.pCache = pPipelineCache;
	{
		pipelineDescComposite.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescComposite.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &(*rts)->mFormat;
		//pipelineSettings.pSrgbValues = &(*rts)->mSrgb;
		pipelineSettings.mSampleCount = (*rts)->mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;
		//pipelineSettings.pRootSignature = pCompositeRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pCompositeShader;
		pipelineSettings.pVertexLayout = &vertexLayoutForVC;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;

		pipelineSettings.pBlendState = &blendStateSkyBoxDesc;

		addPipeline(pRenderer, &pipelineDescComposite, &pCompositePipeline);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PipelineDesc pipelineDescCompositeOverlay = {};
	pipelineDescCompositeOverlay.pCache = pPipelineCache;
	{
		pipelineDescCompositeOverlay.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescCompositeOverlay.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &(*rts)->mFormat;
		//pipelineSettings.pSrgbValues = &(*rts)->mSrgb;
		pipelineSettings.mSampleCount = (*rts)->mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mSampleQuality;
		//pipelineSettings.pRootSignature = pCompositeOverlayRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pCompositeOverlayShader;
		pipelineSettings.pVertexLayout = &vertexLayoutForVC;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;

		pipelineSettings.pBlendState = &blendStateSkyBoxDesc;

		addPipeline(pRenderer, &pipelineDescCompositeOverlay, &pCompositeOverlayPipeline);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PipelineDesc pipelineDesc = {};
	pipelineDesc.pCache = pPipelineCache;
	pipelineDesc.mType = PIPELINE_TYPE_COMPUTE;

	ComputePipelineDesc &comPipelineSettings = pipelineDesc.mComputeDesc;

	comPipelineSettings.pShaderProgram = pGenHiZMipmapShader;
	comPipelineSettings.pRootSignature = pGenHiZMipmapRootSignature;
	addPipeline(pRenderer, &pipelineDesc, &pGenHiZMipmapPipeline);

	comPipelineSettings.pShaderProgram = pCopyTextureShader;
	comPipelineSettings.pRootSignature = pCopyTextureRootSignature;
	addPipeline(pRenderer, &pipelineDesc, &pCopyTexturePipeline);

	comPipelineSettings.pShaderProgram = pCopyRTShader;
	comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
	addPipeline(pRenderer, &pipelineDesc, &pCopyRTPipeline);

#if !USE_VC_FRAGMENTSHADER
	comPipelineSettings.pShaderProgram = pVolumetricCloudCompShader;
	comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
	addPipeline(pRenderer, &pipelineDesc, &pVolumetricCloudCompPipeline);

	comPipelineSettings.pShaderProgram = pVolumetricCloud2ndCompShader;
	comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
	addPipeline(pRenderer, &pipelineDesc, &pVolumetricCloud2ndCompPipeline);	

	comPipelineSettings.pShaderProgram = pVolumetricCloudWithDepthCompShader;
	comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
	addPipeline(pRenderer, &pipelineDesc, &pVolumetricCloudWithDepthCompPipeline);

	comPipelineSettings.pShaderProgram = pVolumetricCloud2ndWithDepthCompShader;
	comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
	addPipeline(pRenderer, &pipelineDesc, &pVolumetricCloud2ndWithDepthCompPipeline);
#endif

	comPipelineSettings.pShaderProgram = pGenHiZMipmapPRShader;
	comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
	addPipeline(pRenderer, &pipelineDesc, &pGenHiZMipmapPRPipeline);

	comPipelineSettings.pShaderProgram = pHorizontalBlurShader;
	comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
	addPipeline(pRenderer, &pipelineDesc, &pHorizontalBlurPipeline);

	comPipelineSettings.pShaderProgram = pVerticalBlurShader;
	comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
	addPipeline(pRenderer, &pipelineDesc, &pVerticalBlurPipeline);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	waitForToken(&token);
	// Prepare descriptor sets
	// Hiz
	{
#if USE_DEPTH_CULLING
#if USE_LOD_DEPTH
		DescriptorData mipParams[2] = {};
		mipParams[0].pName = "SrcTexture";
		mipParams[0].ppTextures = &pLinearDepthTexture->pTexture;
		mipParams[1].pName = "DstTexture";
		mipParams[1].ppTextures = &pHiZDepthBufferX;
		updateDescriptorSet(pRenderer, 0, pVolumetricCloudsDescriptorSetCompute[0], 2, mipParams);
#else
#endif
#endif
	}
	// Volumetric Clouds
	{
#if USE_LOD_DEPTH
		Texture* HiZedDepthTexture = pHiZDepthBufferX;
#else
		Texture* HiZedDepthTexture = pHiZDepthBuffer;
#endif

#if USE_VC_FRAGMENTSHADER
		DescriptorData VCParams[5] = {};
		VCParams[0].pName = "highFreqNoiseTexture";
		VCParams[0].ppTextures = &pHighFrequency3DTexture;
		VCParams[1].pName = "lowFreqNoiseTexture";
		VCParams[1].ppTextures = &pLowFrequency3DTexture;
		VCParams[2].pName = "curlNoiseTexture";
		VCParams[2].ppTextures = &pCurlNoiseTexture;
		VCParams[3].pName = "weatherTexture";
		VCParams[3].ppTextures = &pWeatherTexture;
		VCParams[4].pName = "depthTexture";
		VCParams[4].ppTextures = &HiZedDepthTexture;
		updateDescriptorSet(pRenderer, 6, pVolumetricCloudsDescriptorSetGraphics[0], 5, VCParams);

		VCParams[4].pName = "depthTexture";
		VCParams[4].ppTextures = &pLinearDepthTexture->pTexture;
		updateDescriptorSet(pRenderer, 7, pVolumetricCloudsDescriptorSetGraphics[0], 5, VCParams);
#else
		DescriptorData VCParams[6] = {};
		VCParams[0].pName = "highFreqNoiseTexture";
		VCParams[0].ppTextures = &pHighFrequency3DTexture;
		VCParams[1].pName = "lowFreqNoiseTexture";
		VCParams[1].ppTextures = &pLowFrequency3DTexture;
		VCParams[2].pName = "curlNoiseTexture";
		VCParams[2].ppTextures = &pCurlNoiseTexture;
		VCParams[3].pName = "weatherTexture";
		VCParams[3].ppTextures = &pWeatherTexture;
		VCParams[4].pName = "volumetricCloudsDstTexture";
		VCParams[4].ppTextures = &plowResCloudTexture;
		VCParams[5].pName = "depthTexture";
		VCParams[5].ppTextures = &HiZedDepthTexture;
		updateDescriptorSet(pRenderer, 4, pVolumetricCloudsDescriptorSetCompute[0], 6, VCParams);

		VCParams[5].pName = "depthTexture";
		VCParams[5].ppTextures = &pLinearDepthTexture->pTexture;
		updateDescriptorSet(pRenderer, 5, pVolumetricCloudsDescriptorSetCompute[0], 6, VCParams);
#endif
		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			DescriptorData VCParams[1] = {};
			VCParams[0].pName = "VolumetricCloudsCBuffer";
			VCParams[0].ppBuffers = &VolumetricCloudsCBuffer[i];
#if !USE_VC_FRAGMENTSHADER
			updateDescriptorSet(pRenderer, i, pVolumetricCloudsDescriptorSetCompute[1], 1, VCParams);
#endif
			updateDescriptorSet(pRenderer, i, pVolumetricCloudsDescriptorSetGraphics[1], 1, VCParams);
		}
	}
	// Reprojection
	{
#if USE_RP_FRAGMENTSHADER
		DescriptorData RPparams[2] = {};
		RPparams[0].pName = "LowResCloudTexture";
#if USE_VC_FRAGMENTSHADER
		RPparams[0].ppTextures = &plowResCloudRT->pTexture;
#else
		RPparams[0].ppTextures = &plowResCloudTexture;
#endif
		RPparams[1].pName = "g_PrevFrameTexture";
		RPparams[1].ppTextures = &pSavePrevTexture;
		updateDescriptorSet(pRenderer, 0, pVolumetricCloudsDescriptorSetGraphics[0], 2, RPparams);
#endif
	}
	// Copy RT
	{
		DescriptorData ppParams[3] = {};
		ppParams[0].pName = "SrcTexture";
		ppParams[0].ppTextures = &phighResCloudRT->pTexture;
		ppParams[1].pName = "SavePrevTexture";
		ppParams[1].ppTextures = &pSavePrevTexture;
		updateDescriptorSet(pRenderer, 1, pVolumetricCloudsDescriptorSetCompute[0], 2, ppParams);
	}
	// HBlur
	{
		DescriptorData params[3] = {};
		params[0].pName = "InputTex";
		params[0].ppTextures = &pSavePrevTexture;
		params[1].pName = "OutputTex";
		params[1].ppTextures = &pHBlurTex;
		updateDescriptorSet(pRenderer, 2, pVolumetricCloudsDescriptorSetCompute[0], 2, params);
	}
	// VBlur
	{
		DescriptorData params[3] = {};
		params[0].pName = "InputTex";
		params[0].ppTextures = &pHBlurTex;
		params[1].pName = "OutputTex";
		params[1].ppTextures = &pVBlurTex;
		updateDescriptorSet(pRenderer, 3, pVolumetricCloudsDescriptorSetCompute[0], 2, params);
	}
	// Post process
	{
		DescriptorData PPparams[5] = {};
		PPparams[0].pName = "g_SrcTexture2D";
		PPparams[0].ppTextures = &phighResCloudRT->pTexture;
		PPparams[1].pName = "g_SkyBackgroudTexture";
		PPparams[1].ppTextures = &pSceneColorTexture->pTexture;
		PPparams[2].pName = "TransmittanceColor";
		PPparams[2].ppBuffers = &pTransmittanceBuffer;
		
		updateDescriptorSet(pRenderer, 1, pVolumetricCloudsDescriptorSetGraphics[0], 3, PPparams);

		PPparams[3].pName = "g_BlurTexture";
		PPparams[3].ppTextures = &pVBlurTex;
		updateDescriptorSet(pRenderer, 2, pVolumetricCloudsDescriptorSetGraphics[0], 4, PPparams);

	}
	// Godray
	{
		DescriptorData PPparams[2] = {};
		PPparams[0].pName = "g_PrevFrameTexture";
		PPparams[0].ppTextures = &phighResCloudRT->pTexture;
		PPparams[1].pName = "depthTexture";
		PPparams[1].ppTextures = &pDepthTexture->pTexture;
		updateDescriptorSet(pRenderer, 3, pVolumetricCloudsDescriptorSetGraphics[0], 2, PPparams);
	}
	// Composite
	{
		DescriptorData Presentpparams[5] = {};
		Presentpparams[0].pName = "g_PostProcessedTexture";
		Presentpparams[0].ppTextures = &pPostProcessRT->pTexture;
		Presentpparams[1].pName = "depthTexture";
		Presentpparams[1].ppTextures = &pDepthTexture->pTexture;
		Presentpparams[2].pName = "g_PrevVolumetricCloudTexture";
		Presentpparams[2].ppTextures = &phighResCloudRT->pTexture;
		updateDescriptorSet(pRenderer, 5, pVolumetricCloudsDescriptorSetGraphics[0], 3, Presentpparams);
	}
	// Add Godray
	{
		DescriptorData Presentpparams[4] = {};
		Presentpparams[0].pName = "g_GodrayTexture";
		Presentpparams[0].ppTextures = &pGodrayRT->pTexture;
		//Presentpparams[1].pName = "depthTexture";
		//Presentpparams[1].ppTextures = &pOriginDepthTexture;
		updateDescriptorSet(pRenderer, 4, pVolumetricCloudsDescriptorSetGraphics[0], 1, Presentpparams);
	}

	return true;
}

void VolumetricClouds::Unload()
{
	removePipeline(pRenderer, pPostProcessPipeline);
	removePipeline(pRenderer, pPostProcessWithBlurPipeline);
	removePipeline(pRenderer, pCompositeOverlayPipeline);
	removePipeline(pRenderer, pCompositePipeline);
	removePipeline(pRenderer, pVolumetricCloudPipeline);
#if USE_VC_FRAGMENTSHADER
	removePipeline(pRenderer, pVolumetricCloudWithDepthPipeline);
	removePipeline(pRenderer, pVolumetricCloud2ndPipeline);
	removePipeline(pRenderer, pVolumetricCloud2ndWithDepthPipeline);
#else
	removePipeline(pRenderer, pVolumetricCloudCompPipeline);
	removePipeline(pRenderer, pVolumetricCloud2ndCompPipeline);
	removePipeline(pRenderer, pVolumetricCloudWithDepthCompPipeline);
	removePipeline(pRenderer, pVolumetricCloud2ndWithDepthCompPipeline);
#endif
	removePipeline(pRenderer, pReprojectionPipeline);

	removePipeline(pRenderer, pGodrayPipeline);
	removePipeline(pRenderer, pGodrayAddPipeline);
	removePipeline(pRenderer, pCopyTexturePipeline);

	removePipeline(pRenderer, pGenHiZMipmapPipeline);
	removePipeline(pRenderer, pCopyRTPipeline);

	removePipeline(pRenderer, pGenHiZMipmapPRPipeline);
	removePipeline(pRenderer, pHorizontalBlurPipeline);
	removePipeline(pRenderer, pVerticalBlurPipeline);
	//removePipeline(pRenderer, pReprojectionCompPipeline);
	//removePipeline(pRenderer, pCastShadowPipeline);

	removeResource(pTriangularScreenVertexWithMiscBuffer);
	removeResource(pHBlurTex);
	removeResource(pVBlurTex);

	removeResource(pHiZDepthBuffer);
	removeResource(pHiZDepthBuffer2);
	removeResource(pHiZDepthBufferX);
	removeResource(pSavePrevTexture);

	removeRenderTarget(pRenderer, plowResCloudRT);
	removeRenderTarget(pRenderer, phighResCloudRT);
	removeRenderTarget(pRenderer, pPostProcessRT);
	removeRenderTarget(pRenderer, pGodrayRT);
	removeRenderTarget(pRenderer, pCastShadowRT);

	removeResource(plowResCloudTexture);
	removeResource(phighResCloudTexture);
}

void VolumetricClouds::Draw(Cmd* cmd)
{
	cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Volumetric Clouds + Post Process");

	{
		cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Volumetric Clouds");

		TextureBarrier barriers000[] = {
#if USE_LOD_DEPTH
			{ pHiZDepthBufferX, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS }
#else
			{ pHiZDepthBuffer, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS },
#endif
		};
		cmdResourceBarrier(cmd, 0, NULL, 1, barriers000, 0, NULL);

		const uint32_t* pThreadGroupSize = NULL;

		{
#if USE_DEPTH_CULLING

#if USE_LOD_DEPTH


			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Lodded Z DepthBuffer");

			cmdBindPipeline(cmd, pGenHiZMipmapPRPipeline);
			cmdBindDescriptorSet(cmd, 0, pVolumetricCloudsDescriptorSetCompute[0]);
#if !USE_VC_FRAGMENTSHADER
			cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetCompute[1]);
#endif
			pThreadGroupSize = pGenHiZMipmapPRShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
			cmdDispatch(cmd, HiZDepthDesc.mWidth, HiZDepthDesc.mHeight, 1);

			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

#else
			struct Data
			{
				uint mip;
			} data = { 0 };


			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Hi-Z DepthBuffer");

			// Copy Texture
			cmdBindPipeline(cmd, pCopyTexturePipeline);

			data.mip = 0;

			DescriptorData mipParams[3] = {};
			mipParams[0].pName = "RootConstant";
			mipParams[0].pRootConstant = &data;
			mipParams[1].pName = "SrcTexture";
			mipParams[1].ppTextures = &pDepthBuffer->pTexture;
			mipParams[2].pName = "DstTexture";
			mipParams[2].ppTextures = &pHiZDepthBuffer;
			mipParams[2].mUAVMipSlice = 0;
			cmdBindDescriptors(cmd, pVolumetricCloudsDescriptorBinder, 3, mipParams);

			pThreadGroupSize = pCopyTextureShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
			cmdDispatch(cmd, (pDepthBuffer->pTexture->mWidth / pThreadGroupSize[0]) + 1, pDepthBuffer->pTexture->mHeight / pThreadGroupSize[1] + 1, 1);


			for (uint i = 0; i < pHiZDepthBuffer->mMipLevels - 1; i++)
			{

				TextureBarrier barriers0[] = {
				{ pHiZDepthBuffer, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHiZDepthBuffer2, RESOURCE_STATE_UNORDERED_ACCESS }
				};

				cmdResourceBarrier(cmd, 0, NULL, 2, barriers0);

				//Gen mipmap
				cmdBindPipeline(cmd, pGenHiZMipmapPipeline);

				DescriptorData mipParams2[4] = {};

				data.mip = i;
				mipParams2[0].pName = "RootConstant";
				mipParams2[0].pRootConstant = &data;
				mipParams2[1].pName = "SrcTexture";
				mipParams2[1].ppTextures = &pHiZDepthBuffer;
				mipParams2[2].pName = "DstTexture";
				mipParams2[2].ppTextures = &pHiZDepthBuffer2;
				mipParams2[2].mUAVMipSlice = i + 1;

				cmdBindDescriptors(cmd, pVolumetricCloudsDescriptorBinder, 3, mipParams2);

				uint power = (uint)pow(2.0f, float(i + 1));
				uint newWidth = pDepthBuffer->pTexture->mWidth / power;
				uint newHeight = pDepthBuffer->pTexture->mHeight / power;

				const uint32_t* pThreadGroupSize = pGenHiZMipmapShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;

				cmdDispatch(cmd, (newWidth / pThreadGroupSize[0]) + 1, newHeight / pThreadGroupSize[1] + 1, 1);


				TextureBarrier barriers1[] = {
				{ pHiZDepthBuffer2, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHiZDepthBuffer, RESOURCE_STATE_UNORDERED_ACCESS }
				};

				cmdResourceBarrier(cmd, 0, NULL, 2, barriers1);

				// Copy mipmap Texture
				cmdBindPipeline(cmd, pCopyTexturePipeline);


				data.mip = i + 1;
				mipParams[0].pName = "RootConstant";
				mipParams[0].pRootConstant = &data;
				mipParams[1].pName = "SrcTexture";
				mipParams[1].ppTextures = &pHiZDepthBuffer2;
				mipParams[2].pName = "DstTexture";
				mipParams[2].ppTextures = &pHiZDepthBuffer;
				mipParams[2].mUAVMipSlice = i + 1;
				cmdBindDescriptors(cmd, pVolumetricCloudsDescriptorBinder, 3, mipParams);

				pThreadGroupSize = pCopyTextureShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
				cmdDispatch(cmd, (newWidth / pThreadGroupSize[0]) + 1, newHeight / pThreadGroupSize[1] + 1, 1);
			}

			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
#endif

#endif
		}

		{
#if USE_LOD_DEPTH
			Texture* HiZedDepthTexture = pHiZDepthBufferX;
#else
			Texture* HiZedDepthTexture = pHiZDepthBuffer;
#endif

#if USE_VC_FRAGMENTSHADER
			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Clouds (Fragment Shaders Pipeline)");

			TextureBarrier barriers0[] = {
				{ HiZedDepthTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE }
			};

			RenderTarget* pRenderTarget = plowResCloudRT;
			RenderTargetBarrier rtBarriers0[] = {
				{ pRenderTarget, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
			};
			cmdResourceBarrier(cmd, 0, NULL, 1, barriers0, 1, rtBarriers0);

			// simply record the screen cleaning command
			LoadActionsDesc loadActions = {};
			loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
			loadActions.mClearColorValues[0].r = 0.0f;
			loadActions.mClearColorValues[0].g = 0.0f;
			loadActions.mClearColorValues[0].b = 0.0f;
			loadActions.mClearColorValues[0].a = 0.0f;

			cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

			if (gAppSettings.m_EnabledDepthCulling)
			{
				if (gAppSettings.m_Enabled2ndLayer)
				{
					cmdBindPipeline(cmd, pVolumetricCloud2ndPipeline);
				}
				else
				{
					cmdBindPipeline(cmd, pVolumetricCloudPipeline);
				}

				cmdBindDescriptorSet(cmd, 6, pVolumetricCloudsDescriptorSetGraphics[0]);
			}
			else
			{
				if (gAppSettings.m_Enabled2ndLayer)
				{
					cmdBindPipeline(cmd, pVolumetricCloud2ndWithDepthPipeline);
				}
				else
				{
					cmdBindPipeline(cmd, pVolumetricCloudWithDepthPipeline);
				}

				cmdBindDescriptorSet(cmd, 7, pVolumetricCloudsDescriptorSetGraphics[0]);
			}

			cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
			
			const uint32_t miscStride = sizeof(float) * 4;
			cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexWithMiscBuffer, &miscStride, NULL);
			cmdDraw(cmd, 3, 0);

			cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);
#else
			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Clouds (Compute Shaders Pipeline)");

			TextureBarrier barriers0[] = {
				{ HiZedDepthTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
				{ plowResCloudTexture, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS },
			};

			cmdResourceBarrier(cmd, 0, NULL, 2, barriers0, 0, NULL);

			if (gAppSettings.m_EnabledDepthCulling)
			{
				if(gAppSettings.m_Enabled2ndLayer)
				{
					cmdBindPipeline(cmd, pVolumetricCloud2ndCompPipeline);
					pThreadGroupSize = pVolumetricCloud2ndCompShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
				}
				else
				{
					cmdBindPipeline(cmd, pVolumetricCloudCompPipeline);
					pThreadGroupSize = pVolumetricCloudCompShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
				}					

				cmdBindDescriptorSet(cmd, 4, pVolumetricCloudsDescriptorSetCompute[0]);
			}
			else
			{
				if (gAppSettings.m_Enabled2ndLayer)
				{
					cmdBindPipeline(cmd, pVolumetricCloud2ndWithDepthCompPipeline);
					pThreadGroupSize = pVolumetricCloud2ndWithDepthCompShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
				}
				else
				{
					cmdBindPipeline(cmd, pVolumetricCloudWithDepthCompPipeline);
					pThreadGroupSize = pVolumetricCloudWithDepthCompShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
				}			

				cmdBindDescriptorSet(cmd, 5, pVolumetricCloudsDescriptorSetCompute[0]);
			}

			
			cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetCompute[1]);

			cmdDispatch(cmd, (uint32_t)ceil((float)lowResTextureDesc.mWidth / (float)pThreadGroupSize[0]), (uint32_t)ceil((float)lowResTextureDesc.mHeight / (float)pThreadGroupSize[1]), 1);
#endif
			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
		}

		{
			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Reprojection");

			/////////////////////////////////////////////////////////////////////////////
			RenderTargetBarrier rtBarriersForReprojection[] = {
				{ phighResCloudRT, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
				{ plowResCloudRT, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE },
			};

#if USE_VC_FRAGMENTSHADER
			cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, rtBarriersForReprojection);
#else
			TextureBarrier barriersForReprojection[] = {
				{ plowResCloudTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
			};
			cmdResourceBarrier(cmd, 0, NULL, 1, barriersForReprojection, 1, rtBarriersForReprojection);
#endif

#if USE_RP_FRAGMENTSHADER

			RenderTarget* pRenderTarget = phighResCloudRT;

			LoadActionsDesc loadActions = {};
			loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
			//loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
			loadActions.mClearColorValues[0].r = 0.0f;
			loadActions.mClearColorValues[0].g = 0.0f;
			loadActions.mClearColorValues[0].b = 0.0f;
			loadActions.mClearColorValues[0].a = 0.0f;
			//loadActions.mClearDepth = pDepthBuffer->mClearValue;

			cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

			cmdBindPipeline(cmd, pReprojectionPipeline);
			cmdBindDescriptorSet(cmd, 0, pVolumetricCloudsDescriptorSetGraphics[0]);
			cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);

			const uint32_t miscStride = sizeof(float) * 4;
			cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexWithMiscBuffer, &miscStride, NULL);
			cmdDraw(cmd, 3, 0);
			cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);

			// Need to reset g_PrevFrameTexture descriptor slot before we can use it as UAV in the compute root signature
			cmdBindDescriptorSet(cmd, 7, pVolumetricCloudsDescriptorSetGraphics[0]);
#else

			cmdBindPipeline(cmd, pReprojectionCompPipeline);

			DescriptorData RPparams[6] = {};
			RPparams[0].pName = "LowResCloudTexture";
			RPparams[0].ppTextures = &plowResCloudTexture;

			RPparams[1].pName = "g_PrevFrameTexture";
			RPparams[1].ppTextures = &pSavePrevTexture;

			RPparams[2].pName = "BilinearClampSampler";
			RPparams[2].ppSamplers = &pBiClampSampler;
			RPparams[3].pName = "PointClampSampler";
			RPparams[3].ppSamplers = &pPointClampSampler;
			RPparams[4].pName = "uniformGlobalInfoRootConstant";
			RPparams[4].pRootConstant = &VolumetricCloudsData;

			RPparams[5].pName = "DstTexture";
			RPparams[5].ppTextures = &phighResCloudTexture;

			cmdBindDescriptors(cmd, pVolumetricCloudsDescriptorBinder, 6, RPparams);

			pThreadGroupSize = pReprojectionCompShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
			cmdDispatch(cmd, (phighResCloudTexture->mWidth / pThreadGroupSize[0]) + 1, (phighResCloudTexture->mHeight / pThreadGroupSize[1]) + 1, 1);

#endif

			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
		}

		//cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);


		{
			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Save Current RenderTarget");

			TextureBarrier barriersForSaveRT[] = {
				{ pSavePrevTexture, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS },
			};
			RenderTargetBarrier rtBarriersForSaveRT[] = {
				{ phighResCloudRT, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE },
			};

			cmdResourceBarrier(cmd, 0, NULL, 1, barriersForSaveRT, 1, rtBarriersForSaveRT);

			cmdBindPipeline(cmd, pCopyRTPipeline);
			cmdBindDescriptorSet(cmd, 1, pVolumetricCloudsDescriptorSetCompute[0]);

			pThreadGroupSize = pCopyRTShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
			cmdDispatch(cmd, (uint32_t)ceil(phighResCloudRT->mWidth / pThreadGroupSize[0]), (uint32_t)ceil(phighResCloudRT->mHeight / pThreadGroupSize[1]), 1);
			
			TextureBarrier barriersForSaveRTEnd[] = {
			{ pSavePrevTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
			};

			cmdResourceBarrier(cmd, 0, NULL, 1, barriersForSaveRTEnd, 0, NULL);
			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
		}

		if (gAppSettings.m_EnableBlur)
		{
			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Denoise - Blur");

			TextureBarrier barriersForHBlur[] = {
				{ pHBlurTex, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS },
			};
			cmdResourceBarrier(cmd, 0, NULL, 1, barriersForHBlur, 0, NULL);

			struct Data
			{
				uint width;
				uint height;
			} data = { 0 };

			data.width = blurTextureDesc.mWidth;
			data.height = blurTextureDesc.mHeight;

			cmdBindPipeline(cmd, pHorizontalBlurPipeline);
			cmdBindDescriptorSet(cmd, 2, pVolumetricCloudsDescriptorSetCompute[0]);
			cmdDispatch(cmd, 1, blurTextureDesc.mHeight, 1);

			TextureBarrier barriersForVBlur[] = {
				{ pHBlurTex, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
				{ pVBlurTex, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS }
			};

			cmdResourceBarrier(cmd, 0, NULL, 2, barriersForVBlur, 0, NULL);

			cmdBindPipeline(cmd, pVerticalBlurPipeline);
			cmdBindDescriptorSet(cmd, 3, pVolumetricCloudsDescriptorSetCompute[0]);
			cmdDispatch(cmd, blurTextureDesc.mWidth, 1, 1);

			TextureBarrier barriersForEndBlur[] = {
				{ pVBlurTex, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, NULL, 1, barriersForEndBlur, 0, NULL);

			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
		}

		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	}

	// PostProcess
	{
		cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "PostProcess");


		RenderTarget* pRenderTarget = pPostProcessRT;

		RenderTargetBarrier barriersForPostProcess[] = {
			{ pRenderTarget, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
			{ pSceneColorTexture, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, barriersForPostProcess);

		LoadActionsDesc loadActions = {};
		loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
		//loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
		loadActions.mClearColorValues[0].r = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].b = 0.0f;
		loadActions.mClearColorValues[0].a = 0.0f;
		//loadActions.mClearDepth = pDepthBuffer->mClearValue;

		cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

		if (gAppSettings.m_EnableBlur)
		{
			cmdBindPipeline(cmd, pPostProcessWithBlurPipeline);
			cmdBindDescriptorSet(cmd, 2, pVolumetricCloudsDescriptorSetGraphics[0]);
		}
		else
		{
			cmdBindPipeline(cmd, pPostProcessPipeline);
			cmdBindDescriptorSet(cmd, 1, pVolumetricCloudsDescriptorSetGraphics[0]);
		}

		cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
		cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexBuffer, &gTriangleVbStride, NULL);
		cmdDraw(cmd, 3, 0);

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	}
	// PostProcess
	cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

	// Render Godray
	cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Render Godray + Composition");

	RenderTargetBarrier barriersForGodray[] = {
		{ pGodrayRT, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET }
	};
	cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersForGodray);

	if (gAppSettings.m_EnabledGodray)
	{
		cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Render Godray");

		cmdBindRenderTargets(cmd, 1, &pGodrayRT, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pGodrayRT->mWidth, (float)pGodrayRT->mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pGodrayRT->mWidth, pGodrayRT->mHeight);

		cmdBindPipeline(cmd, pGodrayPipeline);
		cmdBindDescriptorSet(cmd, 3, pVolumetricCloudsDescriptorSetGraphics[0]);
		cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
		cmdDraw(cmd, 3, 0);

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	}

	RenderTargetBarrier barriersForGodrayEnd[] = {
		{ pGodrayRT, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE }
	};
	cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersForGodrayEnd);
	// Render Godray


	/*
	{
		cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Cast Shadow", true);

		pRenderTarget = pCastShadowRT;

		TextureBarrier barriers653[] = {
			{ pRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriers653, false);

		cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

		cmdBindPipeline(cmd, pCastShadowPipeline);

		DescriptorData CSRparams[6] = {};
		CSRparams[0].pName = "depthTexture";
		CSRparams[0].ppTextures = &pDepthBuffer->pTexture;
		CSRparams[1].pName = "noiseTexture";
		CSRparams[1].ppTextures = &pWeatherTexture;

		CSRparams[2].pName = "PointClampSampler";
		CSRparams[2].ppSamplers = &pPointClampSampler;
		CSRparams[3].pName = "BilinearSampler";
		CSRparams[3].ppSamplers = &pBilinearSampler;

		CSRparams[4].pName = "VolumetricCloudsCBufferRootConstant";
		CSRparams[4].ppBuffers = &BufferUniformSetting[gFrameIndex];
		//CSRparams[4].pName = "uniformGlobalInfoRootConstant";
		//CSRparams[4].pRootConstant = &VolumetricCloudsData;

		CSRparams[5].pName = "uniformSetting";
		CSRparams[5].ppBuffers = &BufferUniformSetting[gFrameIndex];

		cmdBindDescriptors(cmd, pCastShadowRootSignature, 6, CSRparams);

		cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexBuffer, NULL);
		cmdDraw(cmd, 3, 0);

	cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	}
	*/

	// Composite
	
	{
		cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Composite");

		RenderTarget* pRenderTarget = *pFinalRT;

		RenderTargetBarrier barriersComposition[] = {
			{ pRenderTarget, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET }
			,{ pPostProcessRT, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, barriersComposition);

		cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

		cmdBindPipeline(cmd, pCompositePipeline);
		cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
		cmdBindDescriptorSet(cmd, 5, pVolumetricCloudsDescriptorSetGraphics[0]);
		cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexBuffer, &gTriangleVbStride, NULL);
		cmdDraw(cmd, 3, 0);

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);


		RenderTargetBarrier barriersCompositionEnd[] = {
			{ pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersCompositionEnd);

		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	}
	
	// Composite

	// Add Godray
	if (gAppSettings.m_EnabledGodray)
	{
		cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Add Godray");

		RenderTarget* pRenderTarget = *pFinalRT;

		RenderTargetBarrier barriersAddGodray[] = {
			{ pRenderTarget, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET }
		};
		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersAddGodray);

		cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

		cmdBindPipeline(cmd, pGodrayAddPipeline);
		cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
		cmdBindDescriptorSet(cmd, 4, pVolumetricCloudsDescriptorSetGraphics[0]);

		cmdDraw(cmd, 3, 0);

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		RenderTargetBarrier barriersAddGodrayEnd[] = {
			{ pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersAddGodrayEnd);

		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	}
	// Add Godray
	cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
}

mat4 GetViewCameraOffsetY(ICameraController* pCamera, float heightOffset)
{
	vec2 viewRotation = pCamera->getRotationXY();
	vec3 viewPosition = pCamera->getViewPosition() + vec3(0.0f, heightOffset, 0.0f);
	mat4 r = mat4::rotationXY(-viewRotation.getX(), -viewRotation.getY());
	vec4 t = r * vec4(-viewPosition, 1.0f);
	r.setTranslation(t.getXYZ());
	return r;
}

vec2 GetDirectionXZ(float azimuth)
{
	vec2  dir;
	float angle_degs = azimuth * float(PI / 180.0);
	dir.setX(cos(angle_degs));
	dir.setY(sin(angle_degs));
	return dir;
}

void VolumetricClouds::Update(float deltaTime)
{
	g_currentTime += deltaTime * 1000.0f;
	volumetricCloudsCB.TimeAndScreenSize = vec4(g_currentTime, g_currentTime, (float)((mWidth / gDownsampledCloudSize) & (~31)), (float)((mHeight / gDownsampledCloudSize) & (~31)));

	mat4 cloudViewMat_1st = pCameraController->getViewMatrix();
	mat4 cloudPrevViewMat_1st = prevView;

	volumetricCloudsCB.m_DataPerEye[0].m_WorldToProjMat = projMat * cloudViewMat_1st;
	volumetricCloudsCB.m_DataPerEye[0].m_PrevWorldToProjMat = projMat * cloudPrevViewMat_1st;

	volumetricCloudsCB.m_DataPerEye[0].m_ViewToWorldMat = inverse(cloudViewMat_1st);
	volumetricCloudsCB.m_DataPerEye[0].m_ProjToWorldMat = volumetricCloudsCB.m_DataPerEye[0].m_ViewToWorldMat * inverse(projMat);

	volumetricCloudsCB.m_DataPerEye[0].m_LightToProjMat = mat4::identity();

	volumetricCloudsCB.m_JitterX = (uint32_t)offset[g_LowResFrameIndex].x;
	volumetricCloudsCB.m_JitterY = (uint32_t)offset[g_LowResFrameIndex].y;

	vec2 WeatherTexOffsets = GetDirectionXZ(gAppSettings.WeatherTextureAzimuth);
	volumetricCloudsCB.m_DataPerLayer[0].WeatherTextureOffsetX = WeatherTexOffsets.getX() * gAppSettings.WeatherTextureDistance;
	volumetricCloudsCB.m_DataPerLayer[0].WeatherTextureOffsetZ = WeatherTexOffsets.getY() * gAppSettings.WeatherTextureDistance;

	vec2 WeatherTexOffsets_2nd = GetDirectionXZ(gAppSettings.WeatherTextureAzimuth_2nd);
	volumetricCloudsCB.m_DataPerLayer[1].WeatherTextureOffsetX = WeatherTexOffsets_2nd.getX() * gAppSettings.WeatherTextureDistance_2nd;
	volumetricCloudsCB.m_DataPerLayer[1].WeatherTextureOffsetZ = WeatherTexOffsets_2nd.getY() * gAppSettings.WeatherTextureDistance_2nd;

	vec2 windXZ = GetDirectionXZ(gAppSettings.m_WindAzimuth);
	vec2 flowXZ = GetDirectionXZ(gAppSettings.m_NoiseFlowAzimuth);

	vec2 windXZ_2nd = GetDirectionXZ(gAppSettings.m_WindAzimuth_2nd);
	vec2 flowXZ_2nd = GetDirectionXZ(gAppSettings.m_NoiseFlowAzimuth_2nd);

	volumetricCloudsCB.EarthRadius = gAppSettings.m_EarthRadiusScale * 6360000.0f;
	volumetricCloudsCB.EarthCenter = vec4(0.0f, -volumetricCloudsCB.EarthRadius, 0.0f, 0.0f);
	volumetricCloudsCB.m_DataPerLayer[0].CloudsLayerStart = gAppSettings.m_CloudsLayerStart;
	volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart = volumetricCloudsCB.EarthRadius + volumetricCloudsCB.m_DataPerLayer[0].CloudsLayerStart;
	volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart2 = volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart * volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart;
	volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd = volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart + gAppSettings.m_LayerThickness;
	volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd2 = volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd * volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd;

	//volumetricCloudsCB.EarthRadius_2nd = gAppSettings.m_EarthRadius_2nd;
	//volumetricCloudsCB.EarthCenter_2nd = vec4(0.0f, -volumetricCloudsCB.EarthRadius_2nd, 0.0f, 0.0f);
	volumetricCloudsCB.m_DataPerLayer[1].CloudsLayerStart = gAppSettings.m_CloudsLayerStart_2nd;
	volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart = volumetricCloudsCB.EarthRadius + volumetricCloudsCB.m_DataPerLayer[1].CloudsLayerStart;
	volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart2 = volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart * volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart;
	volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerEnd = volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart + gAppSettings.m_LayerThickness_2nd;
	volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerEnd2 = volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerEnd * volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerEnd;

	volumetricCloudsCB.m_MaxSampleDistance = gAppSettings.m_MaxSampleDistance;


	float windIntensity = gAppSettings.m_EnabledRotation ? 0.0f : gAppSettings.m_WindIntensity * (float)deltaTime * 100.0f;
	float flowIntensity = gAppSettings.m_NoiseFlowIntensity * (float)deltaTime * 100.0f;

	g_StandardPosition += vec4(windXZ.getX() * windIntensity, windXZ.getY() * windIntensity, flowXZ.getX() * flowIntensity, flowXZ.getY() * flowIntensity);

	volumetricCloudsCB.m_DataPerLayer[0].StandardPosition = g_StandardPosition;

	float windIntensity_2nd = gAppSettings.m_EnabledRotation_2nd ? 0.0f : gAppSettings.m_WindIntensity_2nd * (float)deltaTime * 100.0f;
	float flowIntensity_2nd = gAppSettings.m_NoiseFlowIntensity_2nd * (float)deltaTime * 100.0f;

	g_StandardPosition_2nd += vec4(windXZ_2nd.getX() * windIntensity_2nd, windXZ_2nd.getY() * windIntensity_2nd, flowXZ_2nd.getX() * flowIntensity_2nd, flowXZ_2nd.getY() * flowIntensity_2nd);

	volumetricCloudsCB.m_DataPerLayer[1].StandardPosition = g_StandardPosition_2nd;

	vec4 lightDir = vec4(f3Tov3(LightDirection));

	volumetricCloudsCB.Test00 = lightDir.getY() < 0.0f ? 0.0f : 1.0f;

	lightDir = lightDir.getY() < 0.0f ? -lightDir : lightDir;
	lightDir.setW(gAppSettings.m_TransStepSize);

	volumetricCloudsCB.lightDirection = lightDir;


	volumetricCloudsCB.Test01 = 0.0f;

	//gAppSettings.m_CustomColor
	vec4 customColor;

	uint32_t red = (gAppSettings.m_CustomColor & 0xFF000000) >> 24;
	uint32_t green = (gAppSettings.m_CustomColor & 0x00FF0000) >> 16;
	uint32_t blue = (gAppSettings.m_CustomColor & 0x0000FF00) >> 8;

	customColor = vec4((float)red / 255.0f, (float)green / 255.0f, (float)blue / 255.0f, gAppSettings.m_CustomColorIntensity);

	volumetricCloudsCB.lightColorAndIntensity = lerp(gAppSettings.m_CustomColorBlendFactor, f4Tov4(LightColorAndIntensity), customColor);

	volumetricCloudsCB.m_DataPerEye[0].cameraPosition = vec4(pCameraController->getViewPosition());
	volumetricCloudsCB.m_DataPerEye[0].cameraPosition.setW(1.0f);
	volumetricCloudsCB.m_DataPerEye[1].cameraPosition = vec4(pCameraController->getViewPosition());
	volumetricCloudsCB.m_DataPerEye[1].cameraPosition.setW(1.0f);

	volumetricCloudsCB.m_DataPerLayer[0].LayerThickness = gAppSettings.m_LayerThickness;
	volumetricCloudsCB.m_DataPerLayer[1].LayerThickness = gAppSettings.m_LayerThickness_2nd;

	if (gAppSettings.m_EnabledLodDepth && gAppSettings.m_EnabledDepthCulling)
	{
		volumetricCloudsCB.DepthMapWidth = HiZDepthDesc.mWidth;
		volumetricCloudsCB.DepthMapHeight = HiZDepthDesc.mHeight;
	}
	else
	{
		volumetricCloudsCB.DepthMapWidth = pLinearDepthTexture->mWidth;
		volumetricCloudsCB.DepthMapHeight = pLinearDepthTexture->mHeight;
	}

	volumetricCloudsCB.m_CorrectU = (float)(volumetricCloudsCB.m_JitterX) / (float)((mWidth / gDownsampledCloudSize) & (~31));
	volumetricCloudsCB.m_CorrectV = (float)(volumetricCloudsCB.m_JitterY) / (float)((mHeight / gDownsampledCloudSize) & (~31));

	volumetricCloudsCB.MIN_ITERATION_COUNT = gAppSettings.m_MinSampleCount;
	volumetricCloudsCB.MAX_ITERATION_COUNT = gAppSettings.m_MaxSampleCount;

	volumetricCloudsCB.m_UseRandomSeed = gAppSettings.m_EnabledTemporalRayOffset ? 1.0f : 0.0f;
	volumetricCloudsCB.m_StepSize = vec4(gAppSettings.m_MinStepSize, gAppSettings.m_MaxStepSize, 0.0f, 0.0f);

	//Cloud
	volumetricCloudsCB.m_DataPerLayer[0].CloudDensity = gAppSettings.m_CloudDensity;
	volumetricCloudsCB.m_DataPerLayer[1].CloudDensity = gAppSettings.m_CloudDensity_2nd;

	volumetricCloudsCB.m_DataPerLayer[0].CloudCoverage = gAppSettings.m_CloudCoverageModifier * gAppSettings.m_CloudCoverageModifier * gAppSettings.m_CloudCoverageModifier;
	volumetricCloudsCB.m_DataPerLayer[1].CloudCoverage = gAppSettings.m_CloudCoverageModifier_2nd * gAppSettings.m_CloudCoverageModifier_2nd * gAppSettings.m_CloudCoverageModifier_2nd;

	volumetricCloudsCB.m_DataPerLayer[0].CloudType = gAppSettings.m_CloudTypeModifier * gAppSettings.m_CloudTypeModifier * gAppSettings.m_CloudTypeModifier;
	volumetricCloudsCB.m_DataPerLayer[1].CloudType = gAppSettings.m_CloudTypeModifier_2nd * gAppSettings.m_CloudTypeModifier_2nd * gAppSettings.m_CloudTypeModifier_2nd;

	volumetricCloudsCB.m_DataPerLayer[0].CloudTopOffset = gAppSettings.m_CloudTopOffset;
	volumetricCloudsCB.m_DataPerLayer[1].CloudTopOffset = gAppSettings.m_CloudTopOffset_2nd;


	//Modeling
	volumetricCloudsCB.m_DataPerLayer[0].CloudSize = gAppSettings.m_CloudSize;
	volumetricCloudsCB.m_DataPerLayer[1].CloudSize = gAppSettings.m_CloudSize_2nd;

	volumetricCloudsCB.m_DataPerLayer[0].BaseShapeTiling = gAppSettings.m_BaseTile;
	volumetricCloudsCB.m_DataPerLayer[1].BaseShapeTiling = gAppSettings.m_BaseTile_2nd;

	volumetricCloudsCB.m_DataPerLayer[0].DetailShapeTiling = gAppSettings.m_DetailTile;
	volumetricCloudsCB.m_DataPerLayer[1].DetailShapeTiling = gAppSettings.m_DetailTile_2nd;

	volumetricCloudsCB.m_DataPerLayer[0].DetailStrenth = gAppSettings.m_DetailStrength;
	volumetricCloudsCB.m_DataPerLayer[1].DetailStrenth = gAppSettings.m_DetailStrength_2nd;

	volumetricCloudsCB.m_DataPerLayer[0].CurlTextureTiling = gAppSettings.m_CurlTile;
	volumetricCloudsCB.m_DataPerLayer[1].CurlTextureTiling = gAppSettings.m_CurlTile_2nd;

	volumetricCloudsCB.m_DataPerLayer[0].CurlStrenth = gAppSettings.m_CurlStrength;
	volumetricCloudsCB.m_DataPerLayer[1].CurlStrenth = gAppSettings.m_CurlStrength_2nd;

	volumetricCloudsCB.m_DataPerLayer[0].WeatherTextureSize = gAppSettings.m_WeatherTexSize;
	volumetricCloudsCB.m_DataPerLayer[1].WeatherTextureSize = gAppSettings.m_WeatherTexSize_2nd;

	volumetricCloudsCB.m_DataPerLayer[0].AnvilBias = gAppSettings.m_AnvilBias;
	volumetricCloudsCB.m_DataPerLayer[1].AnvilBias = gAppSettings.m_AnvilBias_2nd;

	//Lighting
	volumetricCloudsCB.Contrast = gAppSettings.m_Contrast;
	
	volumetricCloudsCB.Eccentricity = gAppSettings.m_Eccentricity;
	volumetricCloudsCB.CloudBrightness = gAppSettings.m_CloudBrightness;
	volumetricCloudsCB.BackgroundBlendFactor = gAppSettings.m_BackgroundBlendFactor;
	volumetricCloudsCB.Precipitation = gAppSettings.m_Precipitation;

	float SilverIntensityCorrectionValue = (1.0f - abs(volumetricCloudsCB.lightDirection.getY()));
	SilverIntensityCorrectionValue *= SilverIntensityCorrectionValue;

	volumetricCloudsCB.SilverliningIntensity = gAppSettings.m_SilverIntensity * SilverIntensityCorrectionValue;
	volumetricCloudsCB.SilverliningSpread = gAppSettings.m_SilverSpread;

	g_ShadowInfo = vec4(gAppSettings.m_ShadowBrightness, gAppSettings.m_ShadowSpeed, gAppSettings.m_ShadowTiling, 0.0);

	//Wind
	volumetricCloudsCB.m_DataPerLayer[0].WindDirection = vec4(windXZ.getX(), 0.0, windXZ.getY(), gAppSettings.m_EnabledRotation ? 0.0f : gAppSettings.m_WindIntensity);
	volumetricCloudsCB.m_DataPerLayer[1].WindDirection = vec4(windXZ_2nd.getX(), 0.0, windXZ_2nd.getY(), gAppSettings.m_EnabledRotation_2nd ? 0.0f : gAppSettings.m_WindIntensity_2nd);

	if (gAppSettings.m_EnabledRotation)
	{
		vec2 RotationOffsets = GetDirectionXZ(gAppSettings.m_RotationPivotAzimuth);

		volumetricCloudsCB.m_DataPerLayer[0].RotationPivotOffsetX = RotationOffsets.getX() * gAppSettings.m_RotationPivotDistance;
		volumetricCloudsCB.m_DataPerLayer[0].RotationPivotOffsetZ = RotationOffsets.getY() * gAppSettings.m_RotationPivotDistance;
		volumetricCloudsCB.m_DataPerLayer[0].RotationAngle = degToRad(gAppSettings.m_RotationIntensity * g_currentTime * 0.001f);
	}
	else
	{
		volumetricCloudsCB.m_DataPerLayer[0].RotationPivotOffsetX = 0.0f;
		volumetricCloudsCB.m_DataPerLayer[0].RotationPivotOffsetZ = 0.0f;
		volumetricCloudsCB.m_DataPerLayer[0].RotationAngle = 0.0f;
	}

	if (gAppSettings.m_EnabledRotation_2nd)
	{
		vec2 RotationOffsets = GetDirectionXZ(gAppSettings.m_RotationPivotAzimuth_2nd);

		volumetricCloudsCB.m_DataPerLayer[1].RotationPivotOffsetX = RotationOffsets.getX() * gAppSettings.m_RotationPivotDistance_2nd;
		volumetricCloudsCB.m_DataPerLayer[1].RotationPivotOffsetZ = RotationOffsets.getY() * gAppSettings.m_RotationPivotDistance_2nd;
		volumetricCloudsCB.m_DataPerLayer[1].RotationAngle = degToRad(gAppSettings.m_RotationIntensity_2nd * g_currentTime * 0.001f);
	}
	else
	{
		volumetricCloudsCB.m_DataPerLayer[1].RotationPivotOffsetX = 0.0f;
		volumetricCloudsCB.m_DataPerLayer[1].RotationPivotOffsetZ = 0.0f;
		volumetricCloudsCB.m_DataPerLayer[1].RotationAngle = 0.0f;
	}

	volumetricCloudsCB.m_DataPerLayer[0].RisingVaporScale = gAppSettings.m_RisingVaporScale * 0.001f;
	volumetricCloudsCB.m_DataPerLayer[1].RisingVaporScale = gAppSettings.m_RisingVaporScale_2nd * 0.001f;

	volumetricCloudsCB.m_DataPerLayer[0].RisingVaporUpDirection = gAppSettings.m_RisingVaporUpDirection;
	volumetricCloudsCB.m_DataPerLayer[1].RisingVaporUpDirection = gAppSettings.m_RisingVaporUpDirection_2nd;

	volumetricCloudsCB.m_DataPerLayer[0].RisingVaporIntensity = gAppSettings.m_RisingVaporIntensity;
	volumetricCloudsCB.m_DataPerLayer[1].RisingVaporIntensity = gAppSettings.m_RisingVaporIntensity_2nd;

	volumetricCloudsCB.Random00 = ((float)rand() / (float)RAND_MAX);
	volumetricCloudsCB.CameraFarClip = TERRAIN_FAR;
	volumetricCloudsCB.EnabledDepthCulling = gAppSettings.m_EnabledDepthCulling ? 1 : 0;
	volumetricCloudsCB.EnabledLodDepthCulling = gAppSettings.m_EnabledLodDepth ? 1 : 0;

	//Godray
	volumetricCloudsCB.GodNumSamples = gAppSettings.m_GodNumSamples;
	volumetricCloudsCB.GodrayExposure = gAppSettings.m_Exposure;
	volumetricCloudsCB.GodrayDecay = gAppSettings.m_Decay;
	volumetricCloudsCB.GodrayDensity = gAppSettings.m_Density;
	volumetricCloudsCB.GodrayWeight = gAppSettings.m_Weight;

	/*
	volumetricCloudsCB.Test00 = gAppSettings.m_Test00;
	volumetricCloudsCB.Test01 = gAppSettings.m_Test01;
	volumetricCloudsCB.Test02 = gAppSettings.m_Test02;
	volumetricCloudsCB.Test03 = gAppSettings.m_Test03;
	*/

	prevView = cloudViewMat_1st;
}

void VolumetricClouds::Update(uint frameIndex)
{
	gFrameIndex = frameIndex;
	BufferUpdateDesc BufferUniformSettingDesc = { VolumetricCloudsCBuffer[frameIndex] };
	beginUpdateResource(&BufferUniformSettingDesc);
	*(VolumetricCloudsCB*)BufferUniformSettingDesc.pMappedData = volumetricCloudsCB;
	endUpdateResource(&BufferUniformSettingDesc, NULL);
	uint a = sizeof(volumetricCloudsCB);
	++a;
}

bool VolumetricClouds::AfterSubmit(uint currentFrameIndex)
{
	g_LowResFrameIndex = (g_LowResFrameIndex + 1) % (glowResBufferSize * glowResBufferSize);	

	if (prevDownSampling != DownSampling)
	{
		prevDownSampling = DownSampling;

		return true;		
	}
	else
		return false;
}

float4 VolumetricClouds::GetProjectionExtents(float fov, float aspect, float width, float height, float texelOffsetX, float texelOffsetY)
{
	//(PI / 180.0f) *
	float oneExtentY = tan(0.5f * fov);
	float oneExtentX = oneExtentY * aspect;
	float texelSizeX = oneExtentX / (0.5f * width);
	float texelSizeY = oneExtentY / (0.5f * height);
	float oneJitterX = texelSizeX * texelOffsetX;
	float oneJitterY = texelSizeY * texelOffsetY;

	return float4(oneExtentX, oneExtentY, oneJitterX, oneJitterY);// xy = frustum extents at distance 1, zw = jitter at distance 1
}

bool VolumetricClouds::AddHiZDepthBuffer()
{
	SyncToken token = {};

	HiZDepthDesc = {};
	HiZDepthDesc.mArraySize = 1;
	HiZDepthDesc.mFormat = TinyImageFormat_R16_SFLOAT;

	HiZDepthDesc.mWidth = mWidth & (~63);
	HiZDepthDesc.mHeight = mHeight & (~63);
	HiZDepthDesc.mDepth = 1;

	HiZDepthDesc.mMipLevels = 7;
	HiZDepthDesc.mSampleCount = SAMPLE_COUNT_1;
	//HiZDepthDesc.mSrgb = false;

	HiZDepthDesc.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	HiZDepthDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	HiZDepthDesc.pName = "HiZDepthBuffer A";
	TextureLoadDesc HiZDepthTextureDesc = {};
	HiZDepthTextureDesc.ppTexture = &pHiZDepthBuffer;
	HiZDepthTextureDesc.pDesc = &HiZDepthDesc;
	addResource(&HiZDepthTextureDesc, &token);


	HiZDepthDesc.pName = "HiZDepthBuffer B";
	HiZDepthTextureDesc.ppTexture = &pHiZDepthBuffer2;
	HiZDepthTextureDesc.pDesc = &HiZDepthDesc;
	addResource(&HiZDepthTextureDesc, &token);

	HiZDepthDesc.mMipLevels = 1;
	HiZDepthDesc.mWidth = (mWidth  & (~63)) / 32;
	HiZDepthDesc.mHeight = (mHeight & (~63)) / 32;
	HiZDepthDesc.pName = "HiZDepthBuffer X";
	HiZDepthTextureDesc.ppTexture = &pHiZDepthBufferX;
	HiZDepthTextureDesc.pDesc = &HiZDepthDesc;
	addResource(&HiZDepthTextureDesc, &token);

	waitForToken(&token);

	return pHiZDepthBuffer != NULL && pHiZDepthBuffer2 != NULL && pHiZDepthBufferX != NULL;
}

bool VolumetricClouds::AddVolumetricCloudsRenderTargets()
{
	RenderTargetDesc HighResCloudRT = {};
	HighResCloudRT.mArraySize = 1;
	HighResCloudRT.mDepth = 1;
	HighResCloudRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	HighResCloudRT.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
	HighResCloudRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	HighResCloudRT.mSampleCount = SAMPLE_COUNT_1;
	HighResCloudRT.mSampleQuality = 0;

	HighResCloudRT.mWidth = (mWidth / gDownsampledCloudSize) & (~31);   //make sure the width and height could be divided by 4, otherwise the low-res buffer can't be aligned with full-res buffer
	HighResCloudRT.mHeight = (mHeight / gDownsampledCloudSize) & (~31);
	HighResCloudRT.pName = "HighResCloudRT";
	addRenderTarget(pRenderer, &HighResCloudRT, &phighResCloudRT);

	RenderTargetDesc PostProcessRT = {};
	PostProcessRT.mArraySize = 1;
	PostProcessRT.mDepth = 1;
	PostProcessRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	PostProcessRT.mFormat = TinyImageFormat_R10G10B10A2_UNORM;
	PostProcessRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	PostProcessRT.mSampleCount = SAMPLE_COUNT_1;
	PostProcessRT.mSampleQuality = 0;

	PostProcessRT.mWidth = (mWidth) & (~63);
	PostProcessRT.mHeight = (mHeight) & (~63);
	PostProcessRT.pName = "PostProcessRT";
	addRenderTarget(pRenderer, &PostProcessRT, &pPostProcessRT);

	pRenderTargetsPostProcess[0] = pPostProcessRT;

	RenderTargetDesc CurrentCloudRT = {};
	CurrentCloudRT.mArraySize = 1;
	CurrentCloudRT.mDepth = 1;
	CurrentCloudRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	CurrentCloudRT.mFormat = HighResCloudRT.mFormat;
	CurrentCloudRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	CurrentCloudRT.mSampleCount = SAMPLE_COUNT_1;
	CurrentCloudRT.mSampleQuality = 0;
	CurrentCloudRT.pName = "CurrentCloudRT";
	CurrentCloudRT.mWidth = HighResCloudRT.mWidth / glowResBufferSize;
	CurrentCloudRT.mHeight = HighResCloudRT.mHeight / glowResBufferSize;
	addRenderTarget(pRenderer, &CurrentCloudRT, &plowResCloudRT);

	RenderTargetDesc GodrayRT = {};
	GodrayRT.mArraySize = 1;
	GodrayRT.mDepth = 1;
	GodrayRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	GodrayRT.mFormat = TinyImageFormat_R10G10B10A2_UNORM;
	GodrayRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	GodrayRT.mSampleCount = SAMPLE_COUNT_1;
	GodrayRT.mSampleQuality = 0;
	GodrayRT.mWidth = mWidth / godRayBufferSize;
	GodrayRT.mHeight = mHeight / godRayBufferSize;

	addRenderTarget(pRenderer, &GodrayRT, &pGodrayRT);

	RenderTargetDesc CastShadowRT = {};
	CastShadowRT.mArraySize = 1;
	CastShadowRT.mDepth = 1;
	CastShadowRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	CastShadowRT.mFormat = TinyImageFormat_R8_UNORM;
	CastShadowRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	CastShadowRT.mSampleCount = SAMPLE_COUNT_1;
	CastShadowRT.mSampleQuality = 0;
	CastShadowRT.mClearValue.r = 1.0f;

	CastShadowRT.mWidth = mWidth / gDownsampledCloudSize;
	CastShadowRT.mHeight = mHeight / gDownsampledCloudSize;

	addRenderTarget(pRenderer, &CastShadowRT, &pCastShadowRT);

	SyncToken token = {};

	TextureDesc SaveTextureDesc = {};
	SaveTextureDesc.mArraySize = 1;
	SaveTextureDesc.mFormat = HighResCloudRT.mFormat;
	
	SaveTextureDesc.mWidth = HighResCloudRT.mWidth;
	SaveTextureDesc.mHeight = HighResCloudRT.mHeight;
	SaveTextureDesc.mDepth = 1;

	SaveTextureDesc.mMipLevels = 1;
	SaveTextureDesc.mSampleCount = SAMPLE_COUNT_1;

	SaveTextureDesc.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	SaveTextureDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE | DESCRIPTOR_TYPE_RW_TEXTURE;
	SaveTextureDesc.pName = "SaveTexture";

	TextureLoadDesc SaveTextureLoadDesc = {};
	SaveTextureLoadDesc.ppTexture = &pSavePrevTexture;
	SaveTextureLoadDesc.pDesc = &SaveTextureDesc;
	addResource(&SaveTextureLoadDesc, &token);

	lowResTextureDesc.mArraySize = 1;
	lowResTextureDesc.mFormat = CurrentCloudRT.mFormat;

	lowResTextureDesc.mWidth = CurrentCloudRT.mWidth;
	lowResTextureDesc.mHeight = CurrentCloudRT.mHeight;
	lowResTextureDesc.mDepth = CurrentCloudRT.mDepth;

	lowResTextureDesc.mMipLevels = 1;
	lowResTextureDesc.mSampleCount = SAMPLE_COUNT_1;
	//lowResTextureDesc.mSrgb = false;

	lowResTextureDesc.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	lowResTextureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	lowResTextureDesc.pName = "low Res Texture";
	TextureLoadDesc lowResLoadDesc = {};
	lowResLoadDesc.ppTexture = &plowResCloudTexture;
	lowResLoadDesc.pDesc = &lowResTextureDesc;
	addResource(&lowResLoadDesc, &token);

	TextureDesc highResTextureDesc = {};
	highResTextureDesc.mArraySize = 1;
	highResTextureDesc.mFormat = HighResCloudRT.mFormat;

	highResTextureDesc.mWidth = HighResCloudRT.mWidth;
	highResTextureDesc.mHeight = HighResCloudRT.mHeight;
	highResTextureDesc.mDepth = HighResCloudRT.mDepth;

	highResTextureDesc.mMipLevels = 1;
	highResTextureDesc.mSampleCount = SAMPLE_COUNT_1;
	//highResTextureDesc.mSrgb = false;

	highResTextureDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
	highResTextureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	highResTextureDesc.pName = "high Res Texture";

	TextureLoadDesc highResLoadDesc = {};
	highResLoadDesc.ppTexture = &phighResCloudTexture;
	highResLoadDesc.pDesc = &highResTextureDesc;
	addResource(&highResLoadDesc, &token);



	blurTextureDesc.mArraySize = 1;
	blurTextureDesc.mFormat = SaveTextureDesc.mFormat;

	blurTextureDesc.mWidth = SaveTextureDesc.mWidth / 2;
	blurTextureDesc.mHeight = SaveTextureDesc.mHeight / 2;
	blurTextureDesc.mDepth = SaveTextureDesc.mDepth;

	blurTextureDesc.mMipLevels = 1;
	blurTextureDesc.mSampleCount = SAMPLE_COUNT_1;
	//blurTextureDesc.mSrgb = false;

	blurTextureDesc.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	blurTextureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	blurTextureDesc.pName = "H Blur Texture";


	TextureLoadDesc blurTextureLoadDesc = {};
	blurTextureLoadDesc.ppTexture = &pHBlurTex;
	blurTextureLoadDesc.pDesc = &blurTextureDesc;
	addResource(&blurTextureLoadDesc, &token);


	blurTextureDesc.pName = "V Blur Texture";

	blurTextureLoadDesc = {};
	blurTextureLoadDesc.ppTexture = &pVBlurTex;
	blurTextureLoadDesc.pDesc = &blurTextureDesc;
	addResource(&blurTextureLoadDesc, &token);

	waitForToken(&token);

	return plowResCloudRT != NULL && phighResCloudRT != NULL && pPostProcessRT != NULL && pGodrayRT != NULL && pSavePrevTexture != NULL && plowResCloudTexture != NULL && phighResCloudTexture != NULL
		&& pHBlurTex != NULL && pVBlurTex != NULL && pCastShadowRT != NULL;
}

void VolumetricClouds::AddUniformBuffers()
{
	// Uniform buffer for extended camera data
	BufferLoadDesc ubSettingDesc = {};
	ubSettingDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubSettingDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	ubSettingDesc.mDesc.mSize = sizeof(volumetricCloudsCB);
	ubSettingDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	ubSettingDesc.pData = NULL;

	for (uint i = 0; i < gImageCount; i++)
	{
		ubSettingDesc.ppBuffer = &VolumetricCloudsCBuffer[i];
		addResource(&ubSettingDesc, NULL);
	}
}

void VolumetricClouds::RemoveUniformBuffers()
{
	for (uint i = 0; i < gImageCount; i++)
	{
		removeResource(VolumetricCloudsCBuffer[i]);
	}
}

void VolumetricClouds::InitializeWithLoad(RenderTarget* InLinearDepthTexture, RenderTarget* InSceneColorTexture, RenderTarget* InDepthTexture)
{
	pLinearDepthTexture = InLinearDepthTexture;
	pSceneColorTexture = InSceneColorTexture;
	pDepthTexture = InDepthTexture;	
}

void VolumetricClouds::Initialize(uint InImageCount,
	ICameraController* InCameraController, Queue*	InGraphicsQueue, CmdPool* InTransCmdPool,
	Cmd** InTransCmds, Fence* InTransitionCompleteFences, Fence** InRenderCompleteFences, ProfileToken InGraphicsGpuProfiler, UIApp* InGAppUI, Buffer*	InTransmittanceBuffer)
{
	gImageCount = InImageCount;

	pCameraController = InCameraController;
	pGraphicsQueue = InGraphicsQueue;
	pTransCmdPool = InTransCmdPool;
	ppTransCmds = InTransCmds;
	pTransitionCompleteFences = InTransitionCompleteFences;
	pRenderCompleteFences = InRenderCompleteFences;
    gGpuProfileToken = InGraphicsGpuProfiler;
	pGAppUI = InGAppUI;
	pTransmittanceBuffer = InTransmittanceBuffer;

}

void VolumetricClouds::UseLowQualitySettings()
{
	DownSampling = 2;
	gAppSettings.m_MinSampleCount = 54;
	gAppSettings.m_MaxSampleCount = 128;

	gAppSettings.m_MinStepSize = 1024.0;
	gAppSettings.m_MaxStepSize = 2048.0;

	gAppSettings.m_CloudSize = 103305.805f;

	gAppSettings.m_LayerThickness = 85000.0f;

	gAppSettings.m_EnabledShadow	= true;
	gAppSettings.m_EnabledGodray	= false;
	gAppSettings.m_EnableBlur			= false;
}

void VolumetricClouds::UseMiddleQualitySettings()
{
	DownSampling = 2;
	gAppSettings.m_MinSampleCount = 54;
	gAppSettings.m_MaxSampleCount = 224;

	gAppSettings.m_MinStepSize = 1024.0;
	gAppSettings.m_MaxStepSize = 2048.0;

	gAppSettings.m_CloudSize = 103305.805f;

	gAppSettings.m_LayerThickness = 178000.0f;

	gAppSettings.m_EnabledShadow = true;
	gAppSettings.m_EnabledGodray = true;
	gAppSettings.m_EnableBlur = false;
}

void VolumetricClouds::UseHighQualitySettings()
{
	DownSampling = 1;
	gAppSettings.m_MinSampleCount = 54;
	gAppSettings.m_MaxSampleCount = 224;

	gAppSettings.m_MinStepSize = 1024.0;
	gAppSettings.m_MaxStepSize = 2048.0;

	gAppSettings.m_CloudSize = 103305.805f;

	gAppSettings.m_LayerThickness = 178000.0f;

	gAppSettings.m_EnabledShadow = true;
	gAppSettings.m_EnabledGodray = true;
	gAppSettings.m_EnableBlur = false;
}

void VolumetricClouds::UseUltraQualitySettings()
{
	DownSampling = 1;
	gAppSettings.m_MinSampleCount = 64;
	gAppSettings.m_MaxSampleCount = 256;

	gAppSettings.m_MinStepSize = 1024.0;
	gAppSettings.m_MaxStepSize = 1560.0;

	gAppSettings.m_CloudSize = 86776.898f;

	gAppSettings.m_LayerThickness = 178000.0f;

	gAppSettings.m_EnabledShadow = true;
	gAppSettings.m_EnabledGodray = true;
	gAppSettings.m_EnableBlur = false;
}

void VolumetricClouds::GenerateCloudTextures()
{
	Shader*        pGenHighTopFreq3DTexShader = NULL;
	RootSignature* pGenHighTopFreq3DTexRootSignature = NULL;
	DescriptorSet* pGenHighTopFreq3DTexDescriptorSet = NULL;
	Pipeline*      pGenHighTopFreq3DTexPipeline = NULL;
	
	Shader*        pGenLowTopFreq3DTexShader = NULL;
	RootSignature* pGenLowTopFreq3DTexRootSignature = NULL;
	DescriptorSet* pGenLowTopFreq3DTexDescriptorSet = NULL;
	Pipeline*      pGenLowTopFreq3DTexPipeline = NULL;
	
	Shader*        pGenMipmapShader = NULL;
	RootSignature* pGenMipmapRootSignature = NULL;
	DescriptorSet* pGenMipmapDescriptorSet = NULL;
	Pipeline*      pGenMipmapPipeline = NULL;
	
	ShaderLoadDesc GenHighTopFreq3DTexShader = {};
	GenHighTopFreq3DTexShader.mStages[0] = { "GenHighTopFreq3Dtex.comp", NULL, 0, NULL };
	addShader(pRenderer, &GenHighTopFreq3DTexShader, &pGenHighTopFreq3DTexShader);
	
	ShaderLoadDesc GenLowTopFreq3DTexShader = {};
	GenLowTopFreq3DTexShader.mStages[0] = { "GenLowTopFreq3Dtex.comp", NULL, 0, NULL };
	addShader(pRenderer, &GenLowTopFreq3DTexShader, &pGenLowTopFreq3DTexShader);

	ShaderLoadDesc GenMipmapShader = {};
	GenMipmapShader.mStages[0] = { "Gen3DtexMipmap.comp", NULL, 0, NULL };
	addShader(pRenderer, &GenMipmapShader, &pGenMipmapShader);
	
	RootSignatureDesc rootGenHighTopDesc = {};
	rootGenHighTopDesc.mShaderCount = 1;
	rootGenHighTopDesc.ppShaders = &pGenHighTopFreq3DTexShader;
	addRootSignature(pRenderer, &rootGenHighTopDesc, &pGenHighTopFreq3DTexRootSignature);

	RootSignatureDesc rootGenLowTopDesc = {};
	rootGenLowTopDesc.mShaderCount = 1;
	rootGenLowTopDesc.ppShaders = &pGenLowTopFreq3DTexShader;
	addRootSignature(pRenderer, &rootGenLowTopDesc, &pGenLowTopFreq3DTexRootSignature);

	RootSignatureDesc rootGenMipmapDesc = {};
	rootGenMipmapDesc.mShaderCount = 1;
	rootGenMipmapDesc.ppShaders = &pGenMipmapShader;
	addRootSignature(pRenderer, &rootGenMipmapDesc, &pGenMipmapRootSignature);
	
	DescriptorSetDesc setDesc = { pGenHighTopFreq3DTexRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
	addDescriptorSet(pRenderer, &setDesc, &pGenHighTopFreq3DTexDescriptorSet);
	
	setDesc = { pGenLowTopFreq3DTexRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
	addDescriptorSet(pRenderer, &setDesc, &pGenLowTopFreq3DTexDescriptorSet);
	
	setDesc = { pGenMipmapRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, HighFreqMipCount + LowFreqMipCount };
	addDescriptorSet(pRenderer, &setDesc, &pGenMipmapDescriptorSet);
	
	DescriptorData params[2] = {};
	params[0].pName = "SrcTexture";
	params[0].mCount = (uint32_t)gHighFrequencyOriginTexturePacked.size();
	params[0].ppTextures = gHighFrequencyOriginTexturePacked.data();
	params[1].pName = "DstTexture";
	params[1].ppTextures = &pHighFrequency3DTextureOrigin;
	updateDescriptorSet(pRenderer, 0, pGenHighTopFreq3DTexDescriptorSet, 2, params);
	
	params[0] = params[1] = {};
	params[0].pName = "SrcTexture";
	params[0].mCount = (uint32_t)gLowFrequencyOriginTexturePacked.size();
	params[0].ppTextures = gLowFrequencyOriginTexturePacked.data();
	params[1].pName = "DstTexture";
	params[1].ppTextures = &pLowFrequency3DTextureOrigin;
	updateDescriptorSet(pRenderer, 0, pGenLowTopFreq3DTexDescriptorSet, 2, params);

	PipelineDesc pipelineDesc = {};
	pipelineDesc.pCache = pPipelineCache;
	pipelineDesc.mType = PIPELINE_TYPE_COMPUTE;

	ComputePipelineDesc &comPipelineSettings = pipelineDesc.mComputeDesc;
	comPipelineSettings.pShaderProgram = pGenHighTopFreq3DTexShader;
	comPipelineSettings.pRootSignature = pGenHighTopFreq3DTexRootSignature;
	addPipeline(pRenderer, &pipelineDesc, &pGenHighTopFreq3DTexPipeline);

	comPipelineSettings.pShaderProgram = pGenLowTopFreq3DTexShader;
	comPipelineSettings.pRootSignature = pGenLowTopFreq3DTexRootSignature;
	addPipeline(pRenderer, &pipelineDesc, &pGenLowTopFreq3DTexPipeline);

	comPipelineSettings.pShaderProgram = pGenMipmapShader;
	comPipelineSettings.pRootSignature = pGenMipmapRootSignature;
	addPipeline(pRenderer, &pipelineDesc, &pGenMipmapPipeline);
	
	Cmd* cmd = ppTransCmds[0];

	beginCmd(cmd);
	
	cmdBeginDebugMarker(cmd, 1.0f, 1.0f, 1.0f, "Preprocessing");
	
	cmdBindPipeline(cmd, pGenHighTopFreq3DTexPipeline);
	cmdBindDescriptorSet(cmd, 0, pGenHighTopFreq3DTexDescriptorSet);
	uint32_t* pThreadGroupSize = pGenHighTopFreq3DTexShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
	cmdDispatch(cmd, gHighFreq3DTextureSize / pThreadGroupSize[0], gHighFreq3DTextureSize / pThreadGroupSize[1], gHighFreq3DTextureSize / pThreadGroupSize[2]);

	cmdBindPipeline(cmd, pGenLowTopFreq3DTexPipeline);
	cmdBindDescriptorSet(cmd, 0, pGenLowTopFreq3DTexDescriptorSet);
	pThreadGroupSize = pGenLowTopFreq3DTexShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
	cmdDispatch(cmd, gLowFreq3DTextureSize / pThreadGroupSize[0], gLowFreq3DTextureSize / pThreadGroupSize[1], gLowFreq3DTextureSize / pThreadGroupSize[2]);

	TextureBarrier barriersForNoise[] = {
	  { pHighFrequency3DTextureOrigin, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
	  { pLowFrequency3DTextureOrigin, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
	};
	cmdResourceBarrier(cmd, 0, NULL, 2, barriersForNoise, 0, NULL);

	cmdBindPipeline(cmd, pGenMipmapPipeline);

	struct Data
	{
		uint mip;
	} data = { 0 };

	uint32_t setIndex = 0;

	for (uint32_t i = 0; i < HighFreqMipCount; i++, ++setIndex)
	{
		data.mip = i;
		cmdBindPushConstants(cmd, pGenMipmapRootSignature, "RootConstant", &data);

		DescriptorData mipParams[2] = {};
		mipParams[0].pName = "SrcTexture";
		mipParams[0].ppTextures = &pHighFrequency3DTextureOrigin;
		mipParams[1].pName = "DstTexture";
		mipParams[1].ppTextures = &pHighFrequency3DTexture;
		mipParams[1].mUAVMipSlice = i;
		updateDescriptorSet(pRenderer, setIndex, pGenMipmapDescriptorSet, 2, mipParams);
		cmdBindDescriptorSet(cmd, setIndex, pGenMipmapDescriptorSet);

		uint mipMapSize = gHighFreq3DTextureSize >> i;

		cmdDispatch(cmd, mipMapSize, mipMapSize, mipMapSize);
	}

	for (uint32_t i = 0; i < LowFreqMipCount; i++, ++setIndex)
	{
		data.mip = i;
		cmdBindPushConstants(cmd, pGenMipmapRootSignature, "RootConstant", &data);

		DescriptorData mipParams[2] = {};
		mipParams[0].pName = "SrcTexture";
		mipParams[0].ppTextures = &pLowFrequency3DTextureOrigin;
		mipParams[1].pName = "DstTexture";
		mipParams[1].ppTextures = &pLowFrequency3DTexture;
		mipParams[1].mUAVMipSlice = i;
		updateDescriptorSet(pRenderer, setIndex, pGenMipmapDescriptorSet, 2, mipParams);
		cmdBindDescriptorSet(cmd, setIndex, pGenMipmapDescriptorSet);

		uint mipMapSize = gLowFreq3DTextureSize >> i;

		cmdDispatch(cmd, mipMapSize, mipMapSize, mipMapSize);
	}

	TextureBarrier barriersForNoise2[] = {
	  { pHighFrequency3DTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
	  { pLowFrequency3DTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
	};
	cmdResourceBarrier(cmd, 0, NULL, 2, barriersForNoise2, 0, NULL);
	
	cmdEndDebugMarker(cmd);
	
	endCmd(cmd);

	QueueSubmitDesc submitDesc = {};
	submitDesc.mCmdCount = 1;
	submitDesc.ppCmds = &cmd;
	submitDesc.pSignalFence = pTransitionCompleteFences;
	submitDesc.mSubmitDone = true;
	queueSubmit(pGraphicsQueue, &submitDesc);
	waitForFences(pRenderer, 1, &pTransitionCompleteFences);

	removeDescriptorSet(pRenderer, pGenHighTopFreq3DTexDescriptorSet);
	removeDescriptorSet(pRenderer, pGenLowTopFreq3DTexDescriptorSet);
	removeDescriptorSet(pRenderer, pGenMipmapDescriptorSet);
	
	removeRootSignature(pRenderer, pGenHighTopFreq3DTexRootSignature);
	removeRootSignature(pRenderer, pGenLowTopFreq3DTexRootSignature);
	removeRootSignature(pRenderer, pGenMipmapRootSignature);
	
	removeShader(pRenderer, pGenHighTopFreq3DTexShader);
	removeShader(pRenderer, pGenLowTopFreq3DTexShader);
	removeShader(pRenderer, pGenMipmapShader);
	
	removePipeline(pRenderer, pGenHighTopFreq3DTexPipeline);
	removePipeline(pRenderer, pGenLowTopFreq3DTexPipeline);
	removePipeline(pRenderer, pGenMipmapPipeline);

	resetCmdPool(pRenderer, pTransCmdPool);
}
