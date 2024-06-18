/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#include "VolumetricClouds.h"

#include "../../../../The-Forge/Common_3/Application/Interfaces/IUI.h"
#include "../../../../The-Forge/Common_3/Game/Interfaces/IScripting.h"
#include "../../../../The-Forge/Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/ILog.h"

#include "../../../../The-Forge/Common_3/Utilities/RingBuffer.h"
#include "../../src/AppSettings.h"

#include "../../../../The-Forge/Common_3/Utilities/Interfaces/IMemory.h"

extern AppSettings  gAppSettings;
extern UIComponent* pGuiCloudWindow;

#define _CLOUDS_LAYER_START     15000.0f // The height where the clouds' layer get started
#define _CLOUDS_LAYER_THICKNESS 20000.0f // The height where the clouds' layer covers
#define _CLOUDS_LAYER_END \
    (_CLOUDS_LAYER_START + _CLOUDS_LAYER_THICKNESS) // The height where the clouds' layer get ended (End = Start + Thickness)
#define USE_DEPTH_CULLING     1
#define USE_LOD_DEPTH         1
#define USE_VC_FRAGMENTSHADER 0 // 0: compute shaders 1: fragment shaders

const uint32_t glowResBufferSize = 4;
const uint32_t godRayBufferSize = 8;

Texture* pHiZDepthBuffer = NULL;
Texture* pHiZDepthBuffer2 = NULL;

Texture* pHiZDepthBufferX = NULL;

RenderTarget* plowResCloudRT = NULL;
RenderTarget* phighResCloudRT = NULL;
Texture*      plowResCloudTexture = NULL;
RenderTarget* pReprojectedCloudRT = NULL;
Texture*      pReprojectedCloudTexture = NULL;

Buffer* pTriangularScreenVertexBuffer = NULL;
Buffer* pTriangularScreenVertexWithMiscBuffer = NULL;

RenderTarget* pPostProcessRT = NULL;
RenderTarget* pRenderTargetsPostProcess[2] = { nullptr };

Shader*        pGenHiZMipmapShader = NULL;
Pipeline*      pGenHiZMipmapPipeline = NULL;
RootSignature* pGenHiZMipmapRootSignature = NULL;

Shader*        pCopyTextureShader = NULL;
Pipeline*      pCopyTexturePipeline = NULL;
RootSignature* pCopyTextureRootSignature = NULL;

// Shader*                   pCopyWeatherTextureShader = NULL;
// Pipeline*                 pCopyWeatherTexturePipeline = NULL;
// RootSignature*            pCopyWeatherTextureRootSignature = NULL;

// Draw stage

// Graphics
Shader*   pVolumetricCloudShader = NULL;
Pipeline* pVolumetricCloudPipeline = NULL;

Shader*   pPostProcessShader = NULL;
Pipeline* pPostProcessPipeline = NULL;

Shader*   pPostProcessWithBlurShader = NULL;
Pipeline* pPostProcessWithBlurPipeline = NULL;

Shader*   pGodrayShader = NULL;
Pipeline* pGodrayPipeline = NULL;

Shader*   pGodrayAddShader = NULL;
Pipeline* pGodrayAddPipeline = NULL;

Shader*   pCompositeShader = NULL;
Pipeline* pCompositePipeline = NULL;

Shader*   pCompositeOverlayShader = NULL;
Pipeline* pCompositeOverlayPipeline = NULL;

RootSignature* pVolumetricCloudsRootSignatureGraphics = NULL;
DescriptorSet* pVolumetricCloudsDescriptorSetGraphics[2] = { NULL };

#if USE_VC_FRAGMENTSHADER
/*********Volumetric cloud generation fragment shaders pipeline**********/
Shader*   pVolumetricCloudWithDepthShader = NULL;
Pipeline* pVolumetricCloudWithDepthPipeline = NULL;

Shader*   pVolumetricCloud2ndShader = NULL;
Pipeline* pVolumetricCloud2ndPipeline = NULL;

Shader*   pVolumetricCloud2ndWithDepthShader = NULL;
Pipeline* pVolumetricCloud2ndWithDepthPipeline = NULL;

Shader*   pRealTimeVolumetricCloudShader = NULL;
Pipeline* pRealTimeVolumetricCloudPipeline = NULL;

Shader*   pRealTimeVolumetricCloudWithDepthShader = NULL;
Pipeline* pRealTimeVolumetricCloudWithDepthPipeline = NULL;
/************************************************************************/
#else
/**********Volumetric cloud generation compute shaders pipeline**********/
Shader*   pVolumetricCloudCompShader = NULL;
Pipeline* pVolumetricCloudCompPipeline = NULL;

Shader*   pVolumetricCloud2ndCompShader = NULL;
Pipeline* pVolumetricCloud2ndCompPipeline = NULL;

Shader*   pVolumetricCloudWithDepthCompShader = NULL;
Pipeline* pVolumetricCloudWithDepthCompPipeline = NULL;

Shader*   pVolumetricCloud2ndWithDepthCompShader = NULL;
Pipeline* pVolumetricCloud2ndWithDepthCompPipeline = NULL;

Shader*   pRealTimeVolumetricCloudCompShader = NULL;
Pipeline* pRealTimeVolumetricCompPipeline = NULL;

Shader*        pRealTimeVolumetricCloudWithDepthCompShader = NULL;
Pipeline*      pRealTimeVolumetricWithDepthCompPipeline = NULL;
/*************************************************************************/
#endif

// Comp
Shader*   pGenHiZMipmapPRShader = NULL;
Pipeline* pGenHiZMipmapPRPipeline = NULL;

Shader*   pReprojectionShader = NULL;
Pipeline* pReprojectionPipeline = NULL;

Shader*   pCopyRTShader = NULL;
Pipeline* pCopyRTPipeline = NULL;

Shader*   pHorizontalBlurShader = NULL;
Pipeline* pHorizontalBlurPipeline = NULL;

Shader*   pVerticalBlurShader = NULL;
Pipeline* pVerticalBlurPipeline = NULL;

RootSignature* pVolumetricCloudsRootSignatureCompute = NULL;
DescriptorSet* pVolumetricCloudsDescriptorSetCompute[2] = { NULL };

#if USE_VC_FRAGMENTSHADER
const uint32_t gShaderCount = 13;
const uint32_t gCompShaderCount = 4;
#else
const uint32_t gShaderCount = 8;
const uint32_t gCompShaderCount = 10;
#endif

Texture* pHBlurTex;
Texture* pVBlurTex;

RenderTarget* pGodrayRT = NULL;

DescriptorSet* pVolumetricCloudsDescriptorSet = NULL;

Sampler* pBilinearSampler = NULL;
Sampler* pPointSampler = NULL;

Sampler* pBiClampSampler = NULL;
Sampler* pPointClampSampler = NULL;
Sampler* pHiZDepthClampSampler = NULL;

Sampler* pLinearBorderSampler = NULL;

const uint32_t gHighFreq3DTextureSize = 32;
const uint32_t gLowFreq3DTextureSize = 128;

Texture** gHighFrequencyOriginTextureStorage = NULL;
Texture** gLowFrequencyOriginTextureStorage = NULL;

char gHighFrequencyTextureNames[gHighFreq3DTextureSize][128];
char gLowFrequencyTextureNames[gLowFreq3DTextureSize][128];

Texture* pHighFrequency3DTexture;
Texture* pLowFrequency3DTexture;

Texture* pWeatherTexture;
Texture* pWeatherCompactTexture;
Texture* pCurlNoiseTexture;

const uint32_t HighFreqDimension = 32;
const uint32_t LowFreqDimension = 128;
const uint32_t HighFreqMipCount = (uint32_t)log2(HighFreqDimension) + 1;
const uint32_t LowFreqMipCount = (uint32_t)log2(LowFreqDimension) + 1;

const uint32_t gTriangleVbStride = sizeof(float) * 5;

static TextureDesc HiZDepthDesc = {};
static TextureDesc lowResTextureDesc = {};
static TextureDesc blurTextureDesc = {};

static vec3 prevCameraPos;
static mat4 prevViewWithoutTranslation;
static mat4 projMat;

static float4 g_ProjectionExtents;
static float  g_currentTime = 0.0f;

float4 calcSkyBetaR(float sunlocationY, float rayleigh)
{
    float3 totalRayleigh = float3(5.804542996261093E-6f, 1.3562911419845635E-5f, 3.0265902468824876E-5f);

    float sunFade = 1.0f - clamp(1.0f - exp(sunlocationY), 0.0f, 1.0f);
    return float4(totalRayleigh * (rayleigh - 1.f + sunFade), sunFade);
}

float4 calcSkyBetaV(float turbidity)
{
    float3 MIE_CONST = float3(1.8399918514433978E14f, 2.7798023919660528E14f, 4.0790479543861094E14f);
    float  c = (0.2f * turbidity) * 10E-18f;
    return float4(0.434f * c * MIE_CONST, 0.0f);
}

static uint g_LowResFrameIndex = 0;
// static uint haltonSequenceIndex = 0;

Buffer* VolumetricCloudsCBuffer[VolumetricClouds::gDataBufferCount];

static int2 offset[] = { { 2, 1 }, { 1, 2 }, { 2, 0 }, { 0, 1 }, { 2, 3 }, { 3, 2 }, { 3, 1 }, { 0, 3 },
                         { 1, 0 }, { 1, 1 }, { 3, 3 }, { 0, 0 }, { 2, 2 }, { 1, 3 }, { 3, 0 }, { 0, 2 } };

/*
static int haltonSequence[] =
{
    8, 4, 12, 2, 10, 6, 14, 1
};
*/

bool VolumetricClouds::Init(Renderer* renderer, PipelineCache* pCache)
{
    pRenderer = renderer;
    pPipelineCache = pCache;

    g_StandardPosition = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    g_StandardPosition_2nd = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    g_ShadowInfo = vec4(0.0f, 0.0f, 1.0f, 0.0f);
    srand((uint)time(NULL));

    SamplerDesc samplerDesc = { FILTER_LINEAR,       FILTER_LINEAR,       MIPMAP_MODE_LINEAR,
                                ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT };
    addSampler(pRenderer, &samplerDesc, &pBilinearSampler);

    SamplerDesc PointSamplerDesc = { FILTER_NEAREST,      FILTER_NEAREST,      MIPMAP_MODE_LINEAR,
                                     ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT };
    addSampler(pRenderer, &PointSamplerDesc, &pPointSampler);

    SamplerDesc samplerClampDesc = {
        FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE
    };
    addSampler(pRenderer, &samplerClampDesc, &pBiClampSampler);

    SamplerDesc PointSamplerClampDesc = { FILTER_NEAREST,
                                          FILTER_NEAREST,
                                          MIPMAP_MODE_LINEAR,
                                          ADDRESS_MODE_CLAMP_TO_EDGE,
                                          ADDRESS_MODE_CLAMP_TO_EDGE,
                                          ADDRESS_MODE_CLAMP_TO_EDGE };
    addSampler(pRenderer, &PointSamplerClampDesc, &pPointClampSampler);

    SamplerDesc LinearBorderSamplerClampDesc = { FILTER_LINEAR,
                                                 FILTER_LINEAR,
                                                 MIPMAP_MODE_LINEAR,
                                                 ADDRESS_MODE_CLAMP_TO_BORDER,
                                                 ADDRESS_MODE_CLAMP_TO_BORDER,
                                                 ADDRESS_MODE_CLAMP_TO_BORDER };
    addSampler(pRenderer, &LinearBorderSamplerClampDesc, &pLinearBorderSampler);

    SamplerDesc nearestClampDesc = { FILTER_NEAREST,
                                     FILTER_NEAREST,
                                     MIPMAP_MODE_NEAREST,
                                     ADDRESS_MODE_CLAMP_TO_EDGE,
                                     ADDRESS_MODE_CLAMP_TO_EDGE,
                                     ADDRESS_MODE_CLAMP_TO_EDGE };
    addSampler(pRenderer, &nearestClampDesc, &pHiZDepthClampSampler);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    float screenQuadPoints[] = {
        -1.0f, 3.0f, 0.5f, 0.0f, -1.0f, -1.0f, -1.0f, 0.5f, 0.0f, 1.0f, 3.0f, -1.0f, 0.5f, 2.0f, 1.0f,
    };

    SyncToken token = {};

    uint64_t       screenQuadDataSize = 5 * 3 * sizeof(float);
    BufferLoadDesc screenQuadVbDesc = {};
    screenQuadVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    screenQuadVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    screenQuadVbDesc.mDesc.mSize = screenQuadDataSize;
    screenQuadVbDesc.pData = screenQuadPoints;
    screenQuadVbDesc.ppBuffer = &pTriangularScreenVertexBuffer;
    addResource(&screenQuadVbDesc, &token);

    gHighFrequencyOriginTextureStorage = (Texture**)tf_malloc(sizeof(Texture*) * gHighFreq3DTextureSize);

    for (uint32_t i = 0; i < gHighFreq3DTextureSize; ++i)
    {
        snprintf(gHighFrequencyTextureNames[i], 128, "VolumetricClouds/hiResCloudShape/hiResClouds (%u).tex", i);

        TextureLoadDesc highFrequencyOrigin3DTextureDesc = {};
        highFrequencyOrigin3DTextureDesc.pFileName = gHighFrequencyTextureNames[i];
        highFrequencyOrigin3DTextureDesc.ppTexture = &gHighFrequencyOriginTextureStorage[i];
        addResource(&highFrequencyOrigin3DTextureDesc, &token);
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    gLowFrequencyOriginTextureStorage = (Texture**)tf_malloc(sizeof(Texture*) * gLowFreq3DTextureSize);

    for (uint32_t i = 0; i < gLowFreq3DTextureSize; ++i)
    {
        snprintf(gLowFrequencyTextureNames[i], 128, "VolumetricClouds/lowResCloudShape/lowResCloud(%u).tex", i);

        TextureLoadDesc lowFrequencyOrigin3DTextureDesc = {};
        lowFrequencyOrigin3DTextureDesc.pFileName = gLowFrequencyTextureNames[i];
        lowFrequencyOrigin3DTextureDesc.ppTexture = &gLowFrequencyOriginTextureStorage[i];
        addResource(&lowFrequencyOrigin3DTextureDesc, &token);
    }

    waitForToken(&token);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    TextureDesc lowFreqImgDesc = {};
    lowFreqImgDesc.mArraySize = 1;
    lowFreqImgDesc.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;

    lowFreqImgDesc.mWidth = gLowFreq3DTextureSize;
    lowFreqImgDesc.mHeight = gLowFreq3DTextureSize;
    lowFreqImgDesc.mDepth = gLowFreq3DTextureSize;

    lowFreqImgDesc.mMipLevels = LowFreqMipCount;
    lowFreqImgDesc.mSampleCount = SAMPLE_COUNT_1;
    // lowFreqImgDesc.mSrgb = false;

    lowFreqImgDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
    lowFreqImgDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
    lowFreqImgDesc.pName = "LowFrequency3DTexture";

    TextureLoadDesc lowFrequency3DTextureDesc = {};
    lowFrequency3DTextureDesc.ppTexture = &pLowFrequency3DTexture;
    lowFrequency3DTextureDesc.pDesc = &lowFreqImgDesc;
    addResource(&lowFrequency3DTextureDesc, &token);
    //////////////////////////////////////////////////////////////////////////////////////////////

    TextureDesc highFreqImgDesc = {};
    highFreqImgDesc.mArraySize = 1;
    highFreqImgDesc.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;

    highFreqImgDesc.mWidth = gHighFreq3DTextureSize;
    highFreqImgDesc.mHeight = gHighFreq3DTextureSize;
    highFreqImgDesc.mDepth = gHighFreq3DTextureSize;

    highFreqImgDesc.mMipLevels = HighFreqMipCount;
    highFreqImgDesc.mSampleCount = SAMPLE_COUNT_1;
    // highFreqImgDesc.mSrgb = false;

    highFreqImgDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
    highFreqImgDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
    highFreqImgDesc.pName = "HighFrequency3DTexture";

    TextureLoadDesc highFrequency3DTextureDesc = {};
    highFrequency3DTextureDesc.ppTexture = &pHighFrequency3DTexture;
    highFrequency3DTextureDesc.pDesc = &highFreqImgDesc;
    addResource(&highFrequency3DTextureDesc, &token);
    //////////////////////////////////////////////////////////////////////////////////////////////

    TextureLoadDesc curlNoiseTextureDesc = {};
    curlNoiseTextureDesc.pFileName = "VolumetricClouds/CurlNoiseFBM.tex";
    curlNoiseTextureDesc.ppTexture = &pCurlNoiseTexture;
    addResource(&curlNoiseTextureDesc, &token);

    //////////////////////////////////////////////////////////////////////////////////////////////

    TextureLoadDesc weathereTextureDesc = {};
    weathereTextureDesc.pFileName = "VolumetricClouds/WeatherMap.tex";
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
      cmdDispatch(cmd, (uint32_t)ceilf((float)pWeatherTexture->mWidth / (float)pThreadGroupSize[0]),
    (uint32_t)ceilf((float)pWeatherTexture->mHeight / (float)pThreadGroupSize[1]), 1);

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

    UIComponentDesc UIComponentDesc = {};
    float           dpiScale[2];
    const uint32_t  monitorIdx = getActiveMonitorIdx();
    getMonitorDpiScale(monitorIdx, dpiScale);
    UIComponentDesc.mStartPosition = vec2(1080.0f / dpiScale[0], 80.0f / dpiScale[1]);
    UIComponentDesc.mStartSize = vec2(300.0f / dpiScale[0], 250.0f / dpiScale[1]);
    uiCreateComponent("Volumetric Cloud", &UIComponentDesc, &pGuiCloudWindow);

    SeparatorWidget separator;

    SliderUintWidget downsampling;
    downsampling.pData = &gAppSettings.DownSampling;
    downsampling.mMin = 1;
    downsampling.mMax = 3;
    downsampling.mStep = 1;
    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "Downsampling", &downsampling, WIDGET_TYPE_SLIDER_UINT));

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

    CheckboxWidget enableBlur;
    enableBlur.pData = &gAppSettings.m_EnableBlur;
    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "Enabled Edge Blur", &enableBlur, WIDGET_TYPE_CHECKBOX));

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

    static const uint32_t maxWidgets = 64;
    UIWidget              widgetBases[maxWidgets] = {};
    UIWidget*             widgets[maxWidgets];
    for (uint32_t i = 0; i < maxWidgets; ++i)
        widgets[i] = &widgetBases[i];

    CollapsingHeaderWidget collapsingRayMarching;
    collapsingRayMarching.pGroupedWidgets = widgets;

    uint32_t widgetsCount = 0;

    SliderUintWidget minSampleCount;
    minSampleCount.pData = &gAppSettings.m_MinSampleCount;
    minSampleCount.mMin = 1;
    minSampleCount.mMax = 256;
    minSampleCount.mStep = 1;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_UINT;
    strcpy(widgets[widgetsCount]->mLabel, "Min Sample Iteration");
    widgets[widgetsCount]->pWidget = &minSampleCount;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderUintWidget maxSampleCount;
    maxSampleCount.pData = &gAppSettings.m_MaxSampleCount;
    maxSampleCount.mMin = 1;
    maxSampleCount.mMax = 1024;
    maxSampleCount.mStep = 1;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_UINT;
    strcpy(widgets[widgetsCount]->mLabel, "Max Sample Iteration");
    widgets[widgetsCount]->pWidget = &maxSampleCount;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget minStepSize;
    minStepSize.pData = &gAppSettings.m_MinStepSize;
    minStepSize.mMin = 1.0f;
    minStepSize.mMax = 2048.0f;
    minStepSize.mStep = 32.0f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Min Step Size");
    widgets[widgetsCount]->pWidget = &minStepSize;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget maxStepSize;
    maxStepSize.pData = &gAppSettings.m_MaxStepSize;
    maxStepSize.mMin = 1.0f;
    maxStepSize.mMax = 4096.0f;
    maxStepSize.mStep = 32.0f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Max Step Size");
    widgets[widgetsCount]->pWidget = &maxStepSize;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget maxSampleDistance;
    maxSampleDistance.pData = &gAppSettings.m_DefaultMaxSampleDistance;
    maxSampleDistance.mMin = 200000.0f;
    maxSampleDistance.mMax = 2000000.0f;
    maxSampleDistance.mStep = 32.0f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Cutt off distance");
    widgets[widgetsCount]->pWidget = &maxSampleDistance;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    CheckboxWidget enabledTemporalRayOffset;
    enabledTemporalRayOffset.pData = &gAppSettings.m_EnabledTemporalRayOffset;
    widgets[widgetsCount]->mType = WIDGET_TYPE_CHECKBOX;
    strcpy(widgets[widgetsCount]->mLabel, "Enabled Temporal RayOffset");
    widgets[widgetsCount]->pWidget = &enabledTemporalRayOffset;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    collapsingRayMarching.mWidgetsCount = widgetsCount;

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "Ray Marching", &collapsingRayMarching, WIDGET_TYPE_COLLAPSING_HEADER));

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

    // cleanup all widgets
    for (uint32_t i = 0; i < widgetsCount; ++i)
        *widgets[i] = UIWidget{};
    widgetsCount = 0;

    CollapsingHeaderWidget collapsingSkyDome;
    collapsingSkyDome.pGroupedWidgets = widgets;

    SliderFloatWidget cloudsLayerStart;
    cloudsLayerStart.pData = &gAppSettings.m_CloudsLayerStart;
    cloudsLayerStart.mMin = 0.0f;
    cloudsLayerStart.mMax = 100000.0f;
    cloudsLayerStart.mStep = 100.0f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Cloud layer start");
    widgets[widgetsCount]->pWidget = &cloudsLayerStart;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget layerThickness;
    layerThickness.pData = &gAppSettings.m_LayerThickness;
    layerThickness.mMin = 1.0f;
    layerThickness.mMax = 200000.0f;
    layerThickness.mStep = 100.0f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Layer Thickness");
    widgets[widgetsCount]->pWidget = &layerThickness;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    collapsingSkyDome.mWidgetsCount = widgetsCount;

    CollapsingHeaderWidget collapsingCloud;
    // Note: we use same widgets array for all subheaders
    collapsingCloud.pGroupedWidgets = widgets + widgetsCount;

    SliderFloatWidget baseCloudTiling;
    baseCloudTiling.pData = &gAppSettings.m_BaseTile;
    baseCloudTiling.mMin = 0.001f;
    baseCloudTiling.mMax = 10.0f;
    baseCloudTiling.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Base Cloud Tiling");
    widgets[widgetsCount]->pWidget = &baseCloudTiling;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget detailCloudTiling;
    detailCloudTiling.pData = &gAppSettings.m_DetailTile;
    detailCloudTiling.mMin = 0.001f;
    detailCloudTiling.mMax = 10.0f;
    detailCloudTiling.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Detail Cloud Tiling");
    widgets[widgetsCount]->pWidget = &detailCloudTiling;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget detailStrength;
    detailStrength.pData = &gAppSettings.m_DetailStrength;
    detailStrength.mMin = 0.001f;
    detailStrength.mMax = 2.0f;
    detailStrength.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Detail Strength");
    widgets[widgetsCount]->pWidget = &detailStrength;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget cloudTopOffset;
    cloudTopOffset.pData = &gAppSettings.m_CloudTopOffset;
    cloudTopOffset.mMin = 0.01f;
    cloudTopOffset.mMax = 2000.0f;
    cloudTopOffset.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Cloud Top Offseth");
    widgets[widgetsCount]->pWidget = &cloudTopOffset;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget cloudSize;
    cloudSize.pData = &gAppSettings.m_CloudSize;
    cloudSize.mMin = 0.001f;
    cloudSize.mMax = 1000000.0f;
    cloudSize.mStep = 0.1f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Cloud Size");
    widgets[widgetsCount]->pWidget = &cloudSize;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget cloudDensity;
    cloudDensity.pData = &gAppSettings.m_CloudDensity;
    cloudDensity.mMin = 0.0f;
    cloudDensity.mMax = 10.0f;
    cloudDensity.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Cloud Density");
    widgets[widgetsCount]->pWidget = &cloudDensity;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget cloudCoverageModifier;
    cloudCoverageModifier.pData = &gAppSettings.m_CloudCoverageModifier;
    cloudCoverageModifier.mMin = -1.0f;
    cloudCoverageModifier.mMax = 1.0f;
    cloudCoverageModifier.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Cloud Coverage Modifier");
    widgets[widgetsCount]->pWidget = &cloudCoverageModifier;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget cloudTypeModifier;
    cloudTypeModifier.pData = &gAppSettings.m_CloudTypeModifier;
    cloudTypeModifier.mMin = -1.0f;
    cloudTypeModifier.mMax = 1.0f;
    cloudTypeModifier.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Cloud Type Modifier");
    widgets[widgetsCount]->pWidget = &cloudTypeModifier;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget anvilBias;
    anvilBias.pData = &gAppSettings.m_AnvilBias;
    anvilBias.mMin = 0.0f;
    anvilBias.mMax = 1.0f;
    anvilBias.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Anvil Bias");
    widgets[widgetsCount]->pWidget = &anvilBias;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget contrast;
    contrast.pData = &gAppSettings.m_Contrast;
    contrast.mMin = 0.0f;
    contrast.mMax = 2.0f;
    contrast.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Contrast");
    widgets[widgetsCount]->pWidget = &contrast;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget precipitation;
    precipitation.pData = &gAppSettings.m_Precipitation;
    precipitation.mMin = 0.0f;
    precipitation.mMax = 10.0f;
    precipitation.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Precipitation");
    widgets[widgetsCount]->pWidget = &precipitation;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);
    collapsingCloud.mWidgetsCount = (uint32_t)((widgets + widgetsCount) - collapsingCloud.pGroupedWidgets);

    CollapsingHeaderWidget collapsingCloudCurl;
    collapsingCloudCurl.pGroupedWidgets = widgets + widgetsCount;

    SliderFloatWidget curlTiling;
    curlTiling.pData = &gAppSettings.m_CurlTile;
    curlTiling.mMin = 0.001f;
    curlTiling.mMax = 4.0f;
    curlTiling.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Curl Tiling");
    widgets[widgetsCount]->pWidget = &curlTiling;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget curlStrength;
    curlStrength.pData = &gAppSettings.m_CurlStrength;
    curlStrength.mMin = 0.0f;
    curlStrength.mMax = 10000.0f;
    curlStrength.mStep = 0.1f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Curl Strength");
    widgets[widgetsCount]->pWidget = &curlStrength;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    collapsingCloudCurl.mWidgetsCount = (uint32_t)((widgets + widgetsCount) - collapsingCloudCurl.pGroupedWidgets);

    CollapsingHeaderWidget collapsingCloudWeather;
    collapsingCloudWeather.pGroupedWidgets = widgets + widgetsCount;

    SliderFloatWidget weatherTextSize;
    weatherTextSize.pData = &gAppSettings.m_WeatherTexSize;
    weatherTextSize.mMin = 100000.0f;
    weatherTextSize.mMax = 10000000.0f;
    weatherTextSize.mStep = 0.1f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Weather Texture Size");
    widgets[widgetsCount]->pWidget = &weatherTextSize;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget weatherTextDirection;
    weatherTextDirection.pData = &gAppSettings.WeatherTextureAzimuth;
    weatherTextDirection.mMin = -180.0f;
    weatherTextDirection.mMax = 180.0f;
    weatherTextDirection.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Weather Texture Direction");
    widgets[widgetsCount]->pWidget = &weatherTextDirection;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget weatherTextDistance;
    weatherTextDistance.pData = &gAppSettings.WeatherTextureDistance;
    weatherTextDistance.mMin = -300000.0f;
    weatherTextDistance.mMax = 300000.0f;
    weatherTextDistance.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Weather Texture Distance");
    widgets[widgetsCount]->pWidget = &weatherTextDistance;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    collapsingCloudWeather.mWidgetsCount = (uint32_t)((widgets + widgetsCount) - collapsingCloudWeather.pGroupedWidgets);

    CollapsingHeaderWidget collapsingWind;
    collapsingWind.pGroupedWidgets = widgets + widgetsCount;

    SliderFloatWidget windDirection;
    windDirection.pData = &gAppSettings.m_WindAzimuth;
    windDirection.mMin = -180.0f;
    windDirection.mMax = 180.0f;
    windDirection.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Wind Direction");
    widgets[widgetsCount]->pWidget = &windDirection;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget windIntensity;
    windIntensity.pData = &gAppSettings.m_WindIntensity;
    windIntensity.mMin = 0.0f;
    windIntensity.mMax = 100.0f;
    windIntensity.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Wind Intensity");
    widgets[widgetsCount]->pWidget = &windIntensity;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    CheckboxWidget enabledRotation;
    enabledRotation.pData = &gAppSettings.m_EnabledRotation;
    widgets[widgetsCount]->mType = WIDGET_TYPE_CHECKBOX;
    strcpy(widgets[widgetsCount]->mLabel, "Enabled Rotation");
    widgets[widgetsCount]->pWidget = &enabledRotation;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget rotationPivotAzimuth;
    rotationPivotAzimuth.pData = &gAppSettings.m_RotationPivotAzimuth;
    rotationPivotAzimuth.mMin = -180.0f;
    rotationPivotAzimuth.mMax = 180.0f;
    rotationPivotAzimuth.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Rotation Pivot Azimuth");
    widgets[widgetsCount]->pWidget = &rotationPivotAzimuth;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget rotationPivotDistance;
    rotationPivotDistance.pData = &gAppSettings.m_RotationPivotDistance;
    rotationPivotDistance.mMin = -10.0f;
    rotationPivotDistance.mMax = 10.0f;
    rotationPivotDistance.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Rotation Pivot Distance");
    widgets[widgetsCount]->pWidget = &rotationPivotDistance;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget rotationIntensity;
    rotationIntensity.pData = &gAppSettings.m_RotationIntensity;
    rotationIntensity.mMin = -20.0f;
    rotationIntensity.mMax = 20.0f;
    rotationIntensity.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Rotation Intensity");
    widgets[widgetsCount]->pWidget = &rotationIntensity;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget risingVaporScale;
    risingVaporScale.pData = &gAppSettings.m_RisingVaporScale;
    risingVaporScale.mMin = 0.001f;
    risingVaporScale.mMax = 1.0f;
    risingVaporScale.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "RisingVapor Scale");
    widgets[widgetsCount]->pWidget = &risingVaporScale;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget risingVaporUpDirection;
    risingVaporUpDirection.pData = &gAppSettings.m_RisingVaporUpDirection;
    risingVaporUpDirection.mMin = -1.0f;
    risingVaporUpDirection.mMax = 1.0f;
    risingVaporUpDirection.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "RisingVapor UpDirection");
    widgets[widgetsCount]->pWidget = &risingVaporUpDirection;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget risingVaporIntensity;
    risingVaporIntensity.pData = &gAppSettings.m_RisingVaporIntensity;
    risingVaporIntensity.mMin = 0.0f;
    risingVaporIntensity.mMax = 10.0f;
    risingVaporIntensity.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "RisingVapor Intensity");
    widgets[widgetsCount]->pWidget = &risingVaporIntensity;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget noiseFlowAzimuth;
    noiseFlowAzimuth.pData = &gAppSettings.m_NoiseFlowAzimuth;
    noiseFlowAzimuth.mMin = -180.0f;
    noiseFlowAzimuth.mMax = 180.0f;
    noiseFlowAzimuth.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Noise Flow Azimuth");
    widgets[widgetsCount]->pWidget = &noiseFlowAzimuth;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget noiseFlowIntensity;
    noiseFlowIntensity.pData = &gAppSettings.m_NoiseFlowIntensity;
    noiseFlowIntensity.mMin = 0.0f;
    noiseFlowIntensity.mMax = 100.0f;
    noiseFlowIntensity.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Noise Flow Intensity");
    widgets[widgetsCount]->pWidget = &noiseFlowIntensity;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    collapsingWind.mWidgetsCount = (uint32_t)((widgets + widgetsCount) - collapsingWind.pGroupedWidgets);

    UIWidget  vcWidgetBases[5] = {};
    UIWidget* vcWidgets[5];
    for (int i = 0; i < 5; ++i)
    {
        vcWidgets[i] = &vcWidgetBases[i];
        vcWidgets[i]->mType = WIDGET_TYPE_COLLAPSING_HEADER;
    }

    CollapsingHeaderWidget collapsingVC;
    collapsingVC.pGroupedWidgets = vcWidgets;
    collapsingVC.mWidgetsCount = 5;

    strcpy(vcWidgets[0]->mLabel, "Sky Dome");
    vcWidgets[0]->pWidget = &collapsingSkyDome;
    strcpy(vcWidgets[1]->mLabel, "Cloud Modeling");
    vcWidgets[1]->pWidget = &collapsingCloud;
    strcpy(vcWidgets[2]->mLabel, "Cloud Curl");
    vcWidgets[2]->pWidget = &collapsingCloudCurl;
    strcpy(vcWidgets[3]->mLabel, "Weather");
    vcWidgets[3]->pWidget = &collapsingCloudWeather;
    strcpy(vcWidgets[4]->mLabel, "Wind");
    vcWidgets[4]->pWidget = &collapsingWind;

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "Volumetric Clouds", &collapsingVC, WIDGET_TYPE_COLLAPSING_HEADER));

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

    CheckboxWidget enabled2ndLayer;
    enabled2ndLayer.pData = &gAppSettings.m_Enabled2ndLayer;
    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "Enabled 2nd Layer", &enabled2ndLayer, WIDGET_TYPE_CHECKBOX));

    // SECOND LAYER VOLUMETRIC CLOUDS

    // Note: all widgets are the same, so we only change data pointers

    // Skydome
    cloudsLayerStart.pData = &gAppSettings.m_CloudsLayerStart_2nd;
    layerThickness.pData = &gAppSettings.m_LayerThickness_2nd;

    // Cloud
    baseCloudTiling.pData = &gAppSettings.m_BaseTile_2nd;
    detailCloudTiling.pData = &gAppSettings.m_DetailTile_2nd;
    detailStrength.pData = &gAppSettings.m_DetailStrength_2nd;
    cloudTopOffset.pData = &gAppSettings.m_CloudTopOffset_2nd;
    cloudSize.pData = &gAppSettings.m_CloudSize_2nd;
    cloudDensity.pData = &gAppSettings.m_CloudDensity_2nd;
    cloudCoverageModifier.pData = &gAppSettings.m_CloudCoverageModifier_2nd;
    cloudTypeModifier.pData = &gAppSettings.m_CloudTypeModifier_2nd;
    anvilBias.pData = &gAppSettings.m_AnvilBias_2nd;
    contrast.pData = &gAppSettings.m_Contrast_2nd;
    precipitation.pData = &gAppSettings.m_Precipitation_2nd;

    // Cloud Curl
    curlTiling.pData = &gAppSettings.m_CurlTile_2nd;
    curlStrength.pData = &gAppSettings.m_CurlStrength_2nd;

    // Cloud Weather
    weatherTextSize.pData = &gAppSettings.m_WeatherTexSize_2nd;
    weatherTextDirection.pData = &gAppSettings.WeatherTextureAzimuth_2nd;
    weatherTextDistance.pData = &gAppSettings.WeatherTextureDistance_2nd;

    // Wind
    windDirection.pData = &gAppSettings.m_WindAzimuth_2nd;
    windIntensity.pData = &gAppSettings.m_WindIntensity_2nd;
    enabledRotation.pData = &gAppSettings.m_EnabledRotation_2nd;
    rotationPivotAzimuth.pData = &gAppSettings.m_RotationPivotAzimuth_2nd;
    rotationPivotDistance.pData = &gAppSettings.m_RotationPivotDistance_2nd;
    rotationIntensity.pData = &gAppSettings.m_RotationIntensity_2nd;
    risingVaporScale.pData = &gAppSettings.m_RisingVaporScale_2nd;
    risingVaporUpDirection.pData = &gAppSettings.m_RisingVaporUpDirection_2nd;
    risingVaporIntensity.pData = &gAppSettings.m_RisingVaporIntensity_2nd;
    noiseFlowAzimuth.pData = &gAppSettings.m_NoiseFlowAzimuth_2nd;
    noiseFlowIntensity.pData = &gAppSettings.m_NoiseFlowIntensity_2nd;

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "Volumetric Clouds 2", &collapsingVC, WIDGET_TYPE_COLLAPSING_HEADER));

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

    // cleanup all widgets
    for (uint32_t i = 0; i < widgetsCount; ++i)
        *widgets[i] = UIWidget{};
    widgetsCount = 0;

    CollapsingHeaderWidget collapsingCloudLighting;
    collapsingCloudLighting.pGroupedWidgets = widgets;

    ColorPickerWidget customColor;
    customColor.pData = &gAppSettings.m_CustomColor;
    widgets[widgetsCount]->mType = WIDGET_TYPE_COLOR_PICKER;
    strcpy(widgets[widgetsCount]->mLabel, "Custom Color");
    widgets[widgetsCount]->pWidget = &customColor;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget customColorIntensity;
    customColorIntensity.pData = &gAppSettings.m_CustomColorIntensity;
    customColorIntensity.mMin = 0.0f;
    customColorIntensity.mMax = 1.0f;
    customColorIntensity.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Custom Color Intensity");
    widgets[widgetsCount]->pWidget = &customColorIntensity;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget customColorBlend;
    customColorBlend.pData = &gAppSettings.m_CustomColorBlendFactor;
    customColorBlend.mMin = 0.0f;
    customColorBlend.mMax = 1.0f;
    customColorBlend.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Custom Color Blend");
    widgets[widgetsCount]->pWidget = &customColorBlend;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget transStepSize;
    transStepSize.pData = &gAppSettings.m_TransStepSize;
    transStepSize.mMin = 0.0f;
    transStepSize.mMax = 2048.0f;
    transStepSize.mStep = 1.0f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Trans Step Size");
    widgets[widgetsCount]->pWidget = &transStepSize;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget backgroundBlend;
    backgroundBlend.pData = &gAppSettings.m_BackgroundBlendFactor;
    backgroundBlend.mMin = 0.0f;
    backgroundBlend.mMax = 1.0f;
    backgroundBlend.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "BackgroundBlendFactor");
    widgets[widgetsCount]->pWidget = &backgroundBlend;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget silverIntensity;
    silverIntensity.pData = &gAppSettings.m_SilverIntensity;
    silverIntensity.mMin = 0.0f;
    silverIntensity.mMax = 2.0f;
    silverIntensity.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Silver Intensity");
    widgets[widgetsCount]->pWidget = &silverIntensity;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget silverSpread;
    silverSpread.pData = &gAppSettings.m_SilverSpread;
    silverSpread.mMin = 0.0f;
    silverSpread.mMax = 0.99f;
    silverSpread.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Silver Spread");
    widgets[widgetsCount]->pWidget = &silverSpread;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget eccentricity;
    eccentricity.pData = &gAppSettings.m_Eccentricity;
    eccentricity.mMin = 0.0f;
    eccentricity.mMax = 1.0f;
    eccentricity.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Eccentricity");
    widgets[widgetsCount]->pWidget = &eccentricity;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget cloudBrightness;
    cloudBrightness.pData = &gAppSettings.m_CloudBrightness;
    cloudBrightness.mMin = 0.0f;
    cloudBrightness.mMax = 2.0f;
    cloudBrightness.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Cloud Brightness");
    widgets[widgetsCount]->pWidget = &cloudBrightness;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    collapsingCloudLighting.mWidgetsCount = widgetsCount;

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "Cloud Lighting", &collapsingCloudLighting, WIDGET_TYPE_COLLAPSING_HEADER));

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

    // cleanup all widgets
    for (uint32_t i = 0; i < widgetsCount; ++i)
        *widgets[i] = UIWidget{};
    widgetsCount = 0;

    CollapsingHeaderWidget collapsingCulling;
    collapsingCulling.pGroupedWidgets = widgets;

    CheckboxWidget enabledDepthCulling;
    enabledDepthCulling.pData = &gAppSettings.m_EnabledDepthCulling;
    widgets[widgetsCount]->mType = WIDGET_TYPE_CHECKBOX;
    strcpy(widgets[widgetsCount]->mLabel, "Enabled Depth Culling");
    widgets[widgetsCount]->pWidget = &enabledDepthCulling;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    collapsingCulling.mWidgetsCount = widgetsCount;

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "Depth Culling", &collapsingCulling, WIDGET_TYPE_COLLAPSING_HEADER));

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

    // cleanup all widgets
    for (uint32_t i = 0; i < widgetsCount; ++i)
        *widgets[i] = UIWidget{};
    widgetsCount = 0;

    CollapsingHeaderWidget collapsingShadow;
    collapsingShadow.pGroupedWidgets = widgets;

    CheckboxWidget enabledShadow;
    enabledShadow.pData = &gAppSettings.m_EnabledShadow;
    widgets[widgetsCount]->mType = WIDGET_TYPE_CHECKBOX;
    strcpy(widgets[widgetsCount]->mLabel, "Enabled Shadow");
    widgets[widgetsCount]->pWidget = &enabledShadow;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget shadowIntensity;
    shadowIntensity.pData = &gAppSettings.m_ShadowIntensity;
    shadowIntensity.mMin = 0.0f;
    shadowIntensity.mMax = 1.0f;
    shadowIntensity.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Shadow Intensity");
    widgets[widgetsCount]->pWidget = &shadowIntensity;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    collapsingShadow.mWidgetsCount = widgetsCount;

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "Clouds Shadow", &collapsingShadow, WIDGET_TYPE_COLLAPSING_HEADER));

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

    // cleanup all widgets
    for (uint32_t i = 0; i < widgetsCount; ++i)
        *widgets[i] = UIWidget{};
    widgetsCount = 0;

    CollapsingHeaderWidget collapsingGodray;
    collapsingGodray.pGroupedWidgets = widgets;

    CheckboxWidget enabledGodRay;
    enabledGodRay.pData = &gAppSettings.m_EnabledGodray;
    widgets[widgetsCount]->mType = WIDGET_TYPE_CHECKBOX;
    strcpy(widgets[widgetsCount]->mLabel, "Enabled Godray");
    widgets[widgetsCount]->pWidget = &enabledGodRay;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderUintWidget godNumSamples;
    godNumSamples.pData = &gAppSettings.m_GodNumSamples;
    godNumSamples.mMin = 1;
    godNumSamples.mMax = 256;
    godNumSamples.mStep = 1;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_UINT;
    strcpy(widgets[widgetsCount]->mLabel, "Number of Samples");
    widgets[widgetsCount]->pWidget = &godNumSamples;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget exposure;
    exposure.pData = &gAppSettings.m_Exposure;
    exposure.mMin = 0.0f;
    exposure.mMax = 0.1f;
    exposure.mStep = 0.0001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Exposure");
    widgets[widgetsCount]->pWidget = &exposure;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget decay;
    decay.pData = &gAppSettings.m_Decay;
    decay.mMin = 0.0f;
    decay.mMax = 2.0f;
    decay.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Decay");
    widgets[widgetsCount]->pWidget = &decay;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget density;
    density.pData = &gAppSettings.m_Density;
    density.mMin = 0.0f;
    density.mMax = 2.0f;
    density.mStep = 0.001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Density");
    widgets[widgetsCount]->pWidget = &density;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    SliderFloatWidget weight;
    weight.pData = &gAppSettings.m_Weight;
    weight.mMin = 0.0f;
    weight.mMax = 2.0f;
    weight.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Weight");
    widgets[widgetsCount]->pWidget = &weight;
    ++widgetsCount;
    ASSERT(widgetsCount < maxWidgets);

    collapsingGodray.mWidgetsCount = widgetsCount;

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "Clouds Godray", &collapsingGodray, WIDGET_TYPE_COLLAPSING_HEADER));

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

    // cleanup all widgets
    for (uint32_t i = 0; i < widgetsCount; ++i)
        *widgets[i] = UIWidget{};
    widgetsCount = 0;

    CollapsingHeaderWidget collapsingTest;
    collapsingTest.pGroupedWidgets = widgets;

    SliderFloatWidget test00;
    test00.pData = &gAppSettings.m_Test00;
    test00.mMin = -1.0f;
    test00.mMax = 4.0f;
    test00.mStep = 0.1f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Test00");
    widgets[widgetsCount]->pWidget = &test00;
    ++widgetsCount;

    SliderFloatWidget test01;
    test01.pData = &gAppSettings.m_Test01;
    test01.mMin = 0.0f;
    test01.mMax = 0.4f;
    test01.mStep = 0.0001f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Test01");
    widgets[widgetsCount]->pWidget = &test01;
    ++widgetsCount;

    SliderFloatWidget test02;
    test02.pData = &gAppSettings.m_Test02;
    test02.mMin = 0.0f;
    test02.mMax = 1.0f;
    test02.mStep = 0.01f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Test02");
    widgets[widgetsCount]->pWidget = &test02;
    ++widgetsCount;

    SliderFloatWidget test03;
    test03.pData = &gAppSettings.m_Test03;
    test03.mMin = 0.0f;
    test03.mMax = 100.0f;
    test03.mStep = 1.0f;
    widgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
    strcpy(widgets[widgetsCount]->mLabel, "Test03");
    widgets[widgetsCount]->pWidget = &test03;
    ++widgetsCount;

    collapsingTest.mWidgetsCount = widgetsCount;

    luaRegisterWidget(uiCreateComponentWidget(pGuiCloudWindow, "Clouds Test", &collapsingTest, WIDGET_TYPE_COLLAPSING_HEADER));

    ///////////////////////////

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
    RemoveUniformBuffers();

    removeResource(pHighFrequency3DTexture);
    removeResource(pLowFrequency3DTexture);

    removeResource(pWeatherTexture);
    removeResource(pWeatherCompactTexture);
    removeResource(pCurlNoiseTexture);

    // Remove Textures
    for (uint32_t i = 0; i < gHighFreq3DTextureSize; ++i)
    {
        removeResource(gHighFrequencyOriginTextureStorage[i]);
    }

    // Remove Textures
    for (uint32_t i = 0; i < gLowFreq3DTextureSize; ++i)
    {
        removeResource(gLowFrequencyOriginTextureStorage[i]);
    }

    tf_free(gHighFrequencyOriginTextureStorage);
    tf_free(gLowFrequencyOriginTextureStorage);

    removeResource(pTriangularScreenVertexBuffer);

    removeSampler(pRenderer, pPointSampler);
    removeSampler(pRenderer, pBilinearSampler);
    removeSampler(pRenderer, pPointClampSampler);
    removeSampler(pRenderer, pBiClampSampler);
    removeSampler(pRenderer, pLinearBorderSampler);
    removeSampler(pRenderer, pHiZDepthClampSampler);
}

Texture* VolumetricClouds::GetWeatherMap() { return pWeatherTexture; };

bool VolumetricClouds::Load(RenderTarget** rts, uint32_t count)
{
    UNREF_PARAM(rts);
    UNREF_PARAM(count);
    return false;
}

bool VolumetricClouds::Load(uint32_t width, uint32_t height)
{
    gDownsampledCloudSize = (uint32_t)pow(2, (double)gAppSettings.DownSampling);

    mWidth = width;
    mHeight = height;

    float aspect = (float)mWidth / (float)mHeight;
    float aspectInverse = 1.0f / aspect;
    float horizontal_fov = PI / 3.0f;
    float vertical_fov = 2.0f * atan(tan(horizontal_fov * 0.5f) * aspectInverse);
    // projMat = mat4::perspectiveLH(horizontal_fov, aspectInverse, _CLOUDS_LAYER_START * 0.01, _CLOUDS_LAYER_END * 10.0);
    projMat = mat4::perspectiveLH(horizontal_fov, aspectInverse, CAMERA_NEAR, CAMERA_FAR);

    addVolumetricCloudsSaveTextures();

    if (!AddHiZDepthBuffer())
        return false;

    g_ProjectionExtents = GetProjectionExtents(vertical_fov, aspect, (float)((mWidth / gDownsampledCloudSize) & (~31)),
                                               (float)((mHeight / gDownsampledCloudSize) & (~31)), 0.0f, 0.0f);

    float screenMiscPoints[] = {
        g_ProjectionExtents.getX(), g_ProjectionExtents.getY(), g_ProjectionExtents.getZ(), g_ProjectionExtents.getW(),
        g_ProjectionExtents.getX(), g_ProjectionExtents.getY(), g_ProjectionExtents.getZ(), g_ProjectionExtents.getW(),
        g_ProjectionExtents.getX(), g_ProjectionExtents.getY(), g_ProjectionExtents.getZ(), g_ProjectionExtents.getW(),
    };

    SyncToken token = {};

    uint64_t       screenDataSize = 4 * 3 * sizeof(float);
    BufferLoadDesc screenMiscVbDesc = {};
    screenMiscVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    screenMiscVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    screenMiscVbDesc.mDesc.mSize = screenDataSize;
    screenMiscVbDesc.pData = screenMiscPoints;
    screenMiscVbDesc.ppBuffer = &pTriangularScreenVertexWithMiscBuffer;
    addResource(&screenMiscVbDesc, &token);

    volumetricCloudsCB = VolumetricCloudsCB();
    prevCameraPos = pCameraController->getViewPosition();
    prevViewWithoutTranslation = pCameraController->getViewMatrix();
    prevViewWithoutTranslation.setTranslation(vec3(0.0f, 0.0f, 0.0f));

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    waitForToken(&token);

    return true;
}

void VolumetricClouds::Unload()
{
    removeResource(pTriangularScreenVertexWithMiscBuffer);
    removeResource(pHBlurTex);
    removeResource(pVBlurTex);

    removeResource(pHiZDepthBuffer);
    removeResource(pHiZDepthBuffer2);
    removeResource(pHiZDepthBufferX);
    removeResource(pHighResCloudTexture);

    removeResource(plowResCloudTexture);
    removeResource(pReprojectedCloudTexture);
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
        BufferBarrier bufferBarriers[] = { { pTransmittanceBuffer, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE } };
        cmdResourceBarrier(cmd, TF_ARRAY_COUNT(bufferBarriers), bufferBarriers, TF_ARRAY_COUNT(barriers000), barriers000, 0, NULL);

        const uint32_t* pThreadGroupSize = NULL;

        {
#if USE_DEPTH_CULLING

#if USE_LOD_DEPTH
            cmdBindPipeline(cmd, pGenHiZMipmapPRPipeline);

            cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Lodded Z DepthBuffer");

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
            cmdDispatch(cmd, (pDepthBuffer->pTexture->mWidth / pThreadGroupSize[0]) + 1,
                        pDepthBuffer->pTexture->mHeight / pThreadGroupSize[1] + 1, 1);

            for (uint i = 0; i < pHiZDepthBuffer->mMipLevels - 1; i++)
            {
                TextureBarrier barriers0[] = { { pHiZDepthBuffer, RESOURCE_STATE_SHADER_RESOURCE },
                                               { pHiZDepthBuffer2, RESOURCE_STATE_UNORDERED_ACCESS } };

                cmdResourceBarrier(cmd, 0, NULL, 2, barriers0);

                // Gen mipmap
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

                TextureBarrier barriers1[] = { { pHiZDepthBuffer2, RESOURCE_STATE_SHADER_RESOURCE },
                                               { pHiZDepthBuffer, RESOURCE_STATE_UNORDERED_ACCESS } };

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

        if (gAppSettings.TemporalFilteringEnabled)
        {
            {
#if USE_LOD_DEPTH
                Texture* HiZedDepthTexture = pHiZDepthBufferX;
#else
                Texture* HiZedDepthTexture = pHiZDepthBuffer;
#endif

#if USE_VC_FRAGMENTSHADER
                cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Clouds (Fragment Shaders Pipeline)");

                TextureBarrier barriers0[] = { { HiZedDepthTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE } };

                RenderTarget*       pRenderTarget = plowResCloudRT;
                RenderTargetBarrier rtBarriers0[] = {
                    { pRenderTarget, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
                };
                cmdResourceBarrier(cmd, 0, NULL, 1, barriers0, 1, rtBarriers0);

                BindRenderTargetsDesc bindRenderTargets = {};
                bindRenderTargets.mRenderTargetCount = 1;
                bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
                cmdBindRenderTargets(cmd, &bindRenderTargets);
                cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
                cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

                if (gAppSettings.m_EnabledDepthCulling)
                {
                    if (gAppSettings.m_Enabled2ndLayer)
                    {
                        cmdBindPipeline(cmd, pVolumetricCloud2ndWithDepthPipeline);
                    }
                    else
                    {
                        cmdBindPipeline(cmd, pVolumetricCloudWithDepthPipeline);
                    }

                    cmdBindDescriptorSet(cmd, 6, pVolumetricCloudsDescriptorSetGraphics[0]);
                }
                else
                {
                    if (gAppSettings.m_Enabled2ndLayer)
                    {
                        cmdBindPipeline(cmd, pVolumetricCloud2ndPipeline);
                    }
                    else
                    {
                        cmdBindPipeline(cmd, pVolumetricCloudPipeline);
                    }

                    cmdBindDescriptorSet(cmd, 7, pVolumetricCloudsDescriptorSetGraphics[0]);
                }

                cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);

                const uint32_t miscStride = sizeof(float) * 4;
                cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexWithMiscBuffer, &miscStride, NULL);
                cmdDraw(cmd, 3, 0);

                cmdBindRenderTargets(cmd, NULL);
#else
                cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Clouds (Compute Shaders Pipeline)");

                TextureBarrier barriers0[] = {
                    { HiZedDepthTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
                    { plowResCloudTexture, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS },
                };

                cmdResourceBarrier(cmd, 0, NULL, 2, barriers0, 0, NULL);

                if (gAppSettings.m_EnabledDepthCulling)
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

                    cmdBindDescriptorSet(cmd, 4, pVolumetricCloudsDescriptorSetCompute[0]);
                }
                else
                {
                    if (gAppSettings.m_Enabled2ndLayer)
                    {
                        cmdBindPipeline(cmd, pVolumetricCloud2ndCompPipeline);
                        pThreadGroupSize = pVolumetricCloud2ndCompShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
                    }
                    else
                    {
                        cmdBindPipeline(cmd, pVolumetricCloudCompPipeline);
                        pThreadGroupSize = pVolumetricCloudCompShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
                    }

                    cmdBindDescriptorSet(cmd, 5, pVolumetricCloudsDescriptorSetCompute[0]);
                }

                cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetCompute[1]);

                cmdDispatch(cmd, (uint32_t)ceil((float)lowResTextureDesc.mWidth / (float)pThreadGroupSize[0]),
                            (uint32_t)ceil((float)lowResTextureDesc.mHeight / (float)pThreadGroupSize[1]), 1);
#endif
                cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
            }

            {
                cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Reprojection");

                /////////////////////////////////////////////////////////////////////////////
                RenderTargetBarrier rtBarriersForReprojection[] = {
                    { pReprojectedCloudRT, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
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

                RenderTarget* pRenderTarget = pReprojectedCloudRT;

                BindRenderTargetsDesc bindRenderTargets = {};
                bindRenderTargets.mRenderTargetCount = 1;
                bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
                cmdBindRenderTargets(cmd, &bindRenderTargets);
                cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
                cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

                cmdBindPipeline(cmd, pReprojectionPipeline);
                cmdBindDescriptorSet(cmd, 0, pVolumetricCloudsDescriptorSetGraphics[0]);
                cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);

                const uint32_t miscStride = sizeof(float) * 4;
                cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexWithMiscBuffer, &miscStride, NULL);
                cmdDraw(cmd, 3, 0);
                cmdBindRenderTargets(cmd, NULL);

                // Need to reset g_PrevFrameTexture descriptor slot before we can use it as UAV in the compute root signature
                cmdBindDescriptorSet(cmd, 7, pVolumetricCloudsDescriptorSetGraphics[0]);

                cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
            }

            // cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

            {
                cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Save Current RenderTarget");

                TextureBarrier barriersForSaveRT[] = {
                    { pHighResCloudTexture, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS },
                };
                RenderTargetBarrier rtBarriersForSaveRT[] = {
                    { pReprojectedCloudRT, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE },
                };

                cmdResourceBarrier(cmd, 0, NULL, 1, barriersForSaveRT, 1, rtBarriersForSaveRT);

                cmdBindPipeline(cmd, pCopyRTPipeline);
                cmdBindDescriptorSet(cmd, 1, pVolumetricCloudsDescriptorSetCompute[0]);

                pThreadGroupSize = pCopyRTShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
                cmdDispatch(cmd, (uint32_t)ceil(pReprojectedCloudRT->mWidth / pThreadGroupSize[0]),
                            (uint32_t)ceil(pReprojectedCloudRT->mHeight / pThreadGroupSize[1]), 1);

                TextureBarrier barriersForSaveRTEnd[] = {
                    { pHighResCloudTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
                };

                cmdResourceBarrier(cmd, 0, NULL, 1, barriersForSaveRTEnd, 0, NULL);
                cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
            }
        }
        // ------- Real time pipeline, no temporal filtering
        else
        {
#if USE_LOD_DEPTH
            Texture* HiZedDepthTexture = pHiZDepthBufferX;
#else
            Texture* HiZedDepthTexture = pHiZDepthBuffer;
#endif

#if USE_VC_FRAGMENTSHADER
            cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Clouds Real time (Fragment Shaders Pipeline)");
            RenderTarget* pRenderTarget = phighResCloudRT;

            TextureBarrier      barriers0[] = { { HiZedDepthTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE } };
            RenderTargetBarrier rtBarriers0[] = {
                { pRenderTarget, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
            };
            cmdResourceBarrier(cmd, 0, NULL, 1, barriers0, 1, rtBarriers0);

            // simply record the screen cleaning command

            BindRenderTargetsDesc bindRenderTargets = {};
            bindRenderTargets.mRenderTargetCount = 1;
            bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
            cmdBindRenderTargets(cmd, &bindRenderTargets);
            cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
            cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

            if (gAppSettings.m_EnabledDepthCulling)
            {
                cmdBindPipeline(cmd, pRealTimeVolumetricCloudWithDepthPipeline);
                cmdBindDescriptorSet(cmd, 6, pVolumetricCloudsDescriptorSetGraphics[0]);
            }
            else
            {
                cmdBindPipeline(cmd, pRealTimeVolumetricCloudPipeline);
                cmdBindDescriptorSet(cmd, 7, pVolumetricCloudsDescriptorSetGraphics[0]);
            }

            cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);

            const uint32_t miscStride = sizeof(float) * 4;
            cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexWithMiscBuffer, &miscStride, NULL);
            cmdDraw(cmd, 3, 0);

            cmdBindRenderTargets(cmd, NULL);

            RenderTargetBarrier barriersForRealTimeRenderTarget[] = {
                { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE },
            };

            cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersForRealTimeRenderTarget);
#else
            cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Clouds Real time");

            TextureBarrier barriersForSaveRT[] = { { pHighResCloudTexture, RESOURCE_STATE_SHADER_RESOURCE,
                                                     RESOURCE_STATE_UNORDERED_ACCESS },
                                                   { HiZedDepthTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE } };

            cmdResourceBarrier(cmd, 0, NULL, 2, barriersForSaveRT, 0, NULL);

            if (gAppSettings.m_EnabledDepthCulling)
            {
                cmdBindPipeline(cmd, pRealTimeVolumetricWithDepthCompPipeline);
                pThreadGroupSize = pRealTimeVolumetricCloudWithDepthCompShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
                cmdBindDescriptorSet(cmd, 6, pVolumetricCloudsDescriptorSetCompute[0]);
            }
            else
            {
                cmdBindPipeline(cmd, pRealTimeVolumetricCompPipeline);
                pThreadGroupSize = pRealTimeVolumetricCloudCompShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
                cmdBindDescriptorSet(cmd, 7, pVolumetricCloudsDescriptorSetCompute[0]);
            }

            cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetCompute[1]);
            cmdDispatch(cmd, (uint32_t)ceil(pHighResCloudTexture->mWidth / pThreadGroupSize[0]),
                        (uint32_t)ceil(pHighResCloudTexture->mHeight / pThreadGroupSize[1]), 1);

            TextureBarrier barriersRealtimeCloudEnd[] = {
                { pHighResCloudTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
            };

            cmdResourceBarrier(cmd, 0, NULL, 1, barriersRealtimeCloudEnd, 0, NULL);
#endif
            cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
        }

        if (gAppSettings.m_EnableBlur)
        {
            cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Denoise - Blur");

            TextureBarrier barriersForHBlur[] = {
                { pHBlurTex, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS },
            };
            cmdResourceBarrier(cmd, 0, NULL, 1, barriersForHBlur, 0, NULL);

            cmdBindPipeline(cmd, pHorizontalBlurPipeline);
            cmdBindDescriptorSet(cmd, 2, pVolumetricCloudsDescriptorSetCompute[0]);
            cmdDispatch(cmd, 1, blurTextureDesc.mHeight, 1);

            TextureBarrier barriersForVBlur[] = { { pHBlurTex, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
                                                  { pVBlurTex, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS } };

            cmdResourceBarrier(cmd, 0, NULL, 2, barriersForVBlur, 0, NULL);

            cmdBindPipeline(cmd, pVerticalBlurPipeline);
            cmdBindDescriptorSet(cmd, 3, pVolumetricCloudsDescriptorSetCompute[0]);
            cmdDispatch(cmd, blurTextureDesc.mWidth, 1, 1);

            TextureBarrier barriersForEndBlur[] = { { pVBlurTex, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE } };

            cmdResourceBarrier(cmd, 0, NULL, 1, barriersForEndBlur, 0, NULL);

            cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
        }

        cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
    }

    // PostProcess
    {
        cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "PostProcess");

        RenderTarget* pRenderTarget = pPostProcessRT;

        RenderTargetBarrier barriersForPostProcess[] = { { pRenderTarget, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
                                                         { pSceneColorTexture, RESOURCE_STATE_RENDER_TARGET,
                                                           RESOURCE_STATE_SHADER_RESOURCE } };

        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, barriersForPostProcess);

        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount = 1;
        bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
        cmdBindRenderTargets(cmd, &bindRenderTargets);
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

        cmdBindRenderTargets(cmd, NULL);

        cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
    }
    // PostProcess
    cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

    // Render Godray
    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Render Godray + Composition");

    RenderTargetBarrier barriersForGodray[] = { { pGodrayRT, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET } };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersForGodray);

    if (gAppSettings.m_EnabledGodray)
    {
        cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Render Godray");

        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount = 1;
        bindRenderTargets.mRenderTargets[0] = { pGodrayRT, LOAD_ACTION_CLEAR };
        cmdBindRenderTargets(cmd, &bindRenderTargets);
        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pGodrayRT->mWidth, (float)pGodrayRT->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pGodrayRT->mWidth, pGodrayRT->mHeight);

        cmdBindPipeline(cmd, pGodrayPipeline);
        cmdBindDescriptorSet(cmd, 3, pVolumetricCloudsDescriptorSetGraphics[0]);
        cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
        cmdDraw(cmd, 3, 0);

        cmdBindRenderTargets(cmd, NULL);

        cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
    }

    RenderTargetBarrier barriersForGodrayEnd[] = { { pGodrayRT, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE } };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersForGodrayEnd);

    // Composite
    {
        cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Composite");

        RenderTarget* pRenderTarget = pFinalRT;

        RenderTargetBarrier barriersComposition[] = { { pRenderTarget, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
                                                      { pPostProcessRT, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE } };

        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, barriersComposition);

        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount = 1;
        bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_LOAD };
        cmdBindRenderTargets(cmd, &bindRenderTargets);
        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

        cmdBindPipeline(cmd, pCompositePipeline);
        cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
        cmdBindDescriptorSet(cmd, 5, pVolumetricCloudsDescriptorSetGraphics[0]);
        cmdBindVertexBuffer(cmd, 1, &pTriangularScreenVertexBuffer, &gTriangleVbStride, NULL);
        cmdDraw(cmd, 3, 0);

        cmdBindRenderTargets(cmd, NULL);

        RenderTargetBarrier barriersCompositionEnd[] = { { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE } };

        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersCompositionEnd);

        cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
    }

    // Add Godray
    if (gAppSettings.m_EnabledGodray)
    {
        cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Add Godray");

        RenderTarget* pRenderTarget = pFinalRT;

        RenderTargetBarrier barriersAddGodray[] = {
            { pRenderTarget, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
            { pDepthTexture, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_DEPTH_WRITE },
        };
        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, barriersAddGodray);
        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount = 1;
        bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_LOAD };
        bindRenderTargets.mDepthStencil = { pDepthTexture, LOAD_ACTION_LOAD, LOAD_ACTION_LOAD, STORE_ACTION_NONE, STORE_ACTION_NONE };
        cmdBindRenderTargets(cmd, &bindRenderTargets);
        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

        cmdBindPipeline(cmd, pGodrayAddPipeline);
        cmdBindDescriptorSet(cmd, gFrameIndex, pVolumetricCloudsDescriptorSetGraphics[1]);
        cmdBindDescriptorSet(cmd, 4, pVolumetricCloudsDescriptorSetGraphics[0]);

        cmdDraw(cmd, 3, 0);

        cmdBindRenderTargets(cmd, NULL);

        RenderTargetBarrier barriersAddGodrayEnd[] = {
            { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE },
            { pDepthTexture, RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_SHADER_RESOURCE },
        };

        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, barriersAddGodrayEnd);

        cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
    }
    // Add Godray
    cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

    gAppSettings.m_FirstFrame = false;
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
    volumetricCloudsCB.TimeAndScreenSize = vec4(g_currentTime, g_currentTime, (float)((mWidth / gDownsampledCloudSize) & (~31)),
                                                (float)((mHeight / gDownsampledCloudSize) & (~31)));
    volumetricCloudsCB.CameraNear = CAMERA_NEAR;
    volumetricCloudsCB.CameraFar = CAMERA_FAR;

    mat4 cloudViewMat_1st = pCameraController->getViewMatrix();
    mat4 cloudCurrentToPreviousRelativeViewMat_1st = prevViewWithoutTranslation;
    vec4 deltaPrevEye = prevViewWithoutTranslation * vec4(pCameraController->getViewPosition() - prevCameraPos, 1.0f);
    cloudCurrentToPreviousRelativeViewMat_1st.setTranslation(deltaPrevEye.getXYZ());

    volumetricCloudsCB.m_DataPerEye[0].m_WorldToProjMat = projMat * cloudViewMat_1st;
    volumetricCloudsCB.m_DataPerEye[0].m_RelativeToEyetoPreviousProj = projMat * cloudCurrentToPreviousRelativeViewMat_1st;
    volumetricCloudsCB.m_DataPerEye[0].m_ViewToWorldMat = inverse(cloudViewMat_1st);
    volumetricCloudsCB.m_DataPerEye[0].m_LightToProjMat = mat4::identity();

    mat4 viewMatWithoutTranslation = pCameraController->getViewMatrix();
    viewMatWithoutTranslation.setTranslation(vec3(0.0f, 0.0f, 0.0f));
    mat4 invViewMatWithoutTranslation = inverse(viewMatWithoutTranslation);
    volumetricCloudsCB.m_DataPerEye[0].m_ProjToRelativeToEye = invViewMatWithoutTranslation * inverse(projMat);

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

    volumetricCloudsCB.EarthRadius = 6360000.0f;
    volumetricCloudsCB.EarthCenter = vec4(0.0f, -volumetricCloudsCB.EarthRadius, 0.0f, 0.0f);
    volumetricCloudsCB.m_DataPerLayer[0].CloudsLayerStart = gAppSettings.m_CloudsLayerStart;
    volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart =
        volumetricCloudsCB.EarthRadius + volumetricCloudsCB.m_DataPerLayer[0].CloudsLayerStart;
    volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart2 =
        volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart *
        volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart;
    volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd =
        volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart + gAppSettings.m_LayerThickness;
    volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd2 = volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd *
                                                                         volumetricCloudsCB.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd;

    // volumetricCloudsCB.EarthRadius_2nd = gAppSettings.m_EarthRadius_2nd;
    // volumetricCloudsCB.EarthCenter_2nd = vec4(0.0f, -volumetricCloudsCB.EarthRadius_2nd, 0.0f, 0.0f);
    volumetricCloudsCB.m_DataPerLayer[1].CloudsLayerStart = gAppSettings.m_CloudsLayerStart_2nd;
    volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart =
        volumetricCloudsCB.EarthRadius + volumetricCloudsCB.m_DataPerLayer[1].CloudsLayerStart;
    volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart2 =
        volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart *
        volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart;
    volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerEnd =
        volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart + gAppSettings.m_LayerThickness_2nd;
    volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerEnd2 = volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerEnd *
                                                                         volumetricCloudsCB.m_DataPerLayer[1].EarthRadiusAddCloudsLayerEnd;

    volumetricCloudsCB.m_MaxSampleDistance = gAppSettings.m_DefaultMaxSampleDistance;
    volumetricCloudsCB.MIN_ITERATION_COUNT = gAppSettings.m_MinSampleCount;
    volumetricCloudsCB.MAX_ITERATION_COUNT = gAppSettings.m_MaxSampleCount;

    float windIntensity = gAppSettings.m_EnabledRotation ? 0.0f : gAppSettings.m_WindIntensity * (float)deltaTime * 100.0f;
    float flowIntensity = gAppSettings.m_NoiseFlowIntensity * (float)deltaTime * 100.0f;

    g_StandardPosition +=
        vec4(windXZ.getX() * windIntensity, windXZ.getY() * windIntensity, flowXZ.getX() * flowIntensity, flowXZ.getY() * flowIntensity);

    volumetricCloudsCB.m_DataPerLayer[0].StandardPosition = g_StandardPosition;

    float windIntensity_2nd = gAppSettings.m_EnabledRotation_2nd ? 0.0f : gAppSettings.m_WindIntensity_2nd * (float)deltaTime * 100.0f;
    float flowIntensity_2nd = gAppSettings.m_NoiseFlowIntensity_2nd * (float)deltaTime * 100.0f;

    g_StandardPosition_2nd += vec4(windXZ_2nd.getX() * windIntensity_2nd, windXZ_2nd.getY() * windIntensity_2nd,
                                   flowXZ_2nd.getX() * flowIntensity_2nd, flowXZ_2nd.getY() * flowIntensity_2nd);

    volumetricCloudsCB.m_DataPerLayer[1].StandardPosition = g_StandardPosition_2nd;

    vec4 lightDir = vec4(f3Tov3(LightDirection));

    volumetricCloudsCB.Test00 = lightDir.getY() < 0.0f ? 0.0f : 1.0f;

    lightDir = lightDir.getY() < 0.0f ? -lightDir : lightDir;
    lightDir.setW(gAppSettings.m_TransStepSize);

    volumetricCloudsCB.lightDirection = lightDir;

    volumetricCloudsCB.lightColorAndIntensity =
        lerp(gAppSettings.m_CustomColorBlendFactor, gAppSettings.SunColorAndIntensity.toVec4(), gAppSettings.m_CustomColor.toVec4());

    volumetricCloudsCB.m_DataPerEye[0].cameraPosition = vec4(pCameraController->getViewPosition());
    volumetricCloudsCB.m_DataPerEye[0].cameraPosition.setW(1.0f);
    volumetricCloudsCB.m_DataPerEye[1].cameraPosition = vec4(pCameraController->getViewPosition());
    volumetricCloudsCB.m_DataPerEye[1].cameraPosition.setW(1.0f);

    volumetricCloudsCB.m_DataPerLayer[0].LayerThickness = gAppSettings.m_LayerThickness;
    volumetricCloudsCB.m_DataPerLayer[1].LayerThickness = gAppSettings.m_LayerThickness_2nd;

    volumetricCloudsCB.m_HiZDepthMapWidth = HiZDepthDesc.mWidth;
    volumetricCloudsCB.m_HiZDepthMapHeight = HiZDepthDesc.mHeight;

    volumetricCloudsCB.m_CorrectU = (float)(volumetricCloudsCB.m_JitterX) / (float)((mWidth / gDownsampledCloudSize) & (~31));
    volumetricCloudsCB.m_CorrectV = (float)(volumetricCloudsCB.m_JitterY) / (float)((mHeight / gDownsampledCloudSize) & (~31));

    volumetricCloudsCB.m_UseRandomSeed = gAppSettings.m_EnabledTemporalRayOffset ? 1.0f : 0.0f;
    volumetricCloudsCB.m_StepSize = vec4(gAppSettings.m_MinStepSize, gAppSettings.m_MaxStepSize, 0.0f, 0.0f);

    // Cloud
    volumetricCloudsCB.m_DataPerLayer[0].CloudDensity = gAppSettings.m_CloudDensity;
    volumetricCloudsCB.m_DataPerLayer[1].CloudDensity = gAppSettings.m_CloudDensity_2nd;

    volumetricCloudsCB.m_DataPerLayer[0].CloudCoverage =
        gAppSettings.m_CloudCoverageModifier * gAppSettings.m_CloudCoverageModifier * gAppSettings.m_CloudCoverageModifier;
    volumetricCloudsCB.m_DataPerLayer[1].CloudCoverage =
        gAppSettings.m_CloudCoverageModifier_2nd * gAppSettings.m_CloudCoverageModifier_2nd * gAppSettings.m_CloudCoverageModifier_2nd;

    volumetricCloudsCB.m_DataPerLayer[0].CloudType =
        gAppSettings.m_CloudTypeModifier * gAppSettings.m_CloudTypeModifier * gAppSettings.m_CloudTypeModifier;
    volumetricCloudsCB.m_DataPerLayer[1].CloudType =
        gAppSettings.m_CloudTypeModifier_2nd * gAppSettings.m_CloudTypeModifier_2nd * gAppSettings.m_CloudTypeModifier_2nd;

    volumetricCloudsCB.m_DataPerLayer[0].CloudTopOffset = gAppSettings.m_CloudTopOffset;
    volumetricCloudsCB.m_DataPerLayer[1].CloudTopOffset = gAppSettings.m_CloudTopOffset_2nd;

    // Modeling
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

    volumetricCloudsCB.m_DataPerLayer[0].Contrast = gAppSettings.m_Contrast;
    volumetricCloudsCB.m_DataPerLayer[0].Precipitation = gAppSettings.m_Precipitation;
    volumetricCloudsCB.m_DataPerLayer[1].Contrast = gAppSettings.m_Contrast_2nd;
    volumetricCloudsCB.m_DataPerLayer[1].Precipitation = gAppSettings.m_Precipitation_2nd;

    // Lighting
    volumetricCloudsCB.Eccentricity = gAppSettings.m_Eccentricity;
    volumetricCloudsCB.CloudBrightness = gAppSettings.m_CloudBrightness;
    volumetricCloudsCB.BackgroundBlendFactor = gAppSettings.m_BackgroundBlendFactor;

    float SilverIntensityCorrectionValue = (1.0f - abs(volumetricCloudsCB.lightDirection.getY()));
    SilverIntensityCorrectionValue *= SilverIntensityCorrectionValue;

    volumetricCloudsCB.SilverliningIntensity = gAppSettings.m_SilverIntensity * SilverIntensityCorrectionValue;
    volumetricCloudsCB.SilverliningSpread = gAppSettings.m_SilverSpread;

    g_ShadowInfo = vec4(gAppSettings.m_EnabledShadow ? 1.0f : 0.0f, gAppSettings.m_ShadowIntensity, gAppSettings.m_WeatherTexSize, 0.0f);

    // Wind
    volumetricCloudsCB.m_DataPerLayer[0].WindDirection =
        vec4(windXZ.getX(), 0.0, windXZ.getY(), gAppSettings.m_EnabledRotation ? 0.0f : gAppSettings.m_WindIntensity);
    volumetricCloudsCB.m_DataPerLayer[1].WindDirection =
        vec4(windXZ_2nd.getX(), 0.0, windXZ_2nd.getY(), gAppSettings.m_EnabledRotation_2nd ? 0.0f : gAppSettings.m_WindIntensity_2nd);

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
    volumetricCloudsCB.EnabledDepthCulling = gAppSettings.m_EnabledDepthCulling ? 1 : 0;

    // Godray
    volumetricCloudsCB.GodNumSamples = gAppSettings.m_GodNumSamples;
    volumetricCloudsCB.GodrayExposure = gAppSettings.m_Exposure;
    volumetricCloudsCB.GodrayDecay = gAppSettings.m_Decay;
    volumetricCloudsCB.GodrayDensity = gAppSettings.m_Density;
    volumetricCloudsCB.GodrayWeight = gAppSettings.m_Weight;

    // Test00 used for choosing the atmosphere's transmittance color over the sun predefined one
    volumetricCloudsCB.Test00 = gAppSettings.m_Test00;
    /*volumetricCloudsCB.Test01 = gAppSettings.m_Test01;
    volumetricCloudsCB.Test02 = gAppSettings.m_Test02;
    volumetricCloudsCB.Test03 = gAppSettings.m_Test03;*/

    volumetricCloudsCB.ReprojPrevFrameUnavail = gAppSettings.m_FirstFrame ? 1.0f : 0.0f;

    prevViewWithoutTranslation = viewMatWithoutTranslation;
    prevCameraPos = pCameraController->getViewPosition();
}

void VolumetricClouds::Update(uint frameIndex)
{
    gFrameIndex = frameIndex;
    BufferUpdateDesc BufferUniformSettingDesc = { VolumetricCloudsCBuffer[frameIndex] };
    beginUpdateResource(&BufferUniformSettingDesc);
    memcpy(BufferUniformSettingDesc.pMappedData, &volumetricCloudsCB, sizeof(volumetricCloudsCB));
    endUpdateResource(&BufferUniformSettingDesc);
}

bool VolumetricClouds::AfterSubmit(uint currentFrameIndex)
{
    UNREF_PARAM(currentFrameIndex);
    g_LowResFrameIndex = (g_LowResFrameIndex + 1) % (glowResBufferSize * glowResBufferSize);

    if (gAppSettings.prevDownSampling != gAppSettings.DownSampling)
    {
        gAppSettings.prevDownSampling = gAppSettings.DownSampling;
        return true;
    }

    if (gAppSettings.prevTemporalFilteringEnabled != gAppSettings.TemporalFilteringEnabled)
    {
        gAppSettings.prevTemporalFilteringEnabled = gAppSettings.TemporalFilteringEnabled;
        return true;
    }

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

    return float4(oneExtentX, oneExtentY, oneJitterX, oneJitterY); // xy = frustum extents at distance 1, zw = jitter at distance 1
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
    // HiZDepthDesc.mSrgb = false;

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
    HiZDepthDesc.mWidth = (mWidth & (~63)) / 32;
    HiZDepthDesc.mHeight = (mHeight & (~63)) / 32;
    HiZDepthDesc.pName = "HiZDepthBuffer X";
    HiZDepthTextureDesc.ppTexture = &pHiZDepthBufferX;
    HiZDepthTextureDesc.pDesc = &HiZDepthDesc;
    addResource(&HiZDepthTextureDesc, &token);

    waitForToken(&token);

    return pHiZDepthBuffer != NULL && pHiZDepthBuffer2 != NULL && pHiZDepthBufferX != NULL;
}

void VolumetricClouds::addVolumetricCloudsSaveTextures()
{
    SyncToken token = {};

    const uint32_t saveTextureWidth = (mWidth / gDownsampledCloudSize) & (~31);
    const uint32_t saveTextureheight = (mHeight / gDownsampledCloudSize) & (~31);

    TextureDesc SaveTextureDesc = {};
    SaveTextureDesc.mArraySize = 1;
    SaveTextureDesc.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;

    SaveTextureDesc.mWidth = saveTextureWidth;
    SaveTextureDesc.mHeight = saveTextureheight;
    SaveTextureDesc.mDepth = 1;

    SaveTextureDesc.mMipLevels = 1;
    SaveTextureDesc.mSampleCount = SAMPLE_COUNT_1;

    SaveTextureDesc.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
    SaveTextureDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE | DESCRIPTOR_TYPE_RW_TEXTURE;
    SaveTextureDesc.pName = "HighResolutionCloudTexture";

    TextureLoadDesc HighResTextureLoadDesc = {};
    HighResTextureLoadDesc.ppTexture = &pHighResCloudTexture;
    HighResTextureLoadDesc.pDesc = &SaveTextureDesc;
    addResource(&HighResTextureLoadDesc, &token);

    lowResTextureDesc.mArraySize = 1;
    lowResTextureDesc.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;

    lowResTextureDesc.mWidth = saveTextureWidth / glowResBufferSize;
    lowResTextureDesc.mHeight = saveTextureheight / glowResBufferSize;
    lowResTextureDesc.mDepth = 1;

    lowResTextureDesc.mMipLevels = 1;
    lowResTextureDesc.mSampleCount = SAMPLE_COUNT_1;
    // lowResTextureDesc.mSrgb = false;

    lowResTextureDesc.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
    lowResTextureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
    lowResTextureDesc.pName = "low Res Texture";
    TextureLoadDesc lowResLoadDesc = {};
    lowResLoadDesc.ppTexture = &plowResCloudTexture;
    lowResLoadDesc.pDesc = &lowResTextureDesc;
    addResource(&lowResLoadDesc, &token);

    TextureDesc reprojectedTextureDesc = {};
    reprojectedTextureDesc.mArraySize = 1;
    reprojectedTextureDesc.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;

    reprojectedTextureDesc.mWidth = saveTextureWidth;
    reprojectedTextureDesc.mHeight = saveTextureheight;
    reprojectedTextureDesc.mDepth = 1;

    reprojectedTextureDesc.mMipLevels = 1;
    reprojectedTextureDesc.mSampleCount = SAMPLE_COUNT_1;
    // reprojectedTextureDesc.mSrgb = false;

    reprojectedTextureDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
    reprojectedTextureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE | DESCRIPTOR_TYPE_TEXTURE;
    reprojectedTextureDesc.pName = "high Res Texture";

    TextureLoadDesc reprojectedTexLoadDesc = {};
    reprojectedTexLoadDesc.ppTexture = &pReprojectedCloudTexture;
    reprojectedTexLoadDesc.pDesc = &reprojectedTextureDesc;
    addResource(&reprojectedTexLoadDesc, &token);

    blurTextureDesc.mArraySize = 1;
    blurTextureDesc.mFormat = SaveTextureDesc.mFormat;

    blurTextureDesc.mWidth = SaveTextureDesc.mWidth / 2;
    blurTextureDesc.mHeight = SaveTextureDesc.mHeight / 2;
    blurTextureDesc.mDepth = SaveTextureDesc.mDepth;

    blurTextureDesc.mMipLevels = 1;
    blurTextureDesc.mSampleCount = SAMPLE_COUNT_1;
    // blurTextureDesc.mSrgb = false;

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

    gAppSettings.m_FirstFrame = true;
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

    for (uint i = 0; i < gDataBufferCount; i++)
    {
        ubSettingDesc.ppBuffer = &VolumetricCloudsCBuffer[i];
        addResource(&ubSettingDesc, NULL);
    }
}

void VolumetricClouds::RemoveUniformBuffers()
{
    for (uint i = 0; i < gDataBufferCount; i++)
    {
        removeResource(VolumetricCloudsCBuffer[i]);
    }
}

void VolumetricClouds::addDescriptorSets()
{
    DescriptorSetDesc setDesc = { pVolumetricCloudsRootSignatureCompute, DESCRIPTOR_UPDATE_FREQ_NONE, gCompShaderCount };
    addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetCompute[0]);
#if !USE_VC_FRAGMENTSHADER
    setDesc = { pVolumetricCloudsRootSignatureCompute, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount };
    addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetCompute[1]);
#endif
    setDesc = { pVolumetricCloudsRootSignatureGraphics, DESCRIPTOR_UPDATE_FREQ_NONE, gShaderCount };
    addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetGraphics[0]);
    setDesc = { pVolumetricCloudsRootSignatureGraphics, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount };
    addDescriptorSet(pRenderer, &setDesc, &pVolumetricCloudsDescriptorSetGraphics[1]);
}

void VolumetricClouds::removeDescriptorSets()
{
    removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetCompute[0]);
#if !USE_VC_FRAGMENTSHADER
    removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetCompute[1]);
#endif
    removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetGraphics[0]);
    removeDescriptorSet(pRenderer, pVolumetricCloudsDescriptorSetGraphics[1]);
}

void VolumetricClouds::addRootSignatures()
{
    const char* pVCSamplerNames[] = { "g_LinearWrapSampler", "g_LinearClampSampler", "g_PointClampSampler", "g_LinearBorderSampler",
                                      "g_NearestClampSampler" };
    Sampler*    pVCSamplers[] = { pBilinearSampler, pBiClampSampler, pPointClampSampler, pLinearBorderSampler, pHiZDepthClampSampler };

#if USE_VC_FRAGMENTSHADER
    Shader* shaders[] = { pReprojectionShader,
                          pPostProcessShader,
                          pPostProcessWithBlurShader,
                          pGodrayShader,
                          pGodrayAddShader,
                          pCompositeShader,
                          pCompositeOverlayShader,
                          pVolumetricCloudShader,
                          pVolumetricCloudWithDepthShader,
                          pVolumetricCloud2ndShader,
                          pVolumetricCloud2ndWithDepthShader,
                          pRealTimeVolumetricCloudShader,
                          pRealTimeVolumetricCloudWithDepthShader };
    Shader* shaderComps[] = { pGenHiZMipmapPRShader, pCopyRTShader, pHorizontalBlurShader, pVerticalBlurShader };
#else
    Shader*        shaders[] = { pReprojectionShader, pPostProcessShader, pPostProcessWithBlurShader, pGodrayShader,
                          pGodrayAddShader,    pCompositeShader,   pCompositeOverlayShader,    pVolumetricCloudShader };
    Shader*        shaderComps[] = { pGenHiZMipmapPRShader,
                              pCopyRTShader,
                              pHorizontalBlurShader,
                              pVerticalBlurShader,
                              pVolumetricCloudCompShader,
                              pVolumetricCloud2ndCompShader,
                              pVolumetricCloudWithDepthCompShader,
                              pVolumetricCloud2ndWithDepthCompShader,
                              pRealTimeVolumetricCloudCompShader,
                              pRealTimeVolumetricCloudWithDepthCompShader };
#endif

    RootSignatureDesc rootDesc = {};
    rootDesc.mShaderCount = gShaderCount;
    rootDesc.ppShaders = shaders;
    rootDesc.mStaticSamplerCount = 5;
    rootDesc.ppStaticSamplerNames = pVCSamplerNames;
    rootDesc.ppStaticSamplers = pVCSamplers;
    addRootSignature(pRenderer, &rootDesc, &pVolumetricCloudsRootSignatureGraphics);

    RootSignatureDesc rootCompDesc = {};
    rootCompDesc.mShaderCount = gCompShaderCount;
    rootCompDesc.ppShaders = shaderComps;
    rootCompDesc.mStaticSamplerCount = 5;
    rootCompDesc.ppStaticSamplerNames = pVCSamplerNames;
    rootCompDesc.ppStaticSamplers = pVCSamplers;
    addRootSignature(pRenderer, &rootCompDesc, &pVolumetricCloudsRootSignatureCompute);

    Shader*           GenHiZMipmapShaders[] = { pGenHiZMipmapShader };
    RootSignatureDesc rootGenHiZMipmapDesc = {};
    rootGenHiZMipmapDesc.mShaderCount = 1;
    rootGenHiZMipmapDesc.ppShaders = GenHiZMipmapShaders;
    addRootSignature(pRenderer, &rootGenHiZMipmapDesc, &pGenHiZMipmapRootSignature);

    Shader*           CopyTextureShaders[] = { pCopyTextureShader };
    RootSignatureDesc rootCopyTextureDesc = {};
    rootCopyTextureDesc.mShaderCount = 1;
    rootCopyTextureDesc.ppShaders = CopyTextureShaders;
    addRootSignature(pRenderer, &rootCopyTextureDesc, &pCopyTextureRootSignature);
}

void VolumetricClouds::removeRootSignatures()
{
    // removeRootSignature(pRenderer, pCompositeOverlayRootSignature);

    removeRootSignature(pRenderer, pCopyTextureRootSignature);
    // removeRootSignature(pRenderer, pCopyWeatherTextureRootSignature);

    removeRootSignature(pRenderer, pGenHiZMipmapRootSignature);
    removeRootSignature(pRenderer, pVolumetricCloudsRootSignatureGraphics);
    removeRootSignature(pRenderer, pVolumetricCloudsRootSignatureCompute);
}

void VolumetricClouds::addShaders()
{
    ShaderLoadDesc basicShader = {};
    basicShader.mStages[0].pFileName = "VolumetricCloud.vert";
    basicShader.mStages[1].pFileName = "PostProcess.frag";
    addShader(pRenderer, &basicShader, &pPostProcessShader);

    ShaderLoadDesc postBlurShader = {};
    postBlurShader.mStages[0].pFileName = "VolumetricCloud.vert";
    postBlurShader.mStages[1].pFileName = "PostProcessWithBlur.frag";
    addShader(pRenderer, &postBlurShader, &pPostProcessWithBlurShader);

    ShaderLoadDesc reprojectionShader = {};
    reprojectionShader.mStages[0].pFileName = "VolumetricCloud.vert";
    reprojectionShader.mStages[1].pFileName = "Reprojection.frag";
    addShader(pRenderer, &reprojectionShader, &pReprojectionShader);

    ShaderLoadDesc godrayShader = {};
    godrayShader.mStages[0].pFileName = "Triangular.vert";
    godrayShader.mStages[1].pFileName = "Godray.frag";
    addShader(pRenderer, &godrayShader, &pGodrayShader);

    ShaderLoadDesc godrayAddShader = {};
    godrayAddShader.mStages[0].pFileName = "Triangular.vert";
    godrayAddShader.mStages[1].pFileName = "GodrayAdd.frag";
    addShader(pRenderer, &godrayAddShader, &pGodrayAddShader);

    ShaderLoadDesc compositeShader = {};
    compositeShader.mStages[0].pFileName = "VolumetricCloud.vert";
    compositeShader.mStages[1].pFileName = "Composite.frag";
    addShader(pRenderer, &compositeShader, &pCompositeShader);

    ShaderLoadDesc compositeOverlayShader = {};
    compositeOverlayShader.mStages[0].pFileName = "VolumetricCloud.vert";
    compositeOverlayShader.mStages[1].pFileName = "CompositeOverlay.frag";
    addShader(pRenderer, &compositeOverlayShader, &pCompositeOverlayShader);

    ShaderLoadDesc GenHiZMipmapShader = {};
    GenHiZMipmapShader.mStages[0].pFileName = "HiZdownSampling.comp";
    addShader(pRenderer, &GenHiZMipmapShader, &pGenHiZMipmapShader);

    ShaderLoadDesc GenHiZMipmapPRShader = {};
    GenHiZMipmapPRShader.mStages[0].pFileName = "HiZdownSamplingPR.comp";
    addShader(pRenderer, &GenHiZMipmapPRShader, &pGenHiZMipmapPRShader);

    ShaderLoadDesc CopyTextureShader = {};
    CopyTextureShader.mStages[0].pFileName = "CopyTexture.comp";
    addShader(pRenderer, &CopyTextureShader, &pCopyTextureShader);

    /*ShaderLoadDesc CopyWeatherTextureShader = {};
    CopyWeatherTextureShader.mStages[0] = { "VolumetricClouds/copyWeatherTexture.comp", NULL, 0, NULL };
    addShader(pRenderer, &CopyWeatherTextureShader, &pCopyWeatherTextureShader);

    Shader* CopyWeatherTextureShaders[] = { pCopyWeatherTextureShader };
    */

    ShaderLoadDesc BlurShader = {};
    BlurShader.mStages[0].pFileName = "BlurHorizontal.comp";
    addShader(pRenderer, &BlurShader, &pHorizontalBlurShader);

    BlurShader.mStages[0].pFileName = "BlurVertical.comp";
    addShader(pRenderer, &BlurShader, &pVerticalBlurShader);

    ShaderLoadDesc CopyRTShader = {};
    CopyRTShader.mStages[0].pFileName = "CopyRT.comp";
    addShader(pRenderer, &CopyRTShader, &pCopyRTShader);

    ShaderLoadDesc VolumetricCloudShaderDesc = {};
    VolumetricCloudShaderDesc.mStages[0].pFileName = "VolumetricCloud.vert";
    VolumetricCloudShaderDesc.mStages[1].pFileName = "VolumetricCloud.frag";
    addShader(pRenderer, &VolumetricCloudShaderDesc, &pVolumetricCloudShader);

#if USE_VC_FRAGMENTSHADER

    VolumetricCloudShaderDesc = {};
    VolumetricCloudShaderDesc.mStages[0].pFileName = "VolumetricCloud.vert";
    VolumetricCloudShaderDesc.mStages[1].pFileName = "VolumetricCloudWithDepth.frag";
    addShader(pRenderer, &VolumetricCloudShaderDesc, &pVolumetricCloudWithDepthShader);

    VolumetricCloudShaderDesc = {};
    VolumetricCloudShaderDesc.mStages[0].pFileName = "VolumetricCloud.vert";
    VolumetricCloudShaderDesc.mStages[1].pFileName = "VolumetricCloud_2nd.frag";
    addShader(pRenderer, &VolumetricCloudShaderDesc, &pVolumetricCloud2ndShader);

    VolumetricCloudShaderDesc = {};
    VolumetricCloudShaderDesc.mStages[0].pFileName = "VolumetricCloud.vert";
    VolumetricCloudShaderDesc.mStages[1].pFileName = "VolumetricCloud_2ndWithDepth.frag";
    addShader(pRenderer, &VolumetricCloudShaderDesc, &pVolumetricCloud2ndWithDepthShader);

    VolumetricCloudShaderDesc = {};
    VolumetricCloudShaderDesc.mStages[0].pFileName = "VolumetricCloud.vert";
    VolumetricCloudShaderDesc.mStages[1].pFileName = "RealTimeVolumetricCloud.frag";
    addShader(pRenderer, &VolumetricCloudShaderDesc, &pRealTimeVolumetricCloudShader);

    VolumetricCloudShaderDesc = {};
    VolumetricCloudShaderDesc.mStages[0].pFileName = "VolumetricCloud.vert";
    VolumetricCloudShaderDesc.mStages[1].pFileName = "RealTimeVolumetricCloudWithDepth.frag";
    addShader(pRenderer, &VolumetricCloudShaderDesc, &pRealTimeVolumetricCloudWithDepthShader);

#else
    ShaderLoadDesc VolumetricCloudCompShaderDesc = {};
    VolumetricCloudCompShaderDesc.mStages[0].pFileName = "VolumetricCloud.comp";
    addShader(pRenderer, &VolumetricCloudCompShaderDesc, &pVolumetricCloudCompShader);

    ShaderLoadDesc VolumetricCloud2ndCompShaderDesc = {};
    VolumetricCloud2ndCompShaderDesc.mStages[0].pFileName = "VolumetricCloud_2nd.comp";
    addShader(pRenderer, &VolumetricCloud2ndCompShaderDesc, &pVolumetricCloud2ndCompShader);

    ShaderLoadDesc VolumetricCloudWithDepthCompShaderDesc = {};
    VolumetricCloudWithDepthCompShaderDesc.mStages[0].pFileName = "VolumetricCloudWithDepth.comp";
    addShader(pRenderer, &VolumetricCloudWithDepthCompShaderDesc, &pVolumetricCloudWithDepthCompShader);

    ShaderLoadDesc VolumetricCloud2ndWithDepthCompShaderDesc = {};
    VolumetricCloud2ndWithDepthCompShaderDesc.mStages[0].pFileName = "VolumetricCloud_2ndWithDepth.comp";
    addShader(pRenderer, &VolumetricCloud2ndWithDepthCompShaderDesc, &pVolumetricCloud2ndWithDepthCompShader);

    ShaderLoadDesc RealtimeCloudShaderDesc = {};
    RealtimeCloudShaderDesc.mStages[0].pFileName = "RealTimeVolumetricCloud.comp";
    addShader(pRenderer, &RealtimeCloudShaderDesc, &pRealTimeVolumetricCloudCompShader);

    ShaderLoadDesc RealtimeCloudShaderWithDepthDesc = {};
    RealtimeCloudShaderWithDepthDesc.mStages[0].pFileName = "RealTimeVolumetricCloudWithDepth.comp";
    addShader(pRenderer, &RealtimeCloudShaderWithDepthDesc, &pRealTimeVolumetricCloudWithDepthCompShader);
#endif
}

void VolumetricClouds::removeShaders()
{
    removeShader(pRenderer, pCompositeOverlayShader);

    // removeShader(pRenderer, pSaveCurrentShader);
    removeShader(pRenderer, pPostProcessShader);
    removeShader(pRenderer, pVolumetricCloudShader);

#if USE_VC_FRAGMENTSHADER
    removeShader(pRenderer, pVolumetricCloudWithDepthShader);
    removeShader(pRenderer, pVolumetricCloud2ndShader);
    removeShader(pRenderer, pVolumetricCloud2ndWithDepthShader);
    removeShader(pRenderer, pRealTimeVolumetricCloudShader);
    removeShader(pRenderer, pRealTimeVolumetricCloudWithDepthShader);
#else
    removeShader(pRenderer, pVolumetricCloudCompShader);
    removeShader(pRenderer, pVolumetricCloud2ndCompShader);
    removeShader(pRenderer, pVolumetricCloudWithDepthCompShader);
    removeShader(pRenderer, pVolumetricCloud2ndWithDepthCompShader);
    removeShader(pRenderer, pRealTimeVolumetricCloudCompShader);
    removeShader(pRenderer, pRealTimeVolumetricCloudWithDepthCompShader);
#endif
    removeShader(pRenderer, pReprojectionShader);
    removeShader(pRenderer, pGodrayShader);
    removeShader(pRenderer, pGodrayAddShader);

    removeShader(pRenderer, pGenHiZMipmapShader);
    removeShader(pRenderer, pCopyTextureShader);
    // removeShader(pRenderer, pCopyWeatherTextureShader);
    removeShader(pRenderer, pCopyRTShader);
    removeShader(pRenderer, pCompositeShader);
    removeShader(pRenderer, pGenHiZMipmapPRShader);

    removeShader(pRenderer, pHorizontalBlurShader);
    removeShader(pRenderer, pVerticalBlurShader);

    removeShader(pRenderer, pPostProcessWithBlurShader);
}

void VolumetricClouds::addPipelines()
{
    VertexLayout vertexLayoutForVC = {};
    vertexLayoutForVC.mBindingCount = 1;
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

    blendStateSkyBoxDesc.mSrcFactors[0] = BC_SRC_ALPHA;           // BC_ONE;// BC_ONE_MINUS_DST_ALPHA;
    blendStateSkyBoxDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA; // BC_ZERO;// BC_DST_ALPHA;

    blendStateSkyBoxDesc.mSrcAlphaFactors[0] = BC_ONE;
    blendStateSkyBoxDesc.mDstAlphaFactors[0] = BC_ZERO;

    blendStateSkyBoxDesc.mColorWriteMasks[0] = COLOR_MASK_ALL;
    blendStateSkyBoxDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BlendStateDesc blendStateGodrayDesc = {};
    blendStateGodrayDesc.mBlendModes[0] = BM_ADD;
    blendStateGodrayDesc.mBlendAlphaModes[0] = BM_ADD;

    blendStateGodrayDesc.mSrcFactors[0] = BC_ONE;
    blendStateGodrayDesc.mDstFactors[0] = BC_ONE;

    blendStateGodrayDesc.mSrcAlphaFactors[0] = BC_ZERO;
    blendStateGodrayDesc.mDstAlphaFactors[0] = BC_ONE;

    blendStateGodrayDesc.mColorWriteMasks[0] = COLOR_MASK_ALL;
    blendStateGodrayDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;

    RasterizerStateDesc rasterizerStateDesc = {};
    rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

    PipelineDesc pipelineDescVolumetricCloud = {};
    pipelineDescVolumetricCloud.pCache = pPipelineCache;
    {
        pipelineDescVolumetricCloud.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettings = pipelineDescVolumetricCloud.mGraphicsDesc;

        pipelineSettings = { 0 };
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = NULL;
        pipelineSettings.pColorFormats = &plowResCloudRT->mFormat;
        pipelineSettings.mSampleCount = SAMPLE_COUNT_1;
        pipelineSettings.mSampleQuality = 0;

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
        GraphicsPipelineDesc& pipelineSettings = pipelineDescVolumetricCloudWithDepthShader.mGraphicsDesc;

        pipelineSettings = { 0 };
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = NULL;
        pipelineSettings.pColorFormats = &plowResCloudRT->mFormat;
        pipelineSettings.mSampleCount = SAMPLE_COUNT_1;
        pipelineSettings.mSampleQuality = 0;

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
        GraphicsPipelineDesc& pipelineSettings = pipelineDescVolumetricCloud2nd.mGraphicsDesc;

        pipelineSettings = { 0 };
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = NULL;
        pipelineSettings.pColorFormats = &plowResCloudRT->mFormat;
        pipelineSettings.mSampleCount = SAMPLE_COUNT_1;
        pipelineSettings.mSampleQuality = 0;

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
        GraphicsPipelineDesc& pipelineSettings = pipelineDescVolumetricCloud2ndWithDepth.mGraphicsDesc;

        pipelineSettings = { 0 };
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = NULL;
        pipelineSettings.pColorFormats = &plowResCloudRT->mFormat;
        pipelineSettings.mSampleCount = SAMPLE_COUNT_1;
        pipelineSettings.mSampleQuality = 0;

        pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
        pipelineSettings.pShaderProgram = pVolumetricCloud2ndWithDepthShader;
        pipelineSettings.pVertexLayout = &vertexLayoutForVC;
        pipelineSettings.pRasterizerState = &rasterizerStateDesc;

        addPipeline(pRenderer, &pipelineDescVolumetricCloud2ndWithDepth, &pVolumetricCloud2ndWithDepthPipeline);
    }

    // --- No temporal filtering
    PipelineDesc pipelineDescVolumetricCloudRealTime = {};
    pipelineDescVolumetricCloudRealTime.pCache = pPipelineCache;
    {
        pipelineDescVolumetricCloudRealTime.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettings = pipelineDescVolumetricCloudRealTime.mGraphicsDesc;

        pipelineSettings = { 0 };
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = NULL;
        pipelineSettings.pColorFormats = &phighResCloudRT->mFormat;
        pipelineSettings.mSampleCount = SAMPLE_COUNT_1;
        pipelineSettings.mSampleQuality = 0;

        pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
        pipelineSettings.pShaderProgram = pRealTimeVolumetricCloudShader;
        pipelineSettings.pVertexLayout = &vertexLayoutForVC;
        pipelineSettings.pRasterizerState = &rasterizerStateDesc;
        addPipeline(pRenderer, &pipelineDescVolumetricCloudRealTime, &pRealTimeVolumetricCloudPipeline);

        pipelineSettings.pShaderProgram = pRealTimeVolumetricCloudWithDepthShader;
        addPipeline(pRenderer, &pipelineDescVolumetricCloudRealTime, &pRealTimeVolumetricCloudWithDepthPipeline);
    }
#endif

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    PipelineDesc pipelineDescReprojection = {};
    pipelineDescReprojection.pCache = pPipelineCache;
    {
        pipelineDescReprojection.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettings = pipelineDescReprojection.mGraphicsDesc;

        pipelineSettings = { 0 };
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = NULL;
        pipelineSettings.pColorFormats = &pReprojectedCloudRT->mFormat;
        // pipelineSettings.pSrgbValues = &pReprojectedCloudRT->mSrgb;
        pipelineSettings.mSampleCount = SAMPLE_COUNT_1;
        pipelineSettings.mSampleQuality = 0;

        // pipelineSettings.pRootSignature = pReprojectionRootSignature;
        pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
        pipelineSettings.pShaderProgram = pReprojectionShader;
        pipelineSettings.pVertexLayout = &vertexLayoutForVC;
        pipelineSettings.pRasterizerState = &rasterizerStateDesc;

        addPipeline(pRenderer, &pipelineDescReprojection, &pReprojectionPipeline);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    PipelineDesc pipelineDescGodray = {};
    pipelineDescGodray.pCache = pPipelineCache;
    {
        pipelineDescGodray.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettings = pipelineDescGodray.mGraphicsDesc;

        pipelineSettings = { 0 };
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = NULL;
        pipelineSettings.pColorFormats = &pGodrayRT->mFormat;
        // pipelineSettings.pSrgbValues = &pGodrayRT->mSrgb;
        pipelineSettings.mSampleCount = SAMPLE_COUNT_1;
        pipelineSettings.mSampleQuality = 0;

        // pipelineSettings.pRootSignature = pGodrayRootSignature;
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
        GraphicsPipelineDesc& pipelineSettings = pipelineDescPostProcess.mGraphicsDesc;

        pipelineSettings = { 0 };
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = NULL;

        TinyImageFormat mrtFormats[1] = {};
        for (uint32_t i = 0; i < 1; ++i)
        {
            mrtFormats[i] = pRenderTargetsPostProcess[i]->mFormat;
            // mrtSrgb[i] = pRenderTargetsPostProcess[i]->mSrgb;
        }

        pipelineSettings.pColorFormats = mrtFormats;
        // pipelineSettings.pSrgbValues = mrtSrgb;
        pipelineSettings.mSampleCount = SAMPLE_COUNT_1;
        pipelineSettings.mSampleQuality = 0;
        // pipelineSettings.pRootSignature = pPostProcessRootSignature;
        pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
        pipelineSettings.pShaderProgram = pPostProcessShader;
        pipelineSettings.pVertexLayout = &vertexLayoutForVC;
        pipelineSettings.pRasterizerState = &rasterizerStateDesc;

        addPipeline(pRenderer, &pipelineDescPostProcess, &pPostProcessPipeline);

        // pipelineSettings.pRootSignature = pPostProcessWithBlurRootSignature;
        pipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureGraphics;
        pipelineSettings.pShaderProgram = pPostProcessWithBlurShader;

        addPipeline(pRenderer, &pipelineDescPostProcess, &pPostProcessWithBlurPipeline);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TinyImageFormat mFinalRtFormats[1] = { TinyImageFormat_R10G10B10A2_UNORM };

    PipelineDesc pipelineDescGodrayAdd = {};
    pipelineDescGodrayAdd.pCache = pPipelineCache;
    {
        pipelineDescGodrayAdd.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettings = pipelineDescGodrayAdd.mGraphicsDesc;
        DepthStateDesc        depthState{};
        depthState.mDepthTest = true;
        depthState.mDepthWrite = false;
        depthState.mDepthFunc = CMP_LEQUAL;

        pipelineSettings = { 0 };
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = &depthState;
        pipelineSettings.mDepthStencilFormat = pDepthTexture->mFormat;
        pipelineSettings.pColorFormats = mFinalRtFormats;
        // pipelineSettings.pSrgbValues = &(*pFinalRT)->mSrgb;
        pipelineSettings.mSampleCount = SAMPLE_COUNT_1;
        pipelineSettings.mSampleQuality = 0;
        // pipelineSettings.pRootSignature = pGodrayAddRootSignature;
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
        GraphicsPipelineDesc& pipelineSettings = pipelineDescComposite.mGraphicsDesc;

        pipelineSettings = { 0 };
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = NULL;
        pipelineSettings.pColorFormats = mFinalRtFormats;
        // pipelineSettings.pSrgbValues = &(*pFinalRT)->mSrgb;
        pipelineSettings.mSampleCount = SAMPLE_COUNT_1;
        pipelineSettings.mSampleQuality = 0;
        // pipelineSettings.pRootSignature = pCompositeRootSignature;
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
        GraphicsPipelineDesc& pipelineSettings = pipelineDescCompositeOverlay.mGraphicsDesc;

        pipelineSettings = { 0 };
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = NULL;
        pipelineSettings.pColorFormats = mFinalRtFormats;
        // pipelineSettings.pSrgbValues = &(*pFinalRT)->mSrgb;
        pipelineSettings.mSampleCount = SAMPLE_COUNT_1;
        pipelineSettings.mSampleQuality = 0;
        // pipelineSettings.pRootSignature = pCompositeOverlayRootSignature;
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

    ComputePipelineDesc& comPipelineSettings = pipelineDesc.mComputeDesc;

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

    comPipelineSettings.pShaderProgram = pRealTimeVolumetricCloudCompShader;
    comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
    addPipeline(pRenderer, &pipelineDesc, &pRealTimeVolumetricCompPipeline);

    comPipelineSettings.pShaderProgram = pRealTimeVolumetricCloudWithDepthCompShader;
    comPipelineSettings.pRootSignature = pVolumetricCloudsRootSignatureCompute;
    addPipeline(pRenderer, &pipelineDesc, &pRealTimeVolumetricWithDepthCompPipeline);
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
}

void VolumetricClouds::removePipelines()
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
    removePipeline(pRenderer, pRealTimeVolumetricCloudPipeline);
    removePipeline(pRenderer, pRealTimeVolumetricCloudWithDepthPipeline);
#else
    removePipeline(pRenderer, pVolumetricCloudCompPipeline);
    removePipeline(pRenderer, pVolumetricCloud2ndCompPipeline);
    removePipeline(pRenderer, pVolumetricCloudWithDepthCompPipeline);
    removePipeline(pRenderer, pVolumetricCloud2ndWithDepthCompPipeline);
    removePipeline(pRenderer, pRealTimeVolumetricCompPipeline);
    removePipeline(pRenderer, pRealTimeVolumetricWithDepthCompPipeline);
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
    // removePipeline(pRenderer, pReprojectionCompPipeline);
    // removePipeline(pRenderer, pCastShadowPipeline);
}

void VolumetricClouds::addRenderTargets()
{
    RenderTargetDesc reprojectedCloudRT = {};
    reprojectedCloudRT.mArraySize = 1;
    reprojectedCloudRT.mDepth = 1;
    reprojectedCloudRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
    reprojectedCloudRT.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
    reprojectedCloudRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
    reprojectedCloudRT.mSampleCount = SAMPLE_COUNT_1;
    reprojectedCloudRT.mSampleQuality = 0;

    reprojectedCloudRT.mWidth =
        (mWidth / gDownsampledCloudSize) &
        (~31); // make sure the width and height could be divided by 4, otherwise the low-res buffer can't be aligned with full-res buffer
    reprojectedCloudRT.mHeight = (mHeight / gDownsampledCloudSize) & (~31);
    reprojectedCloudRT.pName = "ReprojectedCloudRT";
    addRenderTarget(pRenderer, &reprojectedCloudRT, &pReprojectedCloudRT);

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
    CurrentCloudRT.mFormat = reprojectedCloudRT.mFormat;
    CurrentCloudRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
    CurrentCloudRT.mSampleCount = SAMPLE_COUNT_1;
    CurrentCloudRT.mSampleQuality = 0;
    CurrentCloudRT.pName = "LowResolutionCloudRT";
    CurrentCloudRT.mWidth = reprojectedCloudRT.mWidth / glowResBufferSize;
    CurrentCloudRT.mHeight = reprojectedCloudRT.mHeight / glowResBufferSize;
    addRenderTarget(pRenderer, &CurrentCloudRT, &plowResCloudRT);

    // No temporal filtering
    CurrentCloudRT.mWidth = (mWidth / gDownsampledCloudSize) & (~31);
    CurrentCloudRT.mHeight = (mHeight / gDownsampledCloudSize) & (~31);
    CurrentCloudRT.pName = "HighResolutionCloudRT";
    addRenderTarget(pRenderer, &CurrentCloudRT, &phighResCloudRT);

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
}

void VolumetricClouds::removeRenderTargets()
{
    removeRenderTarget(pRenderer, plowResCloudRT);
    removeRenderTarget(pRenderer, phighResCloudRT);
    removeRenderTarget(pRenderer, pReprojectedCloudRT);
    removeRenderTarget(pRenderer, pPostProcessRT);
    removeRenderTarget(pRenderer, pGodrayRT);
    removeRenderTarget(pRenderer, pCastShadowRT);
}

void VolumetricClouds::prepareDescriptorSets(RenderTarget** ppPreStageRenderTargets, uint32_t count)
{
    UNREF_PARAM(count);
    pFinalRT = ppPreStageRenderTargets[0];
    pSceneColorTexture = ppPreStageRenderTargets[1];
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
        Texture*       HiZedDepthTexture = pHiZDepthBuffer;
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
        VCParams[0].mBindMipChain = true;
        VCParams[0].ppTextures = &pHighFrequency3DTexture;
        VCParams[1].pName = "lowFreqNoiseTexture";
        VCParams[1].mBindMipChain = true;
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

        VCParams[4].pName = "volumetricCloudsDstTexture";
        VCParams[4].ppTextures = &pHighResCloudTexture;

        updateDescriptorSet(pRenderer, 7, pVolumetricCloudsDescriptorSetCompute[0], 6, VCParams);

        VCParams[5].pName = "depthTexture";
        VCParams[5].ppTextures = &HiZedDepthTexture;
        updateDescriptorSet(pRenderer, 6, pVolumetricCloudsDescriptorSetCompute[0], 6, VCParams);
#endif
        for (uint32_t i = 0; i < gDataBufferCount; ++i)
        {
            DescriptorData params[1] = {};
            params[0].pName = "VolumetricCloudsCBuffer";
            params[0].ppBuffers = &VolumetricCloudsCBuffer[i];
#if !USE_VC_FRAGMENTSHADER
            updateDescriptorSet(pRenderer, i, pVolumetricCloudsDescriptorSetCompute[1], 1, params);
#endif
            updateDescriptorSet(pRenderer, i, pVolumetricCloudsDescriptorSetGraphics[1], 1, params);
        }
    }
    // Reprojection
    {
        DescriptorData RPparams[2] = {};
        RPparams[0].pName = "LowResCloudTexture";
#if USE_VC_FRAGMENTSHADER
        RPparams[0].ppTextures = &plowResCloudRT->pTexture;
#else
        RPparams[0].ppTextures = &plowResCloudTexture;
#endif
        RPparams[1].pName = "g_PrevFrameTexture";
        RPparams[1].ppTextures = &pHighResCloudTexture;

        updateDescriptorSet(pRenderer, 0, pVolumetricCloudsDescriptorSetGraphics[0], 2, RPparams);
    }
    // Copy RT
    {
        DescriptorData ppParams[2] = {};
        ppParams[0].pName = "SrcTexture";
        ppParams[0].ppTextures = &pReprojectedCloudRT->pTexture;
        ppParams[1].pName = "FullResCloudTexture";
        ppParams[1].ppTextures = &pHighResCloudTexture;
        updateDescriptorSet(pRenderer, 1, pVolumetricCloudsDescriptorSetCompute[0], 2, ppParams);
    }
    // HBlur
    {
        DescriptorData params[2] = {};
        params[0].pName = "InputTex";
#if USE_VC_FRAGMENTSHADER
        params[0].ppTextures = gAppSettings.TemporalFilteringEnabled ? &pHighResCloudTexture : &phighResCloudRT->pTexture;
#else
        params[0].ppTextures = &pHighResCloudTexture;
#endif
        params[1].pName = "OutputTex";
        params[1].ppTextures = &pHBlurTex;
        updateDescriptorSet(pRenderer, 2, pVolumetricCloudsDescriptorSetCompute[0], 2, params);
    }
    // VBlur
    {
        DescriptorData params[2] = {};
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
#if USE_VC_FRAGMENTSHADER
        PPparams[0].ppTextures = gAppSettings.TemporalFilteringEnabled ? &pReprojectedCloudRT->pTexture : &phighResCloudRT->pTexture;
#else
        PPparams[0].ppTextures = gAppSettings.TemporalFilteringEnabled ? &pReprojectedCloudRT->pTexture : &pHighResCloudTexture;
#endif
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
        PPparams[0].pName = "g_HighResolutionCloudTexture";
#if USE_VC_FRAGMENTSHADER
        PPparams[0].ppTextures = gAppSettings.TemporalFilteringEnabled ? &pReprojectedCloudRT->pTexture : &phighResCloudRT->pTexture;
#else
        PPparams[0].ppTextures = gAppSettings.TemporalFilteringEnabled ? &pReprojectedCloudRT->pTexture : &pHighResCloudTexture;
#endif
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
        Presentpparams[1].ppTextures = &pLinearDepthTexture->pTexture;
        Presentpparams[2].pName = "g_HighResolutionCloudTexture";
#if USE_VC_FRAGMENTSHADER
        Presentpparams[2].ppTextures = gAppSettings.TemporalFilteringEnabled ? &pReprojectedCloudRT->pTexture : &phighResCloudRT->pTexture;
#else
        Presentpparams[2].ppTextures = gAppSettings.TemporalFilteringEnabled ? &pReprojectedCloudRT->pTexture : &pHighResCloudTexture;
#endif
        updateDescriptorSet(pRenderer, 5, pVolumetricCloudsDescriptorSetGraphics[0], 3, Presentpparams);
    }
    // Add Godray
    {
        DescriptorData Presentpparams[4] = {};
        Presentpparams[0].pName = "g_GodrayTexture";
        Presentpparams[0].ppTextures = &pGodrayRT->pTexture;
        // Presentpparams[1].pName = "depthTexture";
        // Presentpparams[1].ppTextures = &pOriginDepthTexture;
        updateDescriptorSet(pRenderer, 4, pVolumetricCloudsDescriptorSetGraphics[0], 1, Presentpparams);
    }
}

void VolumetricClouds::InitializeWithLoad(RenderTarget* InLinearDepthTexture, RenderTarget* InDepthTexture)
{
    pLinearDepthTexture = InLinearDepthTexture;
    pDepthTexture = InDepthTexture;
}

void VolumetricClouds::Initialize(ICameraController* InCameraController, Queue* InGraphicsQueue, ProfileToken InGraphicsGpuProfiler,
                                  Buffer* InTransmittanceBuffer)
{
    pCameraController = InCameraController;
    pGraphicsQueue = InGraphicsQueue;
    gGpuProfileToken = InGraphicsGpuProfiler;
    pTransmittanceBuffer = InTransmittanceBuffer;
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
    GenHighTopFreq3DTexShader.mStages[0].pFileName = "GenHighTopFreq3Dtex.comp";
    addShader(pRenderer, &GenHighTopFreq3DTexShader, &pGenHighTopFreq3DTexShader);

    ShaderLoadDesc GenLowTopFreq3DTexShader = {};
    GenLowTopFreq3DTexShader.mStages[0].pFileName = "GenLowTopFreq3Dtex.comp";
    addShader(pRenderer, &GenLowTopFreq3DTexShader, &pGenLowTopFreq3DTexShader);

    ShaderLoadDesc GenMipmapShader = {};
    GenMipmapShader.mStages[0].pFileName = "Gen3DtexMipmap.comp";
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
    params[0].mCount = gHighFreq3DTextureSize;
    params[0].ppTextures = gHighFrequencyOriginTextureStorage;
    params[1].pName = "DstTexture";
    params[1].ppTextures = &pHighFrequency3DTexture;
    updateDescriptorSet(pRenderer, 0, pGenHighTopFreq3DTexDescriptorSet, 2, params);

    params[0] = params[1] = {};
    params[0].pName = "SrcTexture";
    params[0].mCount = gLowFreq3DTextureSize;
    params[0].ppTextures = gLowFrequencyOriginTextureStorage;
    params[1].pName = "DstTexture";
    params[1].ppTextures = &pLowFrequency3DTexture;
    updateDescriptorSet(pRenderer, 0, pGenLowTopFreq3DTexDescriptorSet, 2, params);

    PipelineDesc pipelineDesc = {};
    pipelineDesc.pCache = pPipelineCache;
    pipelineDesc.mType = PIPELINE_TYPE_COMPUTE;

    ComputePipelineDesc& comPipelineSettings = pipelineDesc.mComputeDesc;
    comPipelineSettings.pShaderProgram = pGenHighTopFreq3DTexShader;
    comPipelineSettings.pRootSignature = pGenHighTopFreq3DTexRootSignature;
    addPipeline(pRenderer, &pipelineDesc, &pGenHighTopFreq3DTexPipeline);

    comPipelineSettings.pShaderProgram = pGenLowTopFreq3DTexShader;
    comPipelineSettings.pRootSignature = pGenLowTopFreq3DTexRootSignature;
    addPipeline(pRenderer, &pipelineDesc, &pGenLowTopFreq3DTexPipeline);

    comPipelineSettings.pShaderProgram = pGenMipmapShader;
    comPipelineSettings.pRootSignature = pGenMipmapRootSignature;
    addPipeline(pRenderer, &pipelineDesc, &pGenMipmapPipeline);

    GpuCmdRing     preprocessRing = {};
    GpuCmdRingDesc cmdRingDesc = {};
    cmdRingDesc.pQueue = pGraphicsQueue;
    cmdRingDesc.mPoolCount = 1;
    cmdRingDesc.mCmdPerPoolCount = 1;
    cmdRingDesc.mAddSyncPrimitives = true;
    addGpuCmdRing(pRenderer, &cmdRingDesc, &preprocessRing);
    GpuCmdRingElement elem = getNextGpuCmdRingElement(&preprocessRing, true, 1);
    Cmd*              cmd = elem.pCmds[0];

    beginCmd(cmd);

    cmdBindPipeline(cmd, pGenHighTopFreq3DTexPipeline);
    cmdBindDescriptorSet(cmd, 0, pGenHighTopFreq3DTexDescriptorSet);
    uint32_t* pThreadGroupSize = pGenHighTopFreq3DTexShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
    cmdDispatch(cmd, gHighFreq3DTextureSize / pThreadGroupSize[0], gHighFreq3DTextureSize / pThreadGroupSize[1],
                gHighFreq3DTextureSize / pThreadGroupSize[2]);

    cmdBindPipeline(cmd, pGenLowTopFreq3DTexPipeline);
    cmdBindDescriptorSet(cmd, 0, pGenLowTopFreq3DTexDescriptorSet);
    pThreadGroupSize = pGenLowTopFreq3DTexShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;
    cmdDispatch(cmd, gLowFreq3DTextureSize / pThreadGroupSize[0], gLowFreq3DTextureSize / pThreadGroupSize[1],
                gLowFreq3DTextureSize / pThreadGroupSize[2]);

    TextureBarrier barriersForNoise[] = {
        { pHighFrequency3DTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_UNORDERED_ACCESS },
        { pLowFrequency3DTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_UNORDERED_ACCESS },
    };
    cmdResourceBarrier(cmd, 0, NULL, 2, barriersForNoise, 0, NULL);

    cmdBindPipeline(cmd, pGenMipmapPipeline);
    pThreadGroupSize = pGenMipmapShader->pReflection->mStageReflections[0].mNumThreadsPerGroup;

    struct Data
    {
        uint mipSize;
    } data = { 0 };

    uint32_t setIndex = 0;
    uint32_t rootConstantIndex = getDescriptorIndexFromName(pGenMipmapRootSignature, "RootConstant");

    for (uint32_t i = 1; i < HighFreqMipCount; i++, ++setIndex)
    {
        DescriptorData mipParams[2] = {};
        mipParams[0].pName = "SrcTexture";
        mipParams[0].ppTextures = &pHighFrequency3DTexture;
        mipParams[0].mUAVMipSlice = (uint16_t)(i - 1);
        mipParams[1].pName = "DstTexture";
        mipParams[1].ppTextures = &pHighFrequency3DTexture;
        mipParams[1].mUAVMipSlice = (uint16_t)i;
        updateDescriptorSet(pRenderer, setIndex, pGenMipmapDescriptorSet, 2, mipParams);
        cmdBindDescriptorSet(cmd, setIndex, pGenMipmapDescriptorSet);

        data.mipSize = gHighFreq3DTextureSize >> i;
        cmdBindPushConstants(cmd, pGenMipmapRootSignature, rootConstantIndex, &data);
        cmdDispatch(cmd, max(1u, data.mipSize / pThreadGroupSize[0]), max(1u, data.mipSize / pThreadGroupSize[1]),
                    max(1u, data.mipSize / pThreadGroupSize[2]));

        TextureBarrier barrier = { pHighFrequency3DTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_UNORDERED_ACCESS };
        barrier.mSubresourceBarrier = true;
        barrier.mMipLevel = i;
        cmdResourceBarrier(cmd, 0, NULL, 1, &barrier, 0, NULL);
    }

    for (uint32_t i = 1; i < LowFreqMipCount; i++, ++setIndex)
    {
        DescriptorData mipParams[2] = {};
        mipParams[0].pName = "SrcTexture";
        mipParams[0].ppTextures = &pLowFrequency3DTexture;
        mipParams[0].mUAVMipSlice = (uint16_t)(i - 1);
        mipParams[1].pName = "DstTexture";
        mipParams[1].ppTextures = &pLowFrequency3DTexture;
        mipParams[1].mUAVMipSlice = (uint16_t)i;
        updateDescriptorSet(pRenderer, setIndex, pGenMipmapDescriptorSet, 2, mipParams);
        cmdBindDescriptorSet(cmd, setIndex, pGenMipmapDescriptorSet);

        data.mipSize = gLowFreq3DTextureSize >> i;
        cmdBindPushConstants(cmd, pGenMipmapRootSignature, rootConstantIndex, &data);
        cmdDispatch(cmd, max(1u, data.mipSize / pThreadGroupSize[0]), max(1u, data.mipSize / pThreadGroupSize[1]),
                    max(1u, data.mipSize / pThreadGroupSize[2]));

        TextureBarrier barrier = { pLowFrequency3DTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_UNORDERED_ACCESS };
        barrier.mSubresourceBarrier = true;
        barrier.mMipLevel = i;
        cmdResourceBarrier(cmd, 0, NULL, 1, &barrier, 0, NULL);
    }

    TextureBarrier barriersForNoise2[] = {
        { pHighFrequency3DTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
        { pLowFrequency3DTexture, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE },
    };
    cmdResourceBarrier(cmd, 0, NULL, 2, barriersForNoise2, 0, NULL);

    endCmd(cmd);

    QueueSubmitDesc submitDesc = {};
    submitDesc.mCmdCount = 1;
    submitDesc.ppCmds = &cmd;
    submitDesc.mSubmitDone = true;
    queueSubmit(pGraphicsQueue, &submitDesc);
    waitQueueIdle(pGraphicsQueue);

    removeGpuCmdRing(pRenderer, &preprocessRing);
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
}
