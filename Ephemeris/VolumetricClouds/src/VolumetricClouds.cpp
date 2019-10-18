/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "VolumetricClouds.h"

#include "../../../../The-Forge/Common_3/Renderer/ResourceLoader.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"

#define TERRAIN_NEAR 50.0f
#define TERRAIN_FAR 100000000.0f

#if defined(_DURANGO)
#define	USE_CHEAP_SETTINGS
#endif

#if defined(USE_CHEAP_SETTINGS)
static uint               prevDownSampling = 2;
#else
static uint               prevDownSampling = 1;
#endif


static uint32_t           gDownsampledCloudSize = (uint32_t)pow(2, (double)prevDownSampling);
const uint32_t            glowResBufferSize = 4;
static uint32_t           postProcessBufferSize = gDownsampledCloudSize;
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

Texture*                  pSavePrevTexture = NULL;

#if defined(METAL)
Buffer*                   pSavePrevBuffer[3] = { NULL };
#endif

RenderTarget*             pPostProcessRT = NULL;
RenderTarget*             pRenderTargetsPostProcess[2] = { nullptr };

// Pre-stages

Shader*                   pGenMipmapShader = NULL;
Pipeline*                 pGenMipmapPipeline = NULL;
RootSignature*            pGenMipmapRootSignature = NULL;

Shader*                   pGenHiZMipmapShader = NULL;
Pipeline*                 pGenHiZMipmapPipeline = NULL;
RootSignature*            pGenHiZMipmapRootSignature = NULL;

Shader*                   pCopyTextureShader = NULL;
Pipeline*                 pCopyTexturePipeline = NULL;
RootSignature*            pCopyTextureRootSignature = NULL;

Shader*                   pCopyWeatherTextureShader = NULL;
Pipeline*                 pCopyWeatherTexturePipeline = NULL;
RootSignature*            pCopyWeatherTextureRootSignature = NULL;



Shader*                   pGenLowTopFreq3DtexShader = NULL;
Pipeline*                 pGenLowTopFreq3DtexPipeline = NULL;

Shader*                   pGenHighTopFreq3DtexShader = NULL;
Pipeline*                 pGenHighTopFreq3DtexPipeline = NULL;
RootSignature*            pGenFreq3DTexRootSignature = NULL;

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

// Comp

Shader*                   pGenHiZMipmapPRShader = NULL;
Pipeline*                 pGenHiZMipmapPRPipeline = NULL;

Shader*                   pVolumetricCloudCompShader = NULL;
Pipeline*                 pVolumetricCloudCompPipeline = NULL;

Shader*                   pVolumetricCloudWithDepthCompShader = NULL;
Pipeline*                 pVolumetricCloudWithDepthCompPipeline = NULL;

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

RasterizerState*          pRasterizer = NULL;

const uint32_t            gHighFreq3DTextureSize = 32;
const uint32_t            gLowFreq3DTextureSize = 128;

Texture*                  gHighFrequencyOriginTextureStorage = NULL;
Texture*                  gLowFrequencyOriginTextureStorage = NULL;

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

BlendState*               pBlendStateSkyBox;
BlendState*               pBlendStateGodray;

#if defined(METAL)
#define				USE_VC_FRAGMENTSHADER 1
#else
#define				USE_VC_FRAGMENTSHADER 0
#endif

#define				USE_RP_FRAGMENTSHADER 1
#define				USE_DEPTH_CULLING 1
#define				USE_LOD_DEPTH 1
#define				DRAW_SHADOW   0

/*
#define _CLOUDS_LAYER_START			1500.0							// The height where the clouds' layer get started
#define _CLOUDS_LAYER_THICKNESS		8500.0							// The height where the clouds' layer covers
#define _CLOUDS_LAYER_END			10000.0							// The height where the clouds' layer get ended (End = Start + Thickness)
*/

#define _CLOUDS_LAYER_START			15000.0f						// The height where the clouds' layer get started
#define _CLOUDS_LAYER_THICKNESS		20000.0f							// The height where the clouds' layer covers
#define _CLOUDS_LAYER_END			(_CLOUDS_LAYER_START + _CLOUDS_LAYER_THICKNESS)							// The height where the clouds' layer get ended (End = Start + Thickness)

/*
#define _CLOUDS_LAYER_START			15000.0							// The height where the clouds' layer get started
#define _CLOUDS_LAYER_THICKNESS		35000.0							// The height where the clouds' layer covers
#define _CLOUDS_LAYER_END			50000.0							// The height where the clouds' layer get ended (End = Start + Thickness)
*/

static void ShaderPath(const eastl::string &shaderPath, char* pShaderName, eastl::string &result)
{
	result.resize(0);
	eastl::string shaderName(pShaderName);
	result = shaderPath + shaderName;
}

const uint32_t		HighFreqDimension = 32;
const uint32_t		LowFreqDimension = 128;
const uint32_t		HighFreqMipCount = (uint32_t)log((double)HighFreqDimension);
const uint32_t		LowFreqMipCount = (uint32_t)log((double)LowFreqDimension);
const uint32_t		TotalMipCount = HighFreqMipCount + LowFreqMipCount + 2;

static mat4 prevView;
static mat4 projMat;

static float4 g_ProjectionExtents;
static float g_currentTime = 0.0f;

typedef struct AppSettings
{
	uint32_t m_Enabled = true;
#if defined(USE_CHEAP_SETTINGS)
	uint32_t m_DownSampling = 2;
#else
	uint32_t m_DownSampling = 1;
#endif
	// VolumetricClouds raymarching

#if defined(USE_CHEAP_SETTINGS)
	uint32_t m_MinSampleCount = 54;
	uint32_t m_MaxSampleCount = 96;
	float m_MinStepSize = 1024.0;
	float m_MaxStepSize = 2048.0;
#else
	uint32_t m_MinSampleCount = 128;
	uint32_t m_MaxSampleCount = 192;
	float m_MinStepSize = 512.0;
	float m_MaxStepSize = 1536.0;
#endif

	float m_LayerHeightOffset = 5800.0f;
	float	m_LayerThickness = 78000.0f;

	bool m_EnabledTemporalRayOffset = false;

	// VolumetricClouds modeling
	float m_BaseTile = 0.455f;

	float m_DetailTile = 4.381f;
	float m_DetailStrength = 0.298f;
	float m_CurlTile = 0.1f;
	float m_CurlStrength = 2000.0f;

	float m_UpStreamScale = 0.0005f;
	float m_UpStreamSpeed = 1.0f;

	float m_CloudTopOffset = 500.0f;
	float m_CloudSize = 64049.602f;

	float m_CloudDensity = 3.0f;
	float m_CloudCoverageModifier = 0.0f;

	float m_CloudTypeModifier = 0.521f;
	float m_AnvilBias = 1.0f;
	float m_WeatherTexSize = 735537.0f;

	float	WeatherTextureAzimuth = 0.0f;
	float	WeatherTextureDistance = 0.0f;

	// Wind factors
	float m_WindAzimuth = 0.0f;

	float m_WindIntensity = 10.0f;
	float m_NoiseFlowIntensity = 10.0f;
	// VolumetricClouds lighting	
	uint32_t m_CustomColor = 0xFFFFFFFF;

	float  m_CustomColorIntensity = 1.0f;
	float	 m_CustomColorBlendFactor = 0.0f;
	float	 m_Contrast = 1.2f;
	float	 m_TransStepSize = 768.0f;

	float m_BackgroundBlendFactor = 1.0f;
	float m_Precipitation = 1.3f;
	float m_SilverIntensity = 0.2f;
	float m_SilverSpread = 0.29f;

	float m_Eccentricity = 0.65f;
	float m_CloudBrightness = 1.1f;
	//Culling
	bool m_EnabledDepthCulling = true;
	bool m_EnabledLodDepth = true;
	//Shadow

#if defined(USE_CHEAP_SETTINGS)
	bool m_EnabledShadow = false;
#else
	bool m_EnabledShadow = true;
#endif


	float m_ShadowBrightness = 0.5f;
	float m_ShadowTiling = 20.0f;
	float m_ShadowSpeed = 1.0f;
	// VolumetricClouds' Light shaft

#if defined(USE_CHEAP_SETTINGS)
	bool m_EnabledGodray = false;
#else
	bool m_EnabledGodray = true;
#endif


	uint32_t m_GodNumSamples = 80;
	float m_Exposure = 0.008f;
	float m_Decay = 0.975f;
	float m_Density = 0.3f;
	float m_Weight = 0.85f;

	float m_Test00 = 0.5f;
	float m_Test01 = 0.173f;
	float m_Test02 = 0.23f;
	float m_Test03 = 25000.0f;

#if defined(USE_CHEAP_SETTINGS)
	bool m_EnableBlur = true;
#else
	bool m_EnableBlur = false;
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

static int4 bayerOffsets[] =
{
	{0,8,2,10 },
	{12,4,14,6 },
	{3,11,1,9 },
	{15,7,13,5 }
};

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

#if defined(VULKAN)
static void TransitionRenderTargets(RenderTarget *pRT, ResourceState state, Renderer* renderer, Cmd* cmd, Queue* queue, Fence* fence)
{
	// Transition render targets to desired state
	beginCmd(cmd);

	TextureBarrier barrier[] = {
		   { pRT->pTexture, state }
	};

	cmdResourceBarrier(cmd, 0, NULL, 1, barrier);
	endCmd(cmd);

	queueSubmit(queue, 1, &cmd, fence, 0, NULL, 0, NULL);
	waitForFences(renderer, 1, &fence);
}
#endif

bool VolumetricClouds::Init(Renderer* renderer)
{
	pRenderer = renderer;
	g_StandardPosition = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	g_ShadowInfo = vec4(0.0f, 0.0f, 1.0f, 0.0f);
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

#if defined(_DURANGO)
	eastl::string shaderPath("");
#elif defined(DIRECT3D12)
	eastl::string shaderPath("../../../../../Ephemeris/VolumetricClouds/resources/Shaders/D3D12/");
#elif defined(VULKAN)
	eastl::string shaderPath("../../../../../Ephemeris/VolumetricClouds/resources/Shaders/Vulkan/");
#elif defined(METAL)
	eastl::string shaderPath("../../../../../Ephemeris/VolumetricClouds/resources/Shaders/Metal/");
#endif

	ShaderLoadDesc basicShader = {};
	eastl::string basicShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"volumetricCloud.vert", basicShaderFullPath[0]);
	basicShader.mStages[0] = { basicShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"postProcess.frag", basicShaderFullPath[1]);
	basicShader.mStages[1] = { basicShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &basicShader, &pPostProcessShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc postBlurShader = {};
	eastl::string postBlurShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"volumetricCloud.vert", postBlurShaderFullPath[0]);
	postBlurShader.mStages[0] = { postBlurShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"postProcessWithBlur.frag", postBlurShaderFullPath[1]);
	postBlurShader.mStages[1] = { postBlurShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &postBlurShader, &pPostProcessWithBlurShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc reprojectionShader = {};
	eastl::string reprojectionShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"volumetricCloud.vert", reprojectionShaderFullPath[0]);
	reprojectionShader.mStages[0] = { reprojectionShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"reprojection.frag", reprojectionShaderFullPath[1]);
	reprojectionShader.mStages[1] = { reprojectionShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &reprojectionShader, &pReprojectionShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc godrayShader = {};
	eastl::string godrayShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"Triangular.vert", godrayShaderFullPath[0]);
	godrayShader.mStages[0] = { godrayShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"godray.frag", godrayShaderFullPath[1]);
	godrayShader.mStages[1] = { godrayShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &godrayShader, &pGodrayShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc godrayAddShader = {};
	eastl::string godrayAddShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"Triangular.vert", godrayAddShaderFullPath[0]);
	godrayAddShader.mStages[0] = { godrayAddShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"godrayAdd.frag", godrayAddShaderFullPath[1]);
	godrayAddShader.mStages[1] = { godrayAddShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

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
	eastl::string compositeShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"volumetricCloud.vert", compositeShaderFullPath[0]);
	compositeShader.mStages[0] = { compositeShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"composite.frag", compositeShaderFullPath[1]);
	compositeShader.mStages[1] = { compositeShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &compositeShader, &pCompositeShader);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
	ShaderLoadDesc compositeOverlayShader = {};
	eastl::string compositeOverlayShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"volumetricCloud.vert", compositeOverlayShaderFullPath[0]);
	compositeOverlayShader.mStages[0] = { compositeOverlayShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"compositeOverlay.frag", compositeOverlayShaderFullPath[1]);
	compositeOverlayShader.mStages[1] = { compositeOverlayShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &compositeOverlayShader, &pCompositeOverlayShader);
*/
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc GenLowTopFreq3DtexShader = {};
	eastl::string GenLowTopFreq3DtexShaderFullPath[1];
	ShaderPath(shaderPath, (char*)"genLowTopFreq3Dtex.comp", GenLowTopFreq3DtexShaderFullPath[0]);
	GenLowTopFreq3DtexShader.mStages[0] = { GenLowTopFreq3DtexShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &GenLowTopFreq3DtexShader, &pGenLowTopFreq3DtexShader);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc GenHighTopFreq3DtexShader = {};
	eastl::string GenHighTopFreq3DtexShaderFullPath[1];
	ShaderPath(shaderPath, (char*)"genHighTopFreq3Dtex.comp", GenHighTopFreq3DtexShaderFullPath[0]);
	GenHighTopFreq3DtexShader.mStages[0] = { GenHighTopFreq3DtexShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &GenHighTopFreq3DtexShader, &pGenHighTopFreq3DtexShader);

	Shader* GenHighShaders[] = { pGenLowTopFreq3DtexShader, pGenHighTopFreq3DtexShader };

	RootSignatureDesc rootGenHighDesc = {};
	rootGenHighDesc.mShaderCount = 2;
	rootGenHighDesc.ppShaders = GenHighShaders;

	addRootSignature(pRenderer, &rootGenHighDesc, &pGenFreq3DTexRootSignature);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc GenMipmap3DtexShader = {};
	eastl::string GenMipmap3DtexShaderFullPath[1];
	ShaderPath(shaderPath, (char*)"gen3DtexMipmap.comp", GenMipmap3DtexShaderFullPath[0]);
	GenMipmap3DtexShader.mStages[0] = { GenMipmap3DtexShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &GenMipmap3DtexShader, &pGenMipmapShader);

	Shader* GenMipmapShaders[] = { pGenMipmapShader };

	RootSignatureDesc rootGenMipmapDesc = {};
	rootGenMipmapDesc.mShaderCount = 1;
	rootGenMipmapDesc.ppShaders = GenMipmapShaders;

	addRootSignature(pRenderer, &rootGenMipmapDesc, &pGenMipmapRootSignature);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc GenHiZMipmapShader = {};
	eastl::string GenHiZMipmapShaderFullPath[1];
	ShaderPath(shaderPath, (char*)"HiZdownSampling.comp", GenHiZMipmapShaderFullPath[0]);
	GenHiZMipmapShader.mStages[0] = { GenHiZMipmapShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &GenHiZMipmapShader, &pGenHiZMipmapShader);

	Shader* GenHiZMipmapShaders[] = { pGenHiZMipmapShader };

	RootSignatureDesc rootGenHiZMipmapDesc = {};
	rootGenHiZMipmapDesc.mShaderCount = 1;
	rootGenHiZMipmapDesc.ppShaders = GenHiZMipmapShaders;

	addRootSignature(pRenderer, &rootGenHiZMipmapDesc, &pGenHiZMipmapRootSignature);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc GenHiZMipmapPRShader = {};
	eastl::string GenHiZMipmapPRShaderFullPath[1];
	ShaderPath(shaderPath, (char*)"HiZdownSamplingPR.comp", GenHiZMipmapPRShaderFullPath[0]);
	GenHiZMipmapPRShader.mStages[0] = { GenHiZMipmapPRShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &GenHiZMipmapPRShader, &pGenHiZMipmapPRShader);

	Shader* GenHiZMipmapPRShaders[] = { pGenHiZMipmapPRShader };

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc CopyTextureShader = {};
	eastl::string CopyTextureShaderFullPath[1];
	ShaderPath(shaderPath, (char*)"copyTexture.comp", CopyTextureShaderFullPath[0]);
	CopyTextureShader.mStages[0] = { CopyTextureShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &CopyTextureShader, &pCopyTextureShader);

	Shader* CopyTextureShaders[] = { pCopyTextureShader };

	RootSignatureDesc rootCopyTextureDesc = {};
	rootCopyTextureDesc.mShaderCount = 1;
	rootCopyTextureDesc.ppShaders = CopyTextureShaders;

	addRootSignature(pRenderer, &rootCopyTextureDesc, &pCopyTextureRootSignature);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc CopyWeatherTextureShader = {};
	eastl::string CopyWeatherTextureShaderFullPath[1];
	ShaderPath(shaderPath, (char*)"copyWeatherTexture.comp", CopyWeatherTextureShaderFullPath[0]);
	CopyWeatherTextureShader.mStages[0] = { CopyWeatherTextureShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &CopyWeatherTextureShader, &pCopyWeatherTextureShader);

	Shader* CopyWeatherTextureShaders[] = { pCopyWeatherTextureShader };

	RootSignatureDesc rootCopyWeatherTextureDesc = {};
	rootCopyWeatherTextureDesc.mShaderCount = 1;
	rootCopyWeatherTextureDesc.ppShaders = CopyWeatherTextureShaders;

	addRootSignature(pRenderer, &rootCopyWeatherTextureDesc, &pCopyWeatherTextureRootSignature);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////


	ShaderLoadDesc BlurShader = {};
	eastl::string BlurShaderFullPath[1];
	ShaderPath(shaderPath, (char*)"BlurHorizontal.comp", BlurShaderFullPath[0]);
	BlurShader.mStages[0] = { BlurShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &BlurShader, &pHorizontalBlurShader);

	ShaderPath(shaderPath, (char*)"BlurVertical.comp", BlurShaderFullPath[0]);
	BlurShader.mStages[0] = { BlurShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &BlurShader, &pVerticalBlurShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc CopyRTShader = {};
	eastl::string CopyRTShaderFullPath[1];
	ShaderPath(shaderPath, (char*)"copyRT.comp", CopyRTShaderFullPath[0]);
	CopyRTShader.mStages[0] = { CopyRTShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &CopyRTShader, &pCopyRTShader);

	Shader* CopyRTShaders[] = { pCopyRTShader };

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc VolumetricCloudShaderDesc = {};
	eastl::string VolumetricCloudShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"volumetricCloud.vert", VolumetricCloudShaderFullPath[0]);
	VolumetricCloudShaderDesc.mStages[0] = { VolumetricCloudShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"volumetricCloud.frag", VolumetricCloudShaderFullPath[1]);
	VolumetricCloudShaderDesc.mStages[1] = { VolumetricCloudShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &VolumetricCloudShaderDesc, &pVolumetricCloudShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc VolumetricCloudCompShaderDesc = {};
	eastl::string VolumetricCloudCompShaderFullPath[1];
	ShaderPath(shaderPath, (char*)"volumetricCloud.comp", VolumetricCloudCompShaderFullPath[0]);
	VolumetricCloudCompShaderDesc.mStages[0] = { VolumetricCloudCompShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &VolumetricCloudCompShaderDesc, &pVolumetricCloudCompShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc VolumetricCloudWithDepthCompShaderDesc = {};
	eastl::string VolumetricCloudWithDepthCompShaderFullPath[1];
	ShaderPath(shaderPath, (char*)"volumetricCloudWithDepth.comp", VolumetricCloudWithDepthCompShaderFullPath[0]);
	VolumetricCloudWithDepthCompShaderDesc.mStages[0] = { VolumetricCloudWithDepthCompShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &VolumetricCloudWithDepthCompShaderDesc, &pVolumetricCloudWithDepthCompShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	BlendStateDesc blendStateSkyBoxDesc = {};
	blendStateSkyBoxDesc.mBlendModes[0] = BM_ADD;
	blendStateSkyBoxDesc.mBlendAlphaModes[0] = BM_ADD;

	blendStateSkyBoxDesc.mSrcFactors[0] = BC_SRC_ALPHA;// BC_ONE;// BC_ONE_MINUS_DST_ALPHA;
	blendStateSkyBoxDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;// BC_ZERO;// BC_DST_ALPHA;

	blendStateSkyBoxDesc.mSrcAlphaFactors[0] = BC_ONE;
	blendStateSkyBoxDesc.mDstAlphaFactors[0] = BC_ZERO;

	blendStateSkyBoxDesc.mMasks[0] = ALL;
	blendStateSkyBoxDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
	addBlendState(pRenderer, &blendStateSkyBoxDesc, &pBlendStateSkyBox);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	blendStateSkyBoxDesc = {};
	blendStateSkyBoxDesc.mBlendModes[0] = BM_ADD;
	blendStateSkyBoxDesc.mBlendAlphaModes[0] = BM_ADD;

	blendStateSkyBoxDesc.mSrcFactors[0] = BC_ONE;
	blendStateSkyBoxDesc.mDstFactors[0] = BC_ONE;

	blendStateSkyBoxDesc.mSrcAlphaFactors[0] = BC_ZERO;
	blendStateSkyBoxDesc.mDstAlphaFactors[0] = BC_ONE;

	blendStateSkyBoxDesc.mMasks[0] = ALL;
	blendStateSkyBoxDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
	addBlendState(pRenderer, &blendStateSkyBoxDesc, &pBlendStateGodray);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	const char* pVCSamplerNames[] = { "g_LinearWrapSampler", "g_LinearClampSampler", "g_PointClampSampler", "g_LinearBorderSampler" };
	Sampler* pVCSamplers[] = { pBilinearSampler, pBiClampSampler, pPointClampSampler, pLinearBorderSampler };

	Shader*           shaders[] = { pVolumetricCloudShader, pReprojectionShader, pPostProcessShader, pPostProcessWithBlurShader, pGodrayShader, pGodrayAddShader, pCompositeShader/*, pCompositeOverlayShader*/ };
	RootSignatureDesc rootDesc = {};
	rootDesc.mShaderCount = 7;
	rootDesc.ppShaders = shaders;
	rootDesc.mStaticSamplerCount = 4;
	rootDesc.ppStaticSamplerNames = pVCSamplerNames;
	rootDesc.ppStaticSamplers = pVCSamplers;
	addRootSignature(pRenderer, &rootDesc, &pVolumetricCloudsRootSignatureGraphics);

	Shader*           shaderComps[] = { pGenHiZMipmapPRShader, pVolumetricCloudCompShader, pVolumetricCloudWithDepthCompShader, pCopyRTShader, pHorizontalBlurShader, pVerticalBlurShader };
	rootDesc = {};
	rootDesc.mShaderCount = 6;
	rootDesc.ppShaders = shaderComps;
	rootDesc.mStaticSamplerCount = 4;
	rootDesc.ppStaticSamplerNames = pVCSamplerNames;
	rootDesc.ppStaticSamplers = pVCSamplers;
	addRootSignature(pRenderer, &rootDesc, &pVolumetricCloudsRootSignatureCompute);

	DescriptorSetDesc setDesc = { pVolumetricCloudsRootSignatureCompute, DESCRIPTOR_UPDATE_FREQ_NONE, 6 };
	addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetCompute[0]);
	setDesc = { pVolumetricCloudsRootSignatureCompute, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
	addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetCompute[1]);
	setDesc = { pVolumetricCloudsRootSignatureGraphics, DESCRIPTOR_UPDATE_FREQ_NONE, 8 };
	addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetGraphics[0]);
	setDesc = { pVolumetricCloudsRootSignatureGraphics, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
	addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetGraphics[1]);

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
	screenQuadVbDesc.ppBuffer = &pTriangularScreenVertexBuffer;
	addResource(&screenQuadVbDesc);

	gHighFrequencyOriginTexture.resize(gHighFreq3DTextureSize);

#if defined(_DURANGO)
	eastl::string gHighFrequencyOriginTextureName("Textures/hiResCloudShape/hiResClouds (");
#else
	eastl::string gHighFrequencyOriginTextureName("../../../Ephemeris/VolumetricClouds/resources/Textures/hiResCloudShape/hiResClouds (");
#endif

	for (uint32_t i = 0; i < gHighFreq3DTextureSize; ++i)
	{
		char stringInt[4];
		_itoa(i, stringInt);

		eastl::string gHighFrequencyNameLocal = gHighFrequencyOriginTextureName;

		gHighFrequencyNameLocal += eastl::string(stringInt);
		gHighFrequencyNameLocal += eastl::string(")");

		TextureLoadDesc highFrequencyOrigin3DTextureDesc = {};		
		PathHandle highFrequencyOrigin3DTextureFilePath = fsCopyPathInResourceDirectory(RD_OTHER_FILES, gHighFrequencyNameLocal.c_str());
		highFrequencyOrigin3DTextureDesc.pFilePath = highFrequencyOrigin3DTextureFilePath;
		highFrequencyOrigin3DTextureDesc.ppTexture = &gHighFrequencyOriginTexture[i];
		addResource(&highFrequencyOrigin3DTextureDesc, false);
	}

	//////////////////////////////////////////////////////////////////////////////////////////

	gLowFrequencyOriginTexture.resize(gLowFreq3DTextureSize);

#if defined(_DURANGO)
	eastl::string	gLowFrequencyOriginTextureName("Textures/lowResCloudShape/lowResCloud(");
#else
	eastl::string	gLowFrequencyOriginTextureName("../../../Ephemeris/VolumetricClouds/resources/Textures/lowResCloudShape/lowResCloud(");
#endif	

	for (uint32_t i = 0; i < gLowFreq3DTextureSize; ++i)
	{
		char stringInt[4];
		_itoa(i, stringInt);

		eastl::string gLowFrequencyNameLocal = gLowFrequencyOriginTextureName;

		gLowFrequencyNameLocal += eastl::string(stringInt);
		gLowFrequencyNameLocal += eastl::string(")");

		TextureLoadDesc lowFrequencyOrigin3DTextureDesc = {};
		PathHandle lowFrequencyOrigin3DTextureFilePath = fsCopyPathInResourceDirectory(RD_OTHER_FILES, gLowFrequencyNameLocal.c_str());
		lowFrequencyOrigin3DTextureDesc.pFilePath = lowFrequencyOrigin3DTextureFilePath;
		lowFrequencyOrigin3DTextureDesc.ppTexture = &gLowFrequencyOriginTexture[i];
		addResource(&lowFrequencyOrigin3DTextureDesc, false);
	}

	gHighFrequencyOriginTextureStorage = (Texture*)conf_malloc(sizeof(Texture) * gHighFrequencyOriginTexture.size());
	gLowFrequencyOriginTextureStorage = (Texture*)conf_malloc(sizeof(Texture) * gLowFrequencyOriginTexture.size());

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

	lowFreqImgDesc.mMipLevels = 7 /*2^7 = 128*/;
	lowFreqImgDesc.mSampleCount = SAMPLE_COUNT_1;
	//lowFreqImgDesc.mSrgb = false;

	lowFreqImgDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
	lowFreqImgDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	lowFreqImgDesc.pDebugName = L"LowFrequency3DTexture";

	TextureLoadDesc lowFrequency3DTextureDesc = {};
	lowFrequency3DTextureDesc.ppTexture = &pLowFrequency3DTexture;
	lowFrequency3DTextureDesc.pDesc = &lowFreqImgDesc;
	addResource(&lowFrequency3DTextureDesc, false);

	lowFrequency3DTextureDesc.ppTexture = &pLowFrequency3DTextureOrigin;
	addResource(&lowFrequency3DTextureDesc, false);

	//////////////////////////////////////////////////////////////////////////////////////////////

	TextureDesc highFreqImgDesc = {};
	highFreqImgDesc.mArraySize = 1;
	highFreqImgDesc.mFormat = TinyImageFormat_R8G8B8A8_UNORM;

	highFreqImgDesc.mWidth = gHighFreq3DTextureSize;
	highFreqImgDesc.mHeight = gHighFreq3DTextureSize;
	highFreqImgDesc.mDepth = gHighFreq3DTextureSize;

	highFreqImgDesc.mMipLevels = 5 /*2^5 = 32*/;
	highFreqImgDesc.mSampleCount = SAMPLE_COUNT_1;
	//highFreqImgDesc.mSrgb = false;

	highFreqImgDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
	highFreqImgDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	highFreqImgDesc.pDebugName = L"HighFrequency3DTexture";

	TextureLoadDesc highFrequency3DTextureDesc = {};
	highFrequency3DTextureDesc.ppTexture = &pHighFrequency3DTexture;
	highFrequency3DTextureDesc.pDesc = &highFreqImgDesc;
	addResource(&highFrequency3DTextureDesc, false);

	highFrequency3DTextureDesc.ppTexture = &pHighFrequency3DTextureOrigin;
	addResource(&highFrequency3DTextureDesc, false);

	//////////////////////////////////////////////////////////////////////////////////////////////

	TextureLoadDesc curlNoiseTextureDesc = {};
#if defined(_DURANGO)
	PathHandle curlNoiseTextureFilePath = fsCopyPathInResourceDirectory(RD_OTHER_FILES, "Textures/CurlNoiseFBM");	
#else
	PathHandle curlNoiseTextureFilePath = fsCopyPathInResourceDirectory(RD_OTHER_FILES, "../../../Ephemeris/VolumetricClouds/resources/Textures/CurlNoiseFBM");
#endif	
	curlNoiseTextureDesc.pFilePath = curlNoiseTextureFilePath;
	curlNoiseTextureDesc.ppTexture = &pCurlNoiseTexture;
	addResource(&curlNoiseTextureDesc, false);

	//////////////////////////////////////////////////////////////////////////////////////////////

	TextureLoadDesc weathereTextureDesc = {};
#if defined(_DURANGO)
	PathHandle weathereTextureFilePath = fsCopyPathInResourceDirectory(RD_OTHER_FILES, "Textures/WeatherMap");
#else
	PathHandle weathereTextureFilePath = fsCopyPathInResourceDirectory(RD_OTHER_FILES, "../../../Ephemeris/VolumetricClouds/resources/Textures/WeatherMap");
#endif
	weathereTextureDesc.pFilePath = weathereTextureFilePath;
	weathereTextureDesc.ppTexture = &pWeatherTexture;
	addResource(&weathereTextureDesc, false);

	TextureDesc WeatherCompactTextureDesc = {};
	WeatherCompactTextureDesc.mArraySize = 1;
	WeatherCompactTextureDesc.mFormat = TinyImageFormat_R8G8_UNORM;

	WeatherCompactTextureDesc.mWidth = pWeatherTexture->mDesc.mWidth;
	WeatherCompactTextureDesc.mHeight = pWeatherTexture->mDesc.mHeight;
	WeatherCompactTextureDesc.mDepth = pWeatherTexture->mDesc.mDepth;

	WeatherCompactTextureDesc.mMipLevels = 1;
	WeatherCompactTextureDesc.mSampleCount = SAMPLE_COUNT_1;
	//WeatherCompactTextureDesc.mSrgb = false;

	WeatherCompactTextureDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
	WeatherCompactTextureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	WeatherCompactTextureDesc.pDebugName = L"WeatherCompactTexture";


	TextureLoadDesc WeatherCompactTextureLoadDesc = {};
	WeatherCompactTextureLoadDesc.ppTexture = &pWeatherCompactTexture;
	WeatherCompactTextureLoadDesc.pDesc = &WeatherCompactTextureDesc;
	addResource(&WeatherCompactTextureLoadDesc, false);

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
	  cmdDispatch(cmd, (uint32_t)ceilf((float)pWeatherTexture->mDesc.mWidth / (float)pThreadGroupSize[0]), (uint32_t)ceilf((float)pWeatherTexture->mDesc.mHeight / (float)pThreadGroupSize[1]), 1);

	  TextureBarrier barriers[] = {
		{ pWeatherCompactTexture, RESOURCE_STATE_SHADER_RESOURCE }
	  };

	  cmdResourceBarrier(cmd, 0, NULL, 1, barriers, false);

	  endCmd(cmd);
	  queueSubmit(pGraphicsQueue, 1, &cmd, pTransitionCompleteFences, 0, 0, 0, 0);
	  waitForFences(pRenderer, 1, &pTransitionCompleteFences);
	}
	*/


	RasterizerStateDesc rasterizerStateDesc = {};
	rasterizerStateDesc.mCullMode = CULL_MODE_NONE;
	addRasterizerState(pRenderer, &rasterizerStateDesc, &pRasterizer);

	AddUniformBuffers();


	///////////////////////////////////////////////////////////////////
	// UI
	///////////////////////////////////////////////////////////////////

	GuiDesc guiDesc = {};
	guiDesc.mStartPosition = vec2(1600.0f / getDpiScale().getX(), 700.0f / getDpiScale().getY());
	guiDesc.mStartSize = vec2(300.0f / getDpiScale().getX(), 250.0f / getDpiScale().getY());
	pGuiWindow = pGAppUI->AddGuiComponent("Volumetric Cloud", &guiDesc);

	SliderUintWidget DownSampling("Downsampling", &gAppSettings.m_DownSampling, 1, 3, 1);
	pGuiWindow->AddWidget(DownSampling);

	ButtonWidget UseLowQuality("Use  Low Quality Settings");
	UseLowQuality.pOnEdited = VolumetricClouds::UseLowQualitySettings;
	pGuiWindow->AddWidget(UseLowQuality);

	ButtonWidget UseHighQuality("Use High Quality Settings");
	UseHighQuality.pOnEdited = VolumetricClouds::UseHighQualitySettings;
	pGuiWindow->AddWidget(UseHighQuality);

#if !defined(METAL)
	//CheckboxWidget Blur("Enabled Blur", &gAppSettings.m_EnableBlur);
	//pGuiWindow->AddWidget(Blur);
#endif

	CollapsingHeaderWidget CollapsingRayMarching("Ray Marching");

	CollapsingRayMarching.AddSubWidget(SliderUintWidget("Min Sample Iteration", &gAppSettings.m_MinSampleCount, 1, 256, 1));
	CollapsingRayMarching.AddSubWidget(SliderUintWidget("Max Sample Iteration", &gAppSettings.m_MaxSampleCount, 1, 1024, 1));
	CollapsingRayMarching.AddSubWidget(SliderFloatWidget("Min Step Size", &gAppSettings.m_MinStepSize, float(1.0f), float(2048.0f), float(32.0f)));
	CollapsingRayMarching.AddSubWidget(SliderFloatWidget("Max Step Size", &gAppSettings.m_MaxStepSize, float(1.0f), float(4096.0f), float(32.0f)));
	CollapsingRayMarching.AddSubWidget(SliderFloatWidget("Layer Height Offset", &gAppSettings.m_LayerHeightOffset, float(-100000.0f), float(100000.0f), float(100.0f)));
	CollapsingRayMarching.AddSubWidget(SliderFloatWidget("Layer Thickness", &gAppSettings.m_LayerThickness, float(1.0f), float(100000.0f), float(100.0f)));

	CollapsingRayMarching.AddSubWidget(CheckboxWidget("Enabled Temporal RayOffset", &gAppSettings.m_EnabledTemporalRayOffset));

	pGuiWindow->AddWidget(CollapsingRayMarching);


	CollapsingHeaderWidget CollapsingCloud("Cloud Modeling");

	CollapsingCloud.AddSubWidget(SliderFloatWidget("Base Cloud Tiling", &gAppSettings.m_BaseTile, float(0.001f), float(10.0f), float(0.001f)));
	CollapsingCloud.AddSubWidget(SliderFloatWidget("Detail Cloud Tiling", &gAppSettings.m_DetailTile, float(0.001f), float(20.0f), float(0.01f)));
	CollapsingCloud.AddSubWidget(SliderFloatWidget("Detail Strength", &gAppSettings.m_DetailStrength, float(0.001f), float(2.0f), float(0.001f)));
	CollapsingCloud.AddSubWidget(SliderFloatWidget("Cloud Top Offset", &gAppSettings.m_CloudTopOffset, float(0.01f), float(2000.0f), float(0.01f)));
	CollapsingCloud.AddSubWidget(SliderFloatWidget("Cloud Size", &gAppSettings.m_CloudSize, float(0.001f), float(100000.0f), float(0.1f)));
	CollapsingCloud.AddSubWidget(SliderFloatWidget("Cloud Density", &gAppSettings.m_CloudDensity, float(0.0f), float(10.0f), float(0.01f)));
	CollapsingCloud.AddSubWidget(SliderFloatWidget("Cloud Coverage Modifier", &gAppSettings.m_CloudCoverageModifier, float(-1.0f), float(1.0f), float(0.001f)));
	CollapsingCloud.AddSubWidget(SliderFloatWidget("Cloud Type Modifier", &gAppSettings.m_CloudTypeModifier, float(-1.0f), float(1.0f), float(0.001f)));
	CollapsingCloud.AddSubWidget(SliderFloatWidget("Anvil Bias", &gAppSettings.m_AnvilBias, float(0.0f), float(1.0f), float(0.001f)));

	pGuiWindow->AddWidget(CollapsingCloud);

	CollapsingHeaderWidget CollapsingCloudCurl("Cloud Curl");

	CollapsingCloudCurl.AddSubWidget(SliderFloatWidget("Curl Tiling", &gAppSettings.m_CurlTile, float(0.001f), float(4.0f), float(0.001f)));
	CollapsingCloudCurl.AddSubWidget(SliderFloatWidget("Curl Strength", &gAppSettings.m_CurlStrength, float(0.0f), float(10000.0f), float(0.1f)));

	CollapsingCloudCurl.AddSubWidget(SliderFloatWidget("UPstream Scale", &gAppSettings.m_UpStreamScale, float(0.0001f), float(0.001f), float(0.0001f)));
	CollapsingCloudCurl.AddSubWidget(SliderFloatWidget("UPstream Speed", &gAppSettings.m_UpStreamSpeed, float(-10.0f), float(10.0f), float(0.01f)));

	pGuiWindow->AddWidget(CollapsingCloudCurl);

	CollapsingHeaderWidget CollapsingCloudWeather("Weather");

	CollapsingCloudWeather.AddSubWidget(SliderFloatWidget("Weather Texture Size", &gAppSettings.m_WeatherTexSize, float(0.001f), float(1000000.0f), float(0.1f)));
	CollapsingCloudWeather.AddSubWidget(SliderFloatWidget("Weather Texture Direction", &gAppSettings.WeatherTextureAzimuth, float(-180.0f), float(180.0f), float(0.001f)));
	CollapsingCloudWeather.AddSubWidget(SliderFloatWidget("Weather Texture Distance", &gAppSettings.WeatherTextureDistance, float(-1000000.0f), float(1000000.0f), float(0.01f)));

	pGuiWindow->AddWidget(CollapsingCloudWeather);


	CollapsingHeaderWidget CollapsingWind("Wind");

	CollapsingWind.AddSubWidget(SliderFloatWidget("Wind Direction", &gAppSettings.m_WindAzimuth, float(-180.0f), float(180.0f), float(0.001f)));
	CollapsingWind.AddSubWidget(SliderFloatWidget("Wind Intensity", &gAppSettings.m_WindIntensity, float(0.0f), float(100.0f), float(0.001f)));
	CollapsingWind.AddSubWidget(SliderFloatWidget("Noise Flow Intensity", &gAppSettings.m_NoiseFlowIntensity, float(0.0f), float(100.0f), float(0.001f)));

	pGuiWindow->AddWidget(CollapsingWind);


	CollapsingHeaderWidget CollapsingCloudLighting("Cloud Lighting");

	CollapsingCloudLighting.AddSubWidget(ColorPickerWidget("Custom Color", &gAppSettings.m_CustomColor));
	CollapsingCloudLighting.AddSubWidget(SliderFloatWidget("Custom Color Intensity", &gAppSettings.m_CustomColorIntensity, float(0.0f), float(1.0f), float(0.01f)));
	CollapsingCloudLighting.AddSubWidget(SliderFloatWidget("Custom Color Blend", &gAppSettings.m_CustomColorBlendFactor, float(0.0f), float(1.0f), float(0.01f)));
	CollapsingCloudLighting.AddSubWidget(SliderFloatWidget("Trans Step Size", &gAppSettings.m_TransStepSize, float(0.0f), float(2048.0f), float(1.0f)));
	CollapsingCloudLighting.AddSubWidget(SliderFloatWidget("Contrast", &gAppSettings.m_Contrast, float(0.0f), float(2.0f), float(0.01f)));
	CollapsingCloudLighting.AddSubWidget(SliderFloatWidget("BackgroundBlendFactor", &gAppSettings.m_BackgroundBlendFactor, float(0.0f), float(1.0f), float(0.01f)));
	CollapsingCloudLighting.AddSubWidget(SliderFloatWidget("Precipitation", &gAppSettings.m_Precipitation, float(0.0f), float(10.0f), float(0.01f)));
	CollapsingCloudLighting.AddSubWidget(SliderFloatWidget("Silver Intensity", &gAppSettings.m_SilverIntensity, float(0.0f), float(2.0f), float(0.01f)));
	CollapsingCloudLighting.AddSubWidget(SliderFloatWidget("Silver Spread", &gAppSettings.m_SilverSpread, float(0.0f), float(0.99f), float(0.01f)));
	CollapsingCloudLighting.AddSubWidget(SliderFloatWidget("Eccentricity", &gAppSettings.m_Eccentricity, float(0.0f), float(1.0f), float(0.001f)));
	CollapsingCloudLighting.AddSubWidget(SliderFloatWidget("Cloud Brightness", &gAppSettings.m_CloudBrightness, float(0.0f), float(2.0f), float(0.01f)));

	pGuiWindow->AddWidget(CollapsingCloudLighting);


	CollapsingHeaderWidget CollapsingCulling("Depth Culling");

	CollapsingCulling.AddSubWidget(CheckboxWidget("Enabled Depth Culling", &gAppSettings.m_EnabledDepthCulling));
	CollapsingCulling.AddSubWidget(CheckboxWidget("Enabled Lod Depth", &gAppSettings.m_EnabledLodDepth));
	pGuiWindow->AddWidget(CollapsingCulling);

	CollapsingHeaderWidget CollapsingShadow("Clouds Shadow");

	CollapsingCulling.AddSubWidget(CheckboxWidget("Enabled Shadow", &gAppSettings.m_EnabledShadow));

	CollapsingShadow.AddSubWidget(SliderFloatWidget("Shadow Brightness", &gAppSettings.m_ShadowBrightness, float(-1.0f), float(1.0f), float(0.01f)));
	CollapsingShadow.AddSubWidget(SliderFloatWidget("Shadow Tiling", &gAppSettings.m_ShadowTiling, float(0.0f), float(100.0f), float(0.01f)));
	CollapsingShadow.AddSubWidget(SliderFloatWidget("Shadow Speed", &gAppSettings.m_ShadowSpeed, float(0.0f), float(100.0f), float(0.01f)));

	pGuiWindow->AddWidget(CollapsingShadow);


	CollapsingHeaderWidget CollapsingGodray("Clouds Godray");

	CollapsingGodray.AddSubWidget(CheckboxWidget("Enabled Godray", &gAppSettings.m_EnabledGodray));
	CollapsingGodray.AddSubWidget(SliderUintWidget("Number of Samples", &gAppSettings.m_GodNumSamples, 1, 256, 1));
	CollapsingGodray.AddSubWidget(SliderFloatWidget("Exposure", &gAppSettings.m_Exposure, float(0.0f), float(0.1f), float(0.0001f)));
	CollapsingGodray.AddSubWidget(SliderFloatWidget("Decay", &gAppSettings.m_Decay, float(0.0f), float(2.0f), float(0.001f)));
	CollapsingGodray.AddSubWidget(SliderFloatWidget("Density", &gAppSettings.m_Density, float(0.0f), float(2.0f), float(0.001f)));
	CollapsingGodray.AddSubWidget(SliderFloatWidget("Weight", &gAppSettings.m_Weight, float(0.0f), float(2.0f), float(0.01f)));

	pGuiWindow->AddWidget(CollapsingGodray);

	/*
	  CollapsingHeaderWidget CollapsingTest("Clouds Test");

	  CollapsingTest.AddSubWidget(SliderFloatWidget("Test00", &gAppSettings.m_Test00, float(-1.0f), float(1.0f), float(0.001f)));
	  CollapsingTest.AddSubWidget(SliderFloatWidget("Test01", &gAppSettings.m_Test01, float(0.0f), float(2.0f), float(0.001f)));
	  CollapsingTest.AddSubWidget(SliderFloatWidget("Test02", &gAppSettings.m_Test02, float(1.0f), float(200000.0f), float(100.0f)));
	  CollapsingTest.AddSubWidget(SliderFloatWidget("Test03", &gAppSettings.m_Test03, float(5000.0f), float(50000.0f), float(100.0f)));

	  pGuiWindow->AddWidget(CollapsingTest);
	*/
	///////////////////////////

#if defined(USE_CHEAP_SETTINGS)
	UseLowQualitySettings();
#else
	UseHighQualitySettings();
#endif

	PipelineDesc pipelineDesc;
	pipelineDesc.mType = PIPELINE_TYPE_COMPUTE;

	ComputePipelineDesc &comPipelineSettings = pipelineDesc.mComputeDesc;
	comPipelineSettings.pShaderProgram = pGenLowTopFreq3DtexShader;
	comPipelineSettings.pRootSignature = pGenFreq3DTexRootSignature;
	addPipeline(pRenderer, &pipelineDesc, &pGenLowTopFreq3DtexPipeline);

	comPipelineSettings.pShaderProgram = pGenHighTopFreq3DtexShader;
	comPipelineSettings.pRootSignature = pGenFreq3DTexRootSignature;
	addPipeline(pRenderer, &pipelineDesc, &pGenHighTopFreq3DtexPipeline);

	comPipelineSettings.pShaderProgram = pGenMipmapShader;
	comPipelineSettings.pRootSignature = pGenMipmapRootSignature;
	addPipeline(pRenderer, &pipelineDesc, &pGenMipmapPipeline);

	DescriptorSet*            pGenMipmapDescriptorSet = NULL;
	DescriptorSet*            pGenFreq3DTexDescriptorSet = NULL;

	{
		DescriptorSetDesc setDesc = { pGenMipmapRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, HighFreqMipCount + LowFreqMipCount };
		addDescriptorSet(pRenderer, &setDesc, &pGenMipmapDescriptorSet);
#if !defined(METAL)
		setDesc = { pGenFreq3DTexRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 2 };
#else
		setDesc = { pGenFreq3DTexRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, gHighFreq3DTextureSize + gLowFreq3DTextureSize };
#endif
		addDescriptorSet(pRenderer, &setDesc, &pGenFreq3DTexDescriptorSet);
	}

	Cmd* cmd = ppTransCmds[0];
	beginCmd(cmd);

	TextureBarrier barriersForNoiseStart[] = {
		{ pHighFrequency3DTextureOrigin, RESOURCE_STATE_UNORDERED_ACCESS },
		{ pLowFrequency3DTextureOrigin, RESOURCE_STATE_UNORDERED_ACCESS },
	};

	cmdResourceBarrier(cmd, 0, NULL, 2, barriersForNoiseStart);

	cmdBindPipeline(cmd, pGenHighTopFreq3DtexPipeline);

#if !defined(METAL)

	DescriptorData params[2] = {};
	params[0].pName = "SrcTexture";
	params[0].mCount = (uint32_t)gHighFrequencyOriginTexturePacked.size();
	params[0].ppTextures = gHighFrequencyOriginTexturePacked.data();
	params[1].pName = "DstTexture";
	params[1].ppTextures = &pHighFrequency3DTextureOrigin;
	updateDescriptorSet(pRenderer, 0, pGenFreq3DTexDescriptorSet, 2, params);
	cmdBindDescriptorSet(cmd, 0, pGenFreq3DTexDescriptorSet);
	uint32_t* pThreadGroupSize = pGenHighTopFreq3DtexShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
	cmdDispatch(cmd, gHighFreq3DTextureSize / pThreadGroupSize[0], gHighFreq3DTextureSize / pThreadGroupSize[1], gHighFreq3DTextureSize / pThreadGroupSize[2]);

#else

	uint32_t freqSetIndex = 0;
	struct
	{
		uint SliceNum;
	} SliceNumInfo;

	for (uint i = 0; i < gHighFreq3DTextureSize; i++, ++freqSetIndex)
	{
		SliceNumInfo.SliceNum = i;
		cmdBindPushConstants(cmd, pGenFreq3DTexRootSignature, "sliceRootConstant", &SliceNumInfo);

		DescriptorData params[2] = {};
		params[0].pName = "SrcTexture";
		params[0].ppTextures = &gHighFrequencyOriginTexture[i];
		params[1].pName = "DstTexture";
		params[1].ppTextures = &pHighFrequency3DTextureOrigin;
		updateDescriptorSet(pRenderer, freqSetIndex, pGenFreq3DTexDescriptorSet, 2, params);
		cmdBindDescriptorSet(cmd, freqSetIndex, pGenFreq3DTexDescriptorSet);

		uint32_t* pThreadGroupSize = pGenHighTopFreq3DtexShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;

		cmdDispatch(cmd, gHighFreq3DTextureSize / pThreadGroupSize[0], gHighFreq3DTextureSize / pThreadGroupSize[1], 1);
	}



#endif

	cmdBindPipeline(cmd, pGenLowTopFreq3DtexPipeline);

#if !defined(METAL)

	DescriptorData lowParams[2] = {};
	lowParams[0].pName = "SrcTexture";
	lowParams[0].mCount = (uint32_t)gLowFrequencyOriginTexturePacked.size();
	lowParams[0].ppTextures = gLowFrequencyOriginTexturePacked.data();
	lowParams[1].pName = "DstTexture";
	lowParams[1].ppTextures = &pLowFrequency3DTextureOrigin;
	updateDescriptorSet(pRenderer, 1, pGenFreq3DTexDescriptorSet, 2, lowParams);
	cmdBindDescriptorSet(cmd, 1, pGenFreq3DTexDescriptorSet);
	pThreadGroupSize = pGenLowTopFreq3DtexShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
	cmdDispatch(cmd, gLowFreq3DTextureSize / pThreadGroupSize[0], gLowFreq3DTextureSize / pThreadGroupSize[1], gLowFreq3DTextureSize / pThreadGroupSize[2]);

#else

	for (uint i = 0; i < gLowFreq3DTextureSize; i++, ++freqSetIndex)
	{
		SliceNumInfo.SliceNum = i;
		cmdBindPushConstants(cmd, pGenFreq3DTexRootSignature, "sliceRootConstant", &SliceNumInfo);

		DescriptorData lowParams[3] = {};
		lowParams[0].pName = "SrcTexture";
		lowParams[0].ppTextures = &gLowFrequencyOriginTexture[i];
		lowParams[1].pName = "DstTexture";
		lowParams[1].ppTextures = &pLowFrequency3DTextureOrigin;
		updateDescriptorSet(pRenderer, freqSetIndex, pGenFreq3DTexDescriptorSet, 2, lowParams);
		cmdBindDescriptorSet(cmd, freqSetIndex, pGenFreq3DTexDescriptorSet);
		uint32_t* pThreadGroupSize = pGenLowTopFreq3DtexShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
		cmdDispatch(cmd, gLowFreq3DTextureSize / pThreadGroupSize[0], gLowFreq3DTextureSize / pThreadGroupSize[1], 1);
	}
#endif

	TextureBarrier barriersForNoise[] = {
	  { pHighFrequency3DTextureOrigin, RESOURCE_STATE_SHADER_RESOURCE },
	  { pLowFrequency3DTextureOrigin, RESOURCE_STATE_SHADER_RESOURCE },
	};

	cmdResourceBarrier(cmd, 0, NULL, 2, barriersForNoise);


	cmdBindPipeline(cmd, pGenMipmapPipeline);

	struct Data
	{
		uint mip;
	} data = { 0 };

	TextureBarrier barriersForNoise2Start[] = {
	  { pHighFrequency3DTexture, RESOURCE_STATE_UNORDERED_ACCESS },
	  { pLowFrequency3DTexture, RESOURCE_STATE_UNORDERED_ACCESS },
	};

	cmdResourceBarrier(cmd, 0, NULL, 2, barriersForNoise2Start);

	uint32_t setIndex = 0;

	for (uint32_t i = 0; i < HighFreqMipCount; i++, ++setIndex)
	{
		data.mip = i;
		cmdBindPushConstants(cmd, pGenMipmapRootSignature, "RootConstant", &data);

		DescriptorData mipParams[3] = {};
		mipParams[0].pName = "SrcTexture";
		mipParams[0].ppTextures = &pHighFrequency3DTextureOrigin;
		mipParams[1].pName = "DstTexture";
		mipParams[1].ppTextures = &pHighFrequency3DTexture;
		mipParams[1].mUAVMipSlice = i;
		updateDescriptorSet(pRenderer, setIndex, pGenMipmapDescriptorSet, 2, mipParams);
		cmdBindDescriptorSet(cmd, setIndex, pGenMipmapDescriptorSet);

		uint mipMapSize = gHighFreq3DTextureSize / (uint32)pow(2, (i));

		cmdDispatch(cmd, mipMapSize, mipMapSize, mipMapSize);
	}

	for (uint32_t i = 0; i < LowFreqMipCount; i++, ++setIndex)
	{
		data.mip = i;
		cmdBindPushConstants(cmd, pGenMipmapRootSignature, "RootConstant", &data);

		DescriptorData mipParams[3] = {};
		mipParams[0].pName = "SrcTexture";
		mipParams[0].ppTextures = &pLowFrequency3DTextureOrigin;
		mipParams[1].pName = "DstTexture";
		mipParams[1].ppTextures = &pLowFrequency3DTexture;
		mipParams[1].mUAVMipSlice = i;
		updateDescriptorSet(pRenderer, setIndex, pGenMipmapDescriptorSet, 2, mipParams);
		cmdBindDescriptorSet(cmd, setIndex, pGenMipmapDescriptorSet);

		uint mipMapSize = gLowFreq3DTextureSize / (uint32)pow(2, (i));

		cmdDispatch(cmd, mipMapSize, mipMapSize, mipMapSize);
	}


	TextureBarrier barriersForNoise2[] = {
	  { pHighFrequency3DTexture, RESOURCE_STATE_SHADER_RESOURCE },
	  { pLowFrequency3DTexture, RESOURCE_STATE_SHADER_RESOURCE },
	};

	cmdResourceBarrier(cmd, 0, NULL, 2, barriersForNoise2);

	endCmd(cmd);
	queueSubmit(pGraphicsQueue, 1, &cmd, pTransitionCompleteFences, 0, 0, 0, 0);
	waitForFences(pRenderer, 1, &pTransitionCompleteFences);

	removeDescriptorSet(pRenderer, pGenMipmapDescriptorSet);
	removeDescriptorSet(pRenderer, pGenFreq3DTexDescriptorSet);

	return true;
}


void VolumetricClouds::Exit()
{
	removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetCompute[0]);
	removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetCompute[1]);
	removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetGraphics[0]);
	removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetGraphics[1]);

	RemoveUniformBuffers();

	removePipeline(pRenderer, pGenHighTopFreq3DtexPipeline);
	removePipeline(pRenderer, pGenLowTopFreq3DtexPipeline);
	removePipeline(pRenderer, pGenMipmapPipeline);

	removeResource(pHighFrequency3DTexture);
	removeResource(pLowFrequency3DTexture);

	removeResource(pHighFrequency3DTextureOrigin);
	removeResource(pLowFrequency3DTextureOrigin);

	removeResource(pWeatherTexture);
	removeResource(pWeatherCompactTexture);
	removeResource(pCurlNoiseTexture);

	removeBlendState(pBlendStateSkyBox);
	removeBlendState(pBlendStateGodray);

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

	conf_free(gHighFrequencyOriginTextureStorage);
	conf_free(gLowFrequencyOriginTextureStorage);

	gHighFrequencyOriginTexturePacked.set_capacity(0);
	gHighFrequencyOriginTexturePacked.clear();
	gLowFrequencyOriginTexturePacked.set_capacity(0);
	gLowFrequencyOriginTexturePacked.clear();

	gHighFrequencyOriginTexture.set_capacity(0);
	gHighFrequencyOriginTexture.clear();
	gLowFrequencyOriginTexture.set_capacity(0);
	gLowFrequencyOriginTexture.clear();

	//removePipeline(pRenderer, c);

	removeResource(pTriangularScreenVertexBuffer);

	//removeShader(pRenderer, pCompositeOverlayShader);
	//removeRootSignature(pRenderer, pCompositeOverlayRootSignature);
	//removeDescriptorBinder(pRenderer, pCompositeOverlayDescriptorBinder);



	removeSampler(pRenderer, pPointSampler);
	removeSampler(pRenderer, pBilinearSampler);
	removeSampler(pRenderer, pPointClampSampler);
	removeSampler(pRenderer, pBiClampSampler);
	removeSampler(pRenderer, pLinearBorderSampler);
	//removeShader(pRenderer, pSaveCurrentShader);	
	removeShader(pRenderer, pPostProcessShader);
	removeShader(pRenderer, pGenLowTopFreq3DtexShader);
	removeShader(pRenderer, pGenHighTopFreq3DtexShader);
	removeShader(pRenderer, pGenMipmapShader);
	removeShader(pRenderer, pVolumetricCloudShader);
	removeShader(pRenderer, pVolumetricCloudCompShader);
	removeShader(pRenderer, pVolumetricCloudWithDepthCompShader);
	removeShader(pRenderer, pReprojectionShader);
	removeShader(pRenderer, pGodrayShader);
	removeShader(pRenderer, pGodrayAddShader);
	//removeShader(pRenderer, pCastShadowShader);

	removeShader(pRenderer, pGenHiZMipmapShader);
	removeShader(pRenderer, pCopyTextureShader);
	removeShader(pRenderer, pCopyWeatherTextureShader);
	removeShader(pRenderer, pCopyRTShader);
	removeShader(pRenderer, pCompositeShader);
	removeShader(pRenderer, pGenHiZMipmapPRShader);

	removeShader(pRenderer, pHorizontalBlurShader);
	removeShader(pRenderer, pVerticalBlurShader);

	removeShader(pRenderer, pPostProcessWithBlurShader);

	removeRootSignature(pRenderer, pCopyTextureRootSignature);
	removeRootSignature(pRenderer, pCopyWeatherTextureRootSignature);

	removeRootSignature(pRenderer, pGenHiZMipmapRootSignature);
	removeRootSignature(pRenderer, pGenFreq3DTexRootSignature);

	removeRootSignature(pRenderer, pGenMipmapRootSignature);

	removeRootSignature(pRenderer, pVolumetricCloudsRootSignatureGraphics);
	removeRootSignature(pRenderer, pVolumetricCloudsRootSignatureCompute);

	removeRasterizerState(pRasterizer);
}

Texture* VolumetricClouds::GetWeatherMap()
{
	return pWeatherTexture;
};

bool VolumetricClouds::Load(RenderTarget** rts, uint32_t count)
{
	pFinalRT = rts;

	mWidth = (*rts)->mDesc.mWidth;
	mHeight = (*rts)->mDesc.mHeight;

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

	uint64_t screenDataSize = 4 * 3 * sizeof(float);
	BufferLoadDesc screenMiscVbDesc = {};
	screenMiscVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	screenMiscVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	screenMiscVbDesc.mDesc.mSize = screenDataSize;
	screenMiscVbDesc.mDesc.mVertexStride = sizeof(float) * 4;
	screenMiscVbDesc.pData = screenMiscPoints;
	screenMiscVbDesc.ppBuffer = &pTriangularScreenVertexWithMiscBuffer;
	addResource(&screenMiscVbDesc);

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

	PipelineDesc pipelineDescVolumetricCloud;
	{
		pipelineDescVolumetricCloud.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescVolumetricCloud.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &plowResCloudRT->mDesc.mFormat;
		//pipelineSettings.pSrgbValues = &plowResCloudRT->mDesc.mSrgb;
		pipelineSettings.mSampleCount = (*rts)->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mDesc.mSampleQuality;

		//pipelineSettings.pRootSignature = pVolumetricCloudRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pVolumetricCloudShader;
		pipelineSettings.pVertexLayout = &vertexLayoutForVC;
		pipelineSettings.pRasterizerState = pRasterizer;

		addPipeline(pRenderer, &pipelineDescVolumetricCloud, &pVolumetricCloudPipeline);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PipelineDesc pipelineDescReprojection;
	{
		pipelineDescReprojection.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescReprojection.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &phighResCloudRT->mDesc.mFormat;
		//pipelineSettings.pSrgbValues = &phighResCloudRT->mDesc.mSrgb;
		pipelineSettings.mSampleCount = (*rts)->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mDesc.mSampleQuality;

		// pipelineSettings.pRootSignature = pReprojectionRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pReprojectionShader;
		pipelineSettings.pVertexLayout = &vertexLayoutForVC;
		pipelineSettings.pRasterizerState = pRasterizer;

		addPipeline(pRenderer, &pipelineDescReprojection, &pReprojectionPipeline);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	pipelineSettings = { 0 };
	pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
	pipelineSettings.mRenderTargetCount = 1;
	pipelineSettings.pDepthState = NULL;
	pipelineSettings.pColorFormats = &pSaveCurrentRT->mDesc.mFormat;
	pipelineSettings.pSrgbValues = &pSaveCurrentRT->mDesc.mSrgb;
	pipelineSettings.mSampleCount = (*rts)->mDesc.mSampleCount;
	pipelineSettings.mSampleQuality = (*rts)->mDesc.mSampleQuality;
	pipelineSettings.pRootSignature = pSaveCurrentRootSignature;
	pipelineSettings.pShaderProgram = pSaveCurrentShader;
	pipelineSettings.pVertexLayout = &vertexLayout;
	pipelineSettings.pRasterizerState = pPostProcessRast;
	addPipeline(pRenderer, &pipelineSettings, &pSaveCurrentPipeline);
	*/
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PipelineDesc pipelineDescGodray;
	{
		pipelineDescGodray.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescGodray.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &pGodrayRT->mDesc.mFormat;
		//pipelineSettings.pSrgbValues = &pGodrayRT->mDesc.mSrgb;
		pipelineSettings.mSampleCount = (*rts)->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mDesc.mSampleQuality;

		//pipelineSettings.pRootSignature = pGodrayRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pGodrayShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = pRasterizer;

		addPipeline(pRenderer, &pipelineDescGodray, &pGodrayPipeline);
	}



	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PipelineDesc pipelineDescPostProcess;
	{
		pipelineDescPostProcess.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescPostProcess.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;

		TinyImageFormat mrtFormats[1] = {};
		bool mrtSrgb[1] = {};
		for (uint32_t i = 0; i < 1; ++i)
		{
			mrtFormats[i] = pRenderTargetsPostProcess[i]->mDesc.mFormat;
			//mrtSrgb[i] = pRenderTargetsPostProcess[i]->mDesc.mSrgb;
		}

		pipelineSettings.pColorFormats = mrtFormats;
		//pipelineSettings.pSrgbValues = mrtSrgb;
		pipelineSettings.mSampleCount = (*rts)->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mDesc.mSampleQuality;
		//pipelineSettings.pRootSignature = pPostProcessRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pPostProcessShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = pRasterizer;

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
	  pipelineSettings.pColorFormats = &pCastShadowRT->mDesc.mFormat;
	  pipelineSettings.pSrgbValues = &pCastShadowRT->mDesc.mSrgb;
	  pipelineSettings.mSampleCount = (*rts)->mDesc.mSampleCount;
	  pipelineSettings.mSampleQuality = (*rts)->mDesc.mSampleQuality;
	  pipelineSettings.pRootSignature = pCastShadowRootSignature;
	  pipelineSettings.pShaderProgram = pCastShadowShader;
	  pipelineSettings.pVertexLayout = &vertexLayout;
	  pipelineSettings.pRasterizerState = pRasterizer;

	  addPipeline(pRenderer, &pipelineDescCastShadow, &pCastShadowPipeline);
  }
*/



	PipelineDesc pipelineDescGodrayAdd;
	{
		pipelineDescGodrayAdd.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescGodrayAdd.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &(*rts)->mDesc.mFormat;
		//pipelineSettings.pSrgbValues = &(*rts)->mDesc.mSrgb;
		pipelineSettings.mSampleCount = (*rts)->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mDesc.mSampleQuality;
		//pipelineSettings.pRootSignature = pGodrayAddRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pGodrayAddShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = pRasterizer;

		pipelineSettings.pBlendState = pBlendStateGodray;

		addPipeline(pRenderer, &pipelineDescGodrayAdd, &pGodrayAddPipeline);
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PipelineDesc pipelineDescComposite;
	{
		pipelineDescComposite.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescComposite.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &(*rts)->mDesc.mFormat;
		//pipelineSettings.pSrgbValues = &(*rts)->mDesc.mSrgb;
		pipelineSettings.mSampleCount = (*rts)->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mDesc.mSampleQuality;
		//pipelineSettings.pRootSignature = pCompositeRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pCompositeShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = pRasterizer;

		pipelineSettings.pBlendState = pBlendStateSkyBox;

		addPipeline(pRenderer, &pipelineDescComposite, &pCompositePipeline);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	PipelineDesc pipelineDescCompositeOverlay;
	{
		pipelineDescCompositeOverlay.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescCompositeOverlay.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &(*rts)->mDesc.mFormat;
		//pipelineSettings.pSrgbValues = &(*rts)->mDesc.mSrgb;
		pipelineSettings.mSampleCount = (*rts)->mDesc.mSampleCount;
		pipelineSettings.mSampleQuality = (*rts)->mDesc.mSampleQuality;
		//pipelineSettings.pRootSignature = pCompositeOverlayRootSignature;
		pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
		pipelineSettings.pShaderProgram = pCompositeOverlayShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = pRasterizer;

		pipelineSettings.pBlendState = pBlendStateSkyBox;

		addPipeline(pRenderer, &pipelineDescCompositeOverlay, &pCompositeOverlayPipeline);
	}
*/

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PipelineDesc pipelineDesc;
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

	comPipelineSettings.pShaderProgram = pVolumetricCloudCompShader;
	comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
	addPipeline(pRenderer, &pipelineDesc, &pVolumetricCloudCompPipeline);

	comPipelineSettings.pShaderProgram = pVolumetricCloudWithDepthCompShader;
	comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
	addPipeline(pRenderer, &pipelineDesc, &pVolumetricCloudWithDepthCompPipeline);

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

	// Prepare descriptor sets
	// Hiz
	{
#if USE_DEPTH_CULLING
#if USE_LOD_DEPTH
		DescriptorData mipParams[2] = {};
		mipParams[0].pName = "SrcTexture";
		mipParams[0].ppTextures = &pLinearDepthTexture;
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
		DescriptorData VCParams[6] = {};
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
		VCParams[5].pName = "g_LinearDepthTexture";
		VCParams[5].ppTextures = &pLinearDepthTexture;
		updateDescriptorSet(pRenderer, 6, pVolumetricCloudsDescriptorSetGraphics[0], 6, VCParams);
#else
		DescriptorData VCParams[9] = {};
		VCParams[0].pName = "highFreqNoiseTexture";
		VCParams[0].ppTextures = &pHighFrequency3DTexture;
		VCParams[1].pName = "lowFreqNoiseTexture";
		VCParams[1].ppTextures = &pLowFrequency3DTexture;
		VCParams[2].pName = "curlNoiseTexture";
		VCParams[2].ppTextures = &pCurlNoiseTexture;
		VCParams[3].pName = "weatherTexture";
		VCParams[3].ppTextures = &pWeatherTexture;
		//VCParams[3].ppTextures = &pWeatherCompactTexture;
		VCParams[4].pName = "volumetricCloudsDstTexture";
		VCParams[4].ppTextures = &plowResCloudTexture;
		VCParams[5].pName = "depthTexture";
		VCParams[5].ppTextures = &HiZedDepthTexture;
		VCParams[6].pName = "g_LinearDepthTexture";
		VCParams[6].ppTextures = &pLinearDepthTexture;
		updateDescriptorSet(pRenderer, 1, pVolumetricCloudsDescriptorSetCompute[0], 7, VCParams);

		VCParams[5].pName = "depthTexture";
		VCParams[5].ppTextures = &pLinearDepthTexture;
		updateDescriptorSet(pRenderer, 2, pVolumetricCloudsDescriptorSetCompute[0], 6, VCParams);
#endif
		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			DescriptorData VCParams[1] = {};
			VCParams[0].pName = "VolumetricCloudsCBuffer";
			VCParams[0].ppBuffers = &VolumetricCloudsCBuffer[i];
			updateDescriptorSet(pRenderer, i, pVolumetricCloudsDescriptorSetCompute[1], 1, VCParams);
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
#else
#endif
	}
	// Copy RT
	{
		DescriptorData ppParams[3] = {};
		ppParams[0].pName = "SrcTexture";
		ppParams[0].ppTextures = &phighResCloudRT->pTexture;
		ppParams[1].pName = "SavePrevTexture";
		ppParams[1].ppTextures = &pSavePrevTexture;
		updateDescriptorSet(pRenderer, 3, pVolumetricCloudsDescriptorSetCompute[0], 2, ppParams);
	}
	// HBlur
	{
		DescriptorData params[3] = {};
		params[0].pName = "InputTex";
		params[0].ppTextures = &pSavePrevTexture;
		params[1].pName = "OutputTex";
		params[1].ppTextures = &pHBlurTex;
		updateDescriptorSet(pRenderer, 4, pVolumetricCloudsDescriptorSetCompute[0], 2, params);
	}
	// VBlur
	{
		DescriptorData params[3] = {};
		params[0].pName = "InputTex";
		params[0].ppTextures = &pHBlurTex;
		params[1].pName = "OutputTex";
		params[1].ppTextures = &pVBlurTex;
		updateDescriptorSet(pRenderer, 5, pVolumetricCloudsDescriptorSetCompute[0], 2, params);
	}
	// Post process
	{
		DescriptorData PPparams[4] = {};
		PPparams[0].pName = "g_SrcTexture2D";
		PPparams[0].ppTextures = &phighResCloudRT->pTexture;
		PPparams[1].pName = "g_SkyBackgroudTexture";
		PPparams[1].ppTextures = &pSceneColorTexture;
		PPparams[2].pName = "TransmittanceColor";
		PPparams[2].ppBuffers = &pTransmittanceBuffer;
#if !defined(METAL)
		PPparams[3].pName = "g_BlurTexture";
		PPparams[3].ppTextures = &pVBlurTex;
		updateDescriptorSet(pRenderer, 1, pVolumetricCloudsDescriptorSetGraphics[0], 4, PPparams);
		updateDescriptorSet(pRenderer, 2, pVolumetricCloudsDescriptorSetGraphics[0], 3, PPparams);
#else
		updateDescriptorSet(pRenderer, 1, pVolumetricCloudsDescriptorSetGraphics[0], 3, PPparams);
#endif
	}
	// Godray
	{
		DescriptorData PPparams[1] = {};
		PPparams[0].pName = "g_PrevFrameTexture";
		PPparams[0].ppTextures = &phighResCloudRT->pTexture;
		updateDescriptorSet(pRenderer, 3, pVolumetricCloudsDescriptorSetGraphics[0], 1, PPparams);
	}
	// Composite
	{
		DescriptorData Presentpparams[5] = {};
		Presentpparams[0].pName = "g_PostProcessedTexture";
		Presentpparams[0].ppTextures = &pPostProcessRT->pTexture;
		Presentpparams[1].pName = "depthTexture";
		Presentpparams[1].ppTextures = &pDepthTexture;
		Presentpparams[2].pName = "g_PrevVolumetricCloudTexture";
		Presentpparams[2].ppTextures = &phighResCloudRT->pTexture;
		updateDescriptorSet(pRenderer, 4, pVolumetricCloudsDescriptorSetGraphics[0], 3, Presentpparams);
	}
	// Add Godray
	{
		DescriptorData Presentpparams[4] = {};
		Presentpparams[0].pName = "g_GodrayTexture";
		Presentpparams[0].ppTextures = &pGodrayRT->pTexture;
		//Presentpparams[1].pName = "depthTexture";
		//Presentpparams[1].ppTextures = &pOriginDepthTexture;
		updateDescriptorSet(pRenderer, 5, pVolumetricCloudsDescriptorSetGraphics[0], 1, Presentpparams);
	}

	return true;
}

void VolumetricClouds::Unload()
{
	removePipeline(pRenderer, pPostProcessPipeline);
	removePipeline(pRenderer, pPostProcessWithBlurPipeline);
	//removePipeline(pRenderer, pCompositeOverlayPipeline);
	removePipeline(pRenderer, pCompositePipeline);
	removePipeline(pRenderer, pVolumetricCloudPipeline);
	removePipeline(pRenderer, pReprojectionPipeline);

	removePipeline(pRenderer, pGodrayPipeline);
	removePipeline(pRenderer, pGodrayAddPipeline);
	removePipeline(pRenderer, pCopyTexturePipeline);

	removePipeline(pRenderer, pGenHiZMipmapPipeline);
	removePipeline(pRenderer, pCopyRTPipeline);
	removePipeline(pRenderer, pVolumetricCloudCompPipeline);
	removePipeline(pRenderer, pVolumetricCloudWithDepthCompPipeline);
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

	removeRenderTarget(pRenderer, plowResCloudRT);
	removeRenderTarget(pRenderer, phighResCloudRT);
	removeRenderTarget(pRenderer, pPostProcessRT);
	//removeRenderTarget(pRenderer, pSaveCurrentRT);
	removeRenderTarget(pRenderer, pGodrayRT);
	removeRenderTarget(pRenderer, pCastShadowRT);

	removeResource(pSavePrevTexture);

#if defined(METAL)
	for (uint i = 0; i < gImageCount; i++)
	{
		removeResource(pSavePrevBuffer[i]);
	}
#endif

	removeResource(plowResCloudTexture);
	removeResource(phighResCloudTexture);
}

void VolumetricClouds::Draw(Cmd* cmd)
{
#if !defined(METAL)
	cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Volumetric Clouds + Post Process", true);
#endif

	{
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Volumetric Clouds", true);


		TextureBarrier barriers000[] = {
			{ pLinearDepthTexture, RESOURCE_STATE_SHADER_RESOURCE },
			{ pHiZDepthBuffer, RESOURCE_STATE_UNORDERED_ACCESS },
			{ pHiZDepthBufferX, RESOURCE_STATE_UNORDERED_ACCESS }
		};

		cmdResourceBarrier(cmd, 0, NULL, 3, barriers000);

		const uint32_t* pThreadGroupSize = NULL;

		{
#if USE_DEPTH_CULLING

#if USE_LOD_DEPTH


			cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Lodded Z DepthBuffer", true);

			cmdBindPipeline(cmd, pGenHiZMipmapPRPipeline);
			cmdBindDescriptorSet(cmd, 0, pVolumetricCloudsDescriptorSetCompute[0]);
			pThreadGroupSize = pGenHiZMipmapPRShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
			cmdDispatch(cmd, pHiZDepthBufferX->mDesc.mWidth, pHiZDepthBufferX->mDesc.mHeight, 1);

			cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);

#else
			struct Data
			{
				uint mip;
			} data = { 0 };


			cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Hi-Z DepthBuffer", true);

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
			cmdDispatch(cmd, (pDepthBuffer->pTexture->mDesc.mWidth / pThreadGroupSize[0]) + 1, pDepthBuffer->pTexture->mDesc.mHeight / pThreadGroupSize[1] + 1, 1);


			for (uint i = 0; i < pHiZDepthBuffer->mDesc.mMipLevels - 1; i++)
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
				uint newWidth = pDepthBuffer->pTexture->mDesc.mWidth / power;
				uint newHeight = pDepthBuffer->pTexture->mDesc.mHeight / power;

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

			cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
#endif

#endif
		}

		{
			cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Draw Clouds", true);

#if USE_LOD_DEPTH
			Texture* HiZedDepthTexture = pHiZDepthBufferX;
#else
			Texture* HiZedDepthTexture = pHiZDepthBuffer;
#endif


			RenderTarget* pRenderTarget = plowResCloudRT;

			TextureBarrier barriers0[] = {
				{ pRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ HiZedDepthTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pLowFrequency3DTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHighFrequency3DTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ plowResCloudTexture, RESOURCE_STATE_UNORDERED_ACCESS },
			};

			cmdResourceBarrier(cmd, 0, NULL, 5, barriers0);
#if USE_VC_FRAGMENTSHADER
			// simply record the screen cleaning command
			LoadActionsDesc loadActions = {};
			loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
			loadActions.mClearColorValues[0].r = 0.0f;
			loadActions.mClearColorValues[0].g = 0.0f;
			loadActions.mClearColorValues[0].b = 0.0f;
			loadActions.mClearColorValues[0].a = 0.0f;

			cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mDesc.mWidth, (float)pRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pRenderTarget->mDesc.mWidth, pRenderTarget->mDesc.mHeight);

			cmdBindPipeline(cmd, pVolumetricCloudPipeline);
			cmdBindDescriptorSet(cmd, 6, pVolumetricCloudsDescriptorSetGraphics[0]);
			cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
			cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexWithMiscBuffer, NULL);
			cmdDraw(cmd, 3, 0);

			cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);

#else
			if (gAppSettings.m_EnabledDepthCulling)
			{
				cmdBindPipeline(cmd, pVolumetricCloudCompPipeline);
				cmdBindDescriptorSet(cmd, 1, pVolumetricCloudsDescriptorSetCompute[0]);
				pThreadGroupSize = pVolumetricCloudCompShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
			}
			else
			{
				cmdBindPipeline(cmd, pVolumetricCloudWithDepthCompPipeline);
				cmdBindDescriptorSet(cmd, 2, pVolumetricCloudsDescriptorSetCompute[0]);
				pThreadGroupSize = pVolumetricCloudWithDepthCompShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
			}

			cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetCompute[1]);

			cmdDispatch(cmd, (uint32_t)ceil((float)plowResCloudTexture->mDesc.mWidth / (float)pThreadGroupSize[0]), (uint32_t)ceil((float)plowResCloudTexture->mDesc.mHeight / (float)pThreadGroupSize[1]), 1);
#endif

			cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
		}



		{
			cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Reprojection", true);

			/////////////////////////////////////////////////////////////////////////////


			TextureBarrier barriersForReprojection[] = {
				{ phighResCloudRT->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ phighResCloudTexture, RESOURCE_STATE_UNORDERED_ACCESS},
		#if USE_VC_FRAGMENTSHADER
				{ plowResCloudRT->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
		#else
				{ plowResCloudTexture, RESOURCE_STATE_SHADER_RESOURCE },
		#endif
				//{ pSaveCurrentRT->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pSavePrevTexture, RESOURCE_STATE_SHADER_RESOURCE },
			};

#if defined(METAL)
			BufferBarrier bufferBarrierForRep[] =
			{
				{ pSavePrevBuffer[0], RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 1, bufferBarrierForRep, 4, barriersForReprojection);
#else
			cmdResourceBarrier(cmd, 0, NULL, 4, barriersForReprojection);
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
			//loadActions.mClearDepth = pDepthBuffer->mDesc.mClearValue;

			cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mDesc.mWidth, (float)pRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pRenderTarget->mDesc.mWidth, pRenderTarget->mDesc.mHeight);

			cmdBindPipeline(cmd, pReprojectionPipeline);
			cmdBindDescriptorSet(cmd, 0, pVolumetricCloudsDescriptorSetGraphics[0]);
			cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);

			cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexWithMiscBuffer, NULL);
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
			cmdDispatch(cmd, (phighResCloudTexture->mDesc.mWidth / pThreadGroupSize[0]) + 1, (phighResCloudTexture->mDesc.mHeight / pThreadGroupSize[1]) + 1, 1);

#endif

			cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
		}

		//cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);


		{
			cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Save Current RenderTarget", true);

			TextureBarrier barriersForSaveRT[] = {
			{ phighResCloudRT->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
			{ pSavePrevTexture, RESOURCE_STATE_UNORDERED_ACCESS },
			};

#if defined(METAL)
			BufferBarrier bufferBarrierForRep[] =
			{
				{ pSavePrevBuffer[0], RESOURCE_STATE_UNORDERED_ACCESS }
			};

			cmdResourceBarrier(cmd, 1, bufferBarrierForRep, 2, barriersForSaveRT);
#else
			cmdResourceBarrier(cmd, 0, NULL, 2, barriersForSaveRT);
#endif

			cmdBindPipeline(cmd, pCopyRTPipeline);
			cmdBindDescriptorSet(cmd, 3, pVolumetricCloudsDescriptorSetCompute[0]);
			pThreadGroupSize = pCopyRTShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
			cmdDispatch(cmd, (uint32_t)ceil(phighResCloudRT->pTexture->mDesc.mWidth / pThreadGroupSize[0]), (uint32_t)ceil(phighResCloudRT->pTexture->mDesc.mHeight / pThreadGroupSize[1]), 1);

			TextureBarrier barriersForSaveRTEnd[] = {
			{ pSavePrevTexture, RESOURCE_STATE_SHADER_RESOURCE },
			};

#if defined(METAL)
			BufferBarrier bufferBarrierForRepEnd[] =
			{
				{ pSavePrevBuffer[0], RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 1, bufferBarrierForRepEnd, 1, barriersForSaveRTEnd);
#else
			cmdResourceBarrier(cmd, 0, NULL, 1, barriersForSaveRTEnd);
#endif
			cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
		}

#if !defined(METAL)
		TextureBarrier barriersForHBlur[] = {
			{ pHBlurTex, RESOURCE_STATE_UNORDERED_ACCESS },
			{ pVBlurTex, RESOURCE_STATE_SHADER_RESOURCE }
		};
		cmdResourceBarrier(cmd, 0, NULL, 2, barriersForHBlur);

		if (gAppSettings.m_EnableBlur)
		{
			cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Denoise - Blur", true);

			struct Data
			{
				uint width;
				uint height;
			} data = { 0 };

			data.width = pHBlurTex->mDesc.mWidth;
			data.height = pHBlurTex->mDesc.mHeight;

			cmdBindPipeline(cmd, pHorizontalBlurPipeline);
			cmdBindDescriptorSet(cmd, 4, pVolumetricCloudsDescriptorSetCompute[0]);
			cmdDispatch(cmd, 1, pHBlurTex->mDesc.mHeight, 1);

			TextureBarrier barriersForVBlur[] = {
				{ pHBlurTex, RESOURCE_STATE_SHADER_RESOURCE },
				{ pVBlurTex, RESOURCE_STATE_UNORDERED_ACCESS }
			};

			cmdResourceBarrier(cmd, 0, NULL, 2, barriersForVBlur);

			cmdBindPipeline(cmd, pVerticalBlurPipeline);
			cmdBindDescriptorSet(cmd, 5, pVolumetricCloudsDescriptorSetCompute[0]);
			cmdDispatch(cmd, pVBlurTex->mDesc.mWidth, 1, 1);

			cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
		}
#endif

		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}

	// PostProcess
	{
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "PostProcess", true);

		RenderTarget* pRenderTarget = pPostProcessRT;

		TextureBarrier barriersForPostProcess[] = {
			{ pRenderTargetsPostProcess[0]->pTexture, RESOURCE_STATE_RENDER_TARGET },
			{ phighResCloudRT->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
			{ pSceneColorTexture, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 3, barriersForPostProcess);

		LoadActionsDesc loadActions = {};
		loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
		//loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
		loadActions.mClearColorValues[0].r = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].b = 0.0f;
		loadActions.mClearColorValues[0].a = 0.0f;
		//loadActions.mClearDepth = pDepthBuffer->mDesc.mClearValue;

		cmdBindRenderTargets(cmd, 1, pRenderTargetsPostProcess, NULL, &loadActions, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mDesc.mWidth, (float)pRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pRenderTarget->mDesc.mWidth, pRenderTarget->mDesc.mHeight);

#if !defined(METAL)
		if (gAppSettings.m_EnableBlur)
		{
			cmdBindPipeline(cmd, pPostProcessWithBlurPipeline);
		}
		else
		{
			cmdBindPipeline(cmd, pPostProcessPipeline);
		}
#else
		cmdBindPipeline(cmd, pPostProcessPipeline);
#endif

		cmdBindDescriptorSet(cmd, (uint32_t)gAppSettings.m_EnableBlur + 1, pVolumetricCloudsDescriptorSetGraphics[0]);
		cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
		cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexBuffer, NULL);
		cmdDraw(cmd, 3, 0);

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}
	// PostProcess

	// Render Godray
	if (gAppSettings.m_EnabledGodray)
	{
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Render Godray", true);

		TextureBarrier barriersForGodray[] = {
		{ pGodrayRT->pTexture, RESOURCE_STATE_RENDER_TARGET }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriersForGodray);

		cmdBindRenderTargets(cmd, 1, &pGodrayRT, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pGodrayRT->mDesc.mWidth, (float)pGodrayRT->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pGodrayRT->mDesc.mWidth, pGodrayRT->mDesc.mHeight);

		cmdBindPipeline(cmd, pGodrayPipeline);
		cmdBindDescriptorSet(cmd, 3, pVolumetricCloudsDescriptorSetGraphics[0]);
		cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
		cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexBuffer, NULL);
		cmdDraw(cmd, 3, 0);

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);


		TextureBarrier barriersForGodrayEnd[] = {
		{ pGodrayRT->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriersForGodrayEnd);

		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}
	// Render Godray


	/*
	{
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Cast Shadow", true);

		pRenderTarget = pCastShadowRT;

		TextureBarrier barriers653[] = {
			{ pRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriers653, false);

		cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mDesc.mWidth, (float)pRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pRenderTarget->mDesc.mWidth, pRenderTarget->mDesc.mHeight);

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

		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}
	*/

	// Composite
	{
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Composite", true);

		RenderTarget* pRenderTarget = *pFinalRT;

		TextureBarrier barriersComposition[] = {
			{ pRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
			,{ pPostProcessRT->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			,{ pGodrayRT->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			,{ pCastShadowRT->pTexture, RESOURCE_STATE_SHADER_RESOURCE}
		};

		cmdResourceBarrier(cmd, 0, NULL, 4, barriersComposition);

		cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mDesc.mWidth, (float)pRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pRenderTarget->mDesc.mWidth, pRenderTarget->mDesc.mHeight);

		cmdBindPipeline(cmd, pCompositePipeline);
#if defined(METAL)
		cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
#endif
		cmdBindDescriptorSet(cmd, 4, pVolumetricCloudsDescriptorSetGraphics[0]);
		/*
				if (pCameraController->getViewPosition().getY() < (_CLOUDS_LAYER_START * 1.5f))
				{
					// Ground view

					cmdBindPipeline(cmd, pCompositePipeline);

					DescriptorData Presentpparams[4] = {};

					Presentpparams[0].pName = "g_PostProcessedTexture";
					Presentpparams[0].ppTextures = &pPostProcessRT->pTexture;
					Presentpparams[1].pName = "depthTexture";
					Presentpparams[1].ppTextures = &pOriginDepthTexture;
					Presentpparams[2].pName = "g_PrevVolumetricCloudTexture";
					Presentpparams[2].ppTextures = &pSavePrevTexture;
					Presentpparams[3].pName = "g_LinearClampSampler";
					Presentpparams[3].ppSamplers = &pBiClampSampler;
					cmdBindDescriptors(cmd, pCompositeDescriptorBinder, pCompositeRootSignature, 4, Presentpparams);
				}
				else
				{
					cmdBindPipeline(cmd, pCompositeOverlayPipeline);

					DescriptorData Presentpparams[2] = {};

					Presentpparams[0].pName = "g_PostProcessedTexture";
					Presentpparams[0].ppTextures = &pPostProcessRT->pTexture;
					Presentpparams[1].pName = "g_LinearClampSampler";
					Presentpparams[1].ppSamplers = &pBiClampSampler;
					cmdBindDescriptors(cmd, pCompositeOverlayDescriptorBinder, pCompositeOverlayRootSignature, 2, Presentpparams);
				}
		*/

		cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexBuffer, NULL);
		cmdDraw(cmd, 3, 0);

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);


		TextureBarrier barriersCompositionEnd[] = {
			{ pRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriersCompositionEnd);

		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}
	// Composite

	// Add Godray
	if (gAppSettings.m_EnabledGodray)
	{
		cmdBeginGpuTimestampQuery(cmd, pGraphicsGpuProfiler, "Add Godray", true);

		RenderTarget* pRenderTarget = *pFinalRT;

		TextureBarrier barriersAddGodray[] = {
			{ pRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
			,{ pGodrayRT->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 2, barriersAddGodray);

		cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mDesc.mWidth, (float)pRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pRenderTarget->mDesc.mWidth, pRenderTarget->mDesc.mHeight);

		cmdBindPipeline(cmd, pGodrayAddPipeline);
#if defined(METAL)
		cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
#endif
		cmdBindDescriptorSet(cmd, 5, pVolumetricCloudsDescriptorSetGraphics[0]);

		cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexBuffer, NULL);
		cmdDraw(cmd, 3, 0);

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		TextureBarrier barriersAddGodrayEnd[] = {
			{ pRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 1, barriersAddGodrayEnd);

		cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
	}
	// Add Godray
#if !defined(METAL)
	cmdEndGpuTimestampQuery(cmd, pGraphicsGpuProfiler);
#endif
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

	mat4 cloudViewMat_1st = GetViewCameraOffsetY(pCameraController, -gAppSettings.m_LayerHeightOffset);
	//mat4 cloudViewMat_2nd = GetViewCameraOffsetY(pCameraController, -gAppSettings.m_LayerHeightOffset);
	mat4 cloudPrevViewMat_1st = prevView;
	//mat4 cloudPrevViewMat_2nd = prevView;


	volumetricCloudsCB.m_WorldToProjMat_1st = projMat * cloudViewMat_1st;
	volumetricCloudsCB.m_PrevWorldToProjMat_1st = projMat * cloudPrevViewMat_1st;

	volumetricCloudsCB.m_ViewToWorldMat_1st = inverse(cloudViewMat_1st);
	volumetricCloudsCB.m_ProjToWorldMat_1st = volumetricCloudsCB.m_ViewToWorldMat_1st * inverse(projMat);

	volumetricCloudsCB.m_LightToProjMat_1st = mat4::identity(); //keylight_ctx->m_WorldToLight * VolumetricCloudsData.m_WorldToProjMat_1st;

	volumetricCloudsCB.m_JitterX = (uint32_t)offset[g_LowResFrameIndex].x;
	volumetricCloudsCB.m_JitterY = (uint32_t)offset[g_LowResFrameIndex].y;

	vec2 WeatherTexOffsets = GetDirectionXZ(gAppSettings.WeatherTextureAzimuth);
	volumetricCloudsCB.WeatherTextureOffsetX = WeatherTexOffsets.getX() * gAppSettings.WeatherTextureDistance;
	volumetricCloudsCB.WeatherTextureOffsetZ = WeatherTexOffsets.getY() * gAppSettings.WeatherTextureDistance;


	vec2 windXZ = GetDirectionXZ(gAppSettings.m_WindAzimuth);
	vec2 flowXZ = windXZ;

	float windIntensity = gAppSettings.m_WindIntensity * 0.1f * (float)deltaTime * 1000.0f;
	float flowIntensity = gAppSettings.m_NoiseFlowIntensity * 0.1f * (float)deltaTime * 1000.0f;

	g_StandardPosition += vec4(windXZ.getX() * windIntensity, windXZ.getY() * windIntensity, flowXZ.getX() * flowIntensity, flowXZ.getY() * flowIntensity);

	volumetricCloudsCB.StandardPosition = g_StandardPosition;

	vec4 lightDir = vec4(f3Tov3(LightDirection));

	volumetricCloudsCB.Test00 = lightDir.getY() < 0.0f ? 0.0f : 1.0f;

	lightDir = lightDir.getY() < 0.0f ? -lightDir : lightDir;
	lightDir.setW(gAppSettings.m_TransStepSize);

	volumetricCloudsCB.lightDirection = lightDir;


	volumetricCloudsCB.Test01 = gAppSettings.m_LayerHeightOffset;

	//gAppSettings.m_CustomColor
	vec4 customColor;

	uint32_t red = (gAppSettings.m_CustomColor & 0xFF000000) >> 24;
	uint32_t green = (gAppSettings.m_CustomColor & 0x00FF0000) >> 16;
	uint32_t blue = (gAppSettings.m_CustomColor & 0x0000FF00) >> 8;

	customColor = vec4((float)red / 255.0f, (float)green / 255.0f, (float)blue / 255.0f, gAppSettings.m_CustomColorIntensity);

	volumetricCloudsCB.lightColorAndIntensity = lerp(gAppSettings.m_CustomColorBlendFactor, f4Tov4(LightColorAndIntensity), customColor);

	volumetricCloudsCB.cameraPosition_1st = vec4(pCameraController->getViewPosition()) + vec4(0.0f, -gAppSettings.m_LayerHeightOffset, 0.0f, 0.0f);
	volumetricCloudsCB.cameraPosition_1st.setW(1.0f);
	volumetricCloudsCB.cameraPosition_2nd = vec4(pCameraController->getViewPosition()) + vec4(0.0f, -gAppSettings.m_LayerHeightOffset, 0.0f, 0.0f);
	volumetricCloudsCB.cameraPosition_2nd.setW(1.0f);

	volumetricCloudsCB.LayerThickness = gAppSettings.m_LayerThickness;

	volumetricCloudsCB.UpstreamScale = gAppSettings.m_UpStreamScale;
	volumetricCloudsCB.UpstreamSpeed = gAppSettings.m_UpStreamSpeed;

	volumetricCloudsCB.Padding01 = pCameraController->getViewPosition().getY() < (_CLOUDS_LAYER_START * 1.1f) ? 0.0f : 1.0f;

	if (gAppSettings.m_EnabledLodDepth && gAppSettings.m_EnabledDepthCulling)
	{
		volumetricCloudsCB.DepthMapWidth = pHiZDepthBufferX->mDesc.mWidth;
		volumetricCloudsCB.DepthMapHeight = pHiZDepthBufferX->mDesc.mHeight;
	}
	else
	{
		volumetricCloudsCB.DepthMapWidth = pLinearDepthTexture->mDesc.mWidth;
		volumetricCloudsCB.DepthMapHeight = pLinearDepthTexture->mDesc.mHeight;
	}


	volumetricCloudsCB.m_CorrectU = (float)(volumetricCloudsCB.m_JitterX) / ((mWidth / gDownsampledCloudSize) & (~31));
	volumetricCloudsCB.m_CorrectV = (float)(volumetricCloudsCB.m_JitterY) / ((mHeight / gDownsampledCloudSize) & (~31));

	volumetricCloudsCB.MIN_ITERATION_COUNT = gAppSettings.m_MinSampleCount;
	volumetricCloudsCB.MAX_ITERATION_COUNT = gAppSettings.m_MaxSampleCount;

	volumetricCloudsCB.m_UseRandomSeed = gAppSettings.m_EnabledTemporalRayOffset ? 1.0f : 0.0f;
	volumetricCloudsCB.m_StepSize = vec4(gAppSettings.m_MinStepSize, gAppSettings.m_MaxStepSize, 0.0f, 0.0f);

	//Cloud
	volumetricCloudsCB.CloudDensity = gAppSettings.m_CloudDensity;
	volumetricCloudsCB.CloudCoverage = gAppSettings.m_CloudCoverageModifier * gAppSettings.m_CloudCoverageModifier * gAppSettings.m_CloudCoverageModifier;
	volumetricCloudsCB.CloudType = gAppSettings.m_CloudTypeModifier * gAppSettings.m_CloudTypeModifier * gAppSettings.m_CloudTypeModifier;
	volumetricCloudsCB.CloudTopOffset = gAppSettings.m_CloudTopOffset;

	//Modeling
	volumetricCloudsCB.CloudSize = gAppSettings.m_CloudSize;
	volumetricCloudsCB.BaseShapeTiling = gAppSettings.m_BaseTile;
	volumetricCloudsCB.DetailShapeTiling = gAppSettings.m_DetailTile;
	volumetricCloudsCB.DetailStrenth = gAppSettings.m_DetailStrength;

	volumetricCloudsCB.CurlTextureTiling = gAppSettings.m_CurlTile;
	volumetricCloudsCB.CurlStrenth = gAppSettings.m_CurlStrength;
	volumetricCloudsCB.WeatherTextureSize = gAppSettings.m_WeatherTexSize;

	//Lighting
	volumetricCloudsCB.Contrast = gAppSettings.m_Contrast;
	volumetricCloudsCB.AnvilBias = gAppSettings.m_AnvilBias;
	volumetricCloudsCB.Eccentricity = gAppSettings.m_Eccentricity;
	volumetricCloudsCB.CloudBrightness = gAppSettings.m_CloudBrightness;
	volumetricCloudsCB.BackgroundBlendFactor = gAppSettings.m_BackgroundBlendFactor;
	volumetricCloudsCB.Precipitation = gAppSettings.m_Precipitation;
	volumetricCloudsCB.SilverliningIntensity = gAppSettings.m_SilverIntensity;
	volumetricCloudsCB.SilverliningSpread = gAppSettings.m_SilverSpread;

	g_ShadowInfo = vec4(gAppSettings.m_ShadowBrightness, gAppSettings.m_ShadowSpeed, gAppSettings.m_ShadowTiling, 0.0);

	//Wind
	volumetricCloudsCB.WindDirection = vec4(windXZ.getX(), 0.0, windXZ.getY(), gAppSettings.m_WindIntensity);

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
	BufferUpdateDesc BufferUniformSettingDesc = { VolumetricCloudsCBuffer[frameIndex], &volumetricCloudsCB };
	updateResource(&BufferUniformSettingDesc);
}

bool VolumetricClouds::AfterSubmit(uint currentFrameIndex)
{
	g_LowResFrameIndex = (g_LowResFrameIndex + 1) % (glowResBufferSize * glowResBufferSize);

	if (prevDownSampling != gAppSettings.m_DownSampling)
	{
		gDownsampledCloudSize = (uint)pow(2, gAppSettings.m_DownSampling);
		postProcessBufferSize = gDownsampledCloudSize;
		prevDownSampling = gAppSettings.m_DownSampling;
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
	TextureDesc HiZDepthDesc = {};
	HiZDepthDesc.mArraySize = 1;
	HiZDepthDesc.mFormat = TinyImageFormat_R16_SFLOAT;

	HiZDepthDesc.mWidth = mWidth & (~63);
	HiZDepthDesc.mHeight = mHeight & (~63);
	HiZDepthDesc.mDepth = 1;

	HiZDepthDesc.mMipLevels = 7;
	HiZDepthDesc.mSampleCount = SAMPLE_COUNT_1;
	//HiZDepthDesc.mSrgb = false;

	HiZDepthDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
	HiZDepthDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	HiZDepthDesc.pDebugName = L"HiZDepthBuffer A";
	HiZDepthDesc.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
	TextureLoadDesc HiZDepthTextureDesc = {};
	HiZDepthTextureDesc.ppTexture = &pHiZDepthBuffer;
	HiZDepthTextureDesc.pDesc = &HiZDepthDesc;
	addResource(&HiZDepthTextureDesc, false);


	HiZDepthDesc.pDebugName = L"HiZDepthBuffer B";
	HiZDepthTextureDesc.ppTexture = &pHiZDepthBuffer2;
	HiZDepthTextureDesc.pDesc = &HiZDepthDesc;
	addResource(&HiZDepthTextureDesc, false);

	HiZDepthDesc.mMipLevels = 1;
	HiZDepthDesc.mWidth = (mWidth  & (~63)) / 32;
	HiZDepthDesc.mHeight = (mHeight & (~63)) / 32;
	HiZDepthDesc.pDebugName = L"HiZDepthBuffer X";
	HiZDepthTextureDesc.ppTexture = &pHiZDepthBufferX;
	HiZDepthTextureDesc.pDesc = &HiZDepthDesc;
	addResource(&HiZDepthTextureDesc, false);


	return pHiZDepthBuffer != NULL && pHiZDepthBuffer2 != NULL && pHiZDepthBufferX != NULL;
}

bool VolumetricClouds::AddVolumetricCloudsRenderTargets()
{
	RenderTargetDesc HighResCloudRT = {};
	HighResCloudRT.mArraySize = 1;
	HighResCloudRT.mDepth = 1;
	HighResCloudRT.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
	HighResCloudRT.mSampleCount = SAMPLE_COUNT_1;
	HighResCloudRT.mSampleQuality = 0;

	HighResCloudRT.mWidth = (mWidth / gDownsampledCloudSize) & (~31);   //make sure the width and height could be divided by 4, otherwise the low-res buffer can't be aligned with full-res buffer
	HighResCloudRT.mHeight = (mHeight / gDownsampledCloudSize) & (~31);
	HighResCloudRT.pDebugName = L"HighResCloudRT";
	//HighResCloudRT.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
	addRenderTarget(pRenderer, &HighResCloudRT, &phighResCloudRT);

#if defined(VULKAN)
	TransitionRenderTargets(phighResCloudRT, ResourceState::RESOURCE_STATE_RENDER_TARGET, pRenderer, ppTransCmds[0], pGraphicsQueue, pTransitionCompleteFences);
#endif

	RenderTargetDesc PostProcessRT = {};
	PostProcessRT.mArraySize = 1;
	PostProcessRT.mDepth = 1;
	PostProcessRT.mFormat = TinyImageFormat_B10G11R11_UFLOAT;
	PostProcessRT.mSampleCount = SAMPLE_COUNT_1;
	PostProcessRT.mSampleQuality = 0;

	PostProcessRT.mWidth = (mWidth) & (~63);
	PostProcessRT.mHeight = (mHeight) & (~63);
	PostProcessRT.pDebugName = L"PostProcessRT";
	PostProcessRT.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
	addRenderTarget(pRenderer, &PostProcessRT, &pPostProcessRT);

#if defined(VULKAN)
	TransitionRenderTargets(pPostProcessRT, ResourceState::RESOURCE_STATE_RENDER_TARGET, pRenderer, ppTransCmds[0], pGraphicsQueue, pTransitionCompleteFences);
#endif

	pRenderTargetsPostProcess[0] = pPostProcessRT;

	RenderTargetDesc CurrentCloudRT = {};
	CurrentCloudRT.mArraySize = 1;
	CurrentCloudRT.mDepth = 1;
	CurrentCloudRT.mFormat = HighResCloudRT.mFormat;
	CurrentCloudRT.mSampleCount = SAMPLE_COUNT_1;
	CurrentCloudRT.mSampleQuality = 0;
	CurrentCloudRT.pDebugName = L"CurrentCloudRT";
	CurrentCloudRT.mWidth = HighResCloudRT.mWidth / glowResBufferSize;
	CurrentCloudRT.mHeight = HighResCloudRT.mHeight / glowResBufferSize;
	CurrentCloudRT.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
	addRenderTarget(pRenderer, &CurrentCloudRT, &plowResCloudRT);

#if defined(VULKAN)
	TransitionRenderTargets(plowResCloudRT, ResourceState::RESOURCE_STATE_RENDER_TARGET, pRenderer, ppTransCmds[0], pGraphicsQueue, pTransitionCompleteFences);
#endif

	RenderTargetDesc GodrayRT = {};
	GodrayRT.mArraySize = 1;
	GodrayRT.mDepth = 1;
	GodrayRT.mFormat = TinyImageFormat_B10G11R11_UFLOAT;
	GodrayRT.mSampleCount = SAMPLE_COUNT_1;
	GodrayRT.mSampleQuality = 0;
	GodrayRT.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
	GodrayRT.mWidth = mWidth / godRayBufferSize;
	GodrayRT.mHeight = mHeight / godRayBufferSize;

	addRenderTarget(pRenderer, &GodrayRT, &pGodrayRT);

#if defined(VULKAN)
	TransitionRenderTargets(pGodrayRT, ResourceState::RESOURCE_STATE_RENDER_TARGET, pRenderer, ppTransCmds[0], pGraphicsQueue, pTransitionCompleteFences);
#endif

	RenderTargetDesc CastShadowRT = {};
	CastShadowRT.mArraySize = 1;
	CastShadowRT.mDepth = 1;
	CastShadowRT.mFormat = TinyImageFormat_R8_UNORM;
	CastShadowRT.mSampleCount = SAMPLE_COUNT_1;
	CastShadowRT.mSampleQuality = 0;
	CastShadowRT.mClearValue.r = 1.0f;

	CastShadowRT.mWidth = mWidth / postProcessBufferSize;
	CastShadowRT.mHeight = mHeight / postProcessBufferSize;

	addRenderTarget(pRenderer, &CastShadowRT, &pCastShadowRT);

#if defined(VULKAN)
	TransitionRenderTargets(pCastShadowRT, ResourceState::RESOURCE_STATE_RENDER_TARGET, pRenderer, ppTransCmds[0], pGraphicsQueue, pTransitionCompleteFences);
#endif

	TextureDesc SaveTextureDesc = {};
	SaveTextureDesc.mArraySize = 1;
	SaveTextureDesc.mFormat = HighResCloudRT.mFormat;

	SaveTextureDesc.mWidth = HighResCloudRT.mWidth;
	SaveTextureDesc.mHeight = HighResCloudRT.mHeight;
	SaveTextureDesc.mDepth = HighResCloudRT.mDepth;

	SaveTextureDesc.mMipLevels = 1;
	SaveTextureDesc.mSampleCount = SAMPLE_COUNT_1;
	//SaveTextureDesc.mSrgb = false;

	SaveTextureDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
	SaveTextureDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE | DESCRIPTOR_TYPE_RW_TEXTURE;
	//SaveTextureDesc.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
	SaveTextureDesc.pDebugName = L"SaveTexture";

	TextureLoadDesc SaveTextureLoadDesc = {};
	SaveTextureLoadDesc.ppTexture = &pSavePrevTexture;
	SaveTextureLoadDesc.pDesc = &SaveTextureDesc;
	addResource(&SaveTextureLoadDesc, false);


#if defined(METAL)

	BufferLoadDesc SaveBufferLoadDesc = {};
	SaveBufferLoadDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER;
	SaveBufferLoadDesc.mDesc.mElementCount = HighResCloudRT.mWidth * HighResCloudRT.mHeight;
	SaveBufferLoadDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	SaveBufferLoadDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	SaveBufferLoadDesc.mDesc.mStructStride = sizeof(float) * 4; // 64bits
	SaveBufferLoadDesc.mDesc.mSize = SaveBufferLoadDesc.mDesc.mElementCount * SaveBufferLoadDesc.mDesc.mStructStride;
	SaveBufferLoadDesc.mDesc.pDebugName = L"Save Buffer";

	for (uint i = 0; i < gImageCount; i++)
	{
		SaveBufferLoadDesc.ppBuffer = &pSavePrevBuffer[i];
		addResource(&SaveBufferLoadDesc, false);
	}
#endif

	TextureDesc lowResTextureDesc = {};
	lowResTextureDesc.mArraySize = 1;
	lowResTextureDesc.mFormat = CurrentCloudRT.mFormat;

	lowResTextureDesc.mWidth = CurrentCloudRT.mWidth;
	lowResTextureDesc.mHeight = CurrentCloudRT.mHeight;
	lowResTextureDesc.mDepth = CurrentCloudRT.mDepth;

	lowResTextureDesc.mMipLevels = 1;
	lowResTextureDesc.mSampleCount = SAMPLE_COUNT_1;
	//lowResTextureDesc.mSrgb = false;

	lowResTextureDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
	lowResTextureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	lowResTextureDesc.pDebugName = L"low Res Texture";
	lowResTextureDesc.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
	TextureLoadDesc lowResLoadDesc = {};
	lowResLoadDesc.ppTexture = &plowResCloudTexture;
	lowResLoadDesc.pDesc = &lowResTextureDesc;
	addResource(&lowResLoadDesc, false);

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
	highResTextureDesc.pDebugName = L"high Res Texture";
	highResTextureDesc.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;

	TextureLoadDesc highResLoadDesc = {};
	highResLoadDesc.ppTexture = &phighResCloudTexture;
	highResLoadDesc.pDesc = &highResTextureDesc;
	addResource(&highResLoadDesc, false);



	TextureDesc blurTextureDesc = {};
	blurTextureDesc.mArraySize = 1;
	blurTextureDesc.mFormat = pSavePrevTexture->mDesc.mFormat;

	blurTextureDesc.mWidth = pSavePrevTexture->mDesc.mWidth / 2;
	blurTextureDesc.mHeight = pSavePrevTexture->mDesc.mHeight / 2;
	blurTextureDesc.mDepth = pSavePrevTexture->mDesc.mDepth;

	blurTextureDesc.mMipLevels = 1;
	blurTextureDesc.mSampleCount = SAMPLE_COUNT_1;
	//blurTextureDesc.mSrgb = false;

	blurTextureDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
	blurTextureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
	blurTextureDesc.pDebugName = L"H Blur Texture";


	TextureLoadDesc blurTextureLoadDesc = {};
	blurTextureLoadDesc.ppTexture = &pHBlurTex;
	blurTextureLoadDesc.pDesc = &blurTextureDesc;
	addResource(&blurTextureLoadDesc, false);


	blurTextureDesc.pDebugName = L"V Blur Texture";

	blurTextureLoadDesc = {};
	blurTextureLoadDesc.ppTexture = &pVBlurTex;
	blurTextureLoadDesc.pDesc = &blurTextureDesc;
	addResource(&blurTextureLoadDesc, false);

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
	ubSettingDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	ubSettingDesc.pData = NULL;

	for (uint i = 0; i < gImageCount; i++)
	{
		ubSettingDesc.ppBuffer = &VolumetricCloudsCBuffer[i];
		addResource(&ubSettingDesc);
	}
}

void VolumetricClouds::RemoveUniformBuffers()
{
	for (uint i = 0; i < gImageCount; i++)
	{
		removeResource(VolumetricCloudsCBuffer[i]);
	}
}

void VolumetricClouds::InitializeWithLoad(Texture* InLinearDepthTexture, Texture* InSceneColorTexture, Texture* InDepthTexture)
{
	pLinearDepthTexture = InLinearDepthTexture;
	pSceneColorTexture = InSceneColorTexture;
	pDepthTexture = InDepthTexture;
}

void VolumetricClouds::Initialize(uint InImageCount,
	ICameraController* InCameraController, Queue*	InGraphicsQueue,
	Cmd** InTransCmds, Fence* InTransitionCompleteFences, Fence** InRenderCompleteFences, GpuProfiler* InGraphicsGpuProfiler, UIApp* InGAppUI, Buffer*	InTransmittanceBuffer)
{
	gImageCount = InImageCount;

	pCameraController = InCameraController;
	pGraphicsQueue = InGraphicsQueue;
	ppTransCmds = InTransCmds;
	pTransitionCompleteFences = InTransitionCompleteFences;
	pRenderCompleteFences = InRenderCompleteFences;
	pGraphicsGpuProfiler = InGraphicsGpuProfiler;
	pGAppUI = InGAppUI;
	pTransmittanceBuffer = InTransmittanceBuffer;

}

void VolumetricClouds::UseLowQualitySettings()
{
	gAppSettings.m_DownSampling = 2;
	gAppSettings.m_MinSampleCount = 54;
	gAppSettings.m_MaxSampleCount = 96;
	gAppSettings.m_MinStepSize = 1024.0;
	gAppSettings.m_MaxStepSize = 2048.0;
	gAppSettings.m_EnabledShadow = false;
	gAppSettings.m_EnabledGodray = false;
	gAppSettings.m_EnableBlur = true;
}

void VolumetricClouds::UseHighQualitySettings()
{
	gAppSettings.m_DownSampling = 1;
	gAppSettings.m_MinSampleCount = 128;
	gAppSettings.m_MaxSampleCount = 256;
	gAppSettings.m_MinStepSize = 512.0;
	gAppSettings.m_MaxStepSize = 1024.0;
	gAppSettings.m_EnabledShadow = true;
	gAppSettings.m_EnabledGodray = true;
	gAppSettings.m_EnableBlur = false;
}
