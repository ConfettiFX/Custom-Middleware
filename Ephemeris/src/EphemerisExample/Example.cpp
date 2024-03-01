/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

// Ephemeris BEGIN
#include "../../Sky/src/Sky.h"
#include "../../SpaceObjects/src/SpaceObjects.h"
#include "../../Terrain/src/Terrain.h"
#include "../../VolumetricClouds/src/VolumetricClouds.h"
#include "../../src/AppSettings.h"
// Ephemeris END

// Interfaces
#include "../../../../The-Forge/Common_3/Application/Interfaces/IApp.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/ICameraController.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IFont.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IInput.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IProfiler.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IScreenshot.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IUI.h"
#include "../../../../The-Forge/Common_3/Game/Interfaces/IScripting.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/ILog.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/ITime.h"

// Renderer
#include "../../../../The-Forge/Common_3/Graphics/Interfaces/IGraphics.h"
#include "../../../../The-Forge/Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"

#include "../../../../The-Forge/Common_3/Utilities/RingBuffer.h"

// Math
#include "../../../../The-Forge/Common_3/Utilities/Math/MathTypes.h"

// Memory
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/IMemory.h"

#define FOREACH_SETTING(X)            \
    X(InsufficientBindlessEntries, 0) \
    X(QualitySettings, 2)

#define GENERATE_ENUM(x, y)   x,
#define GENERATE_STRING(x, y) #x,
#define GENERATE_STRUCT(x, y) uint32_t m##x = y;

typedef enum ESettings
{
    FOREACH_SETTING(GENERATE_ENUM) Count
} ESettings;

const char* gSettingNames[] = { FOREACH_SETTING(GENERATE_STRING) };

// Useful for using names directly instead of subscripting an array
struct ConfigSettings
{
    FOREACH_SETTING(GENERATE_STRUCT)
} gGpuSettings;

extern AppSettings  gAppSettings;
extern UIComponent* pGuiSkyWindow;
extern UIComponent* pGuiCloudWindow;

// #NOTE: Two sets of resources (one in flight and one being used on CPU)
const uint32_t gDataBufferCount = 2;

Renderer* pRenderer = NULL;

Queue*     pGraphicsQueue = NULL;
GpuCmdRing gGraphicsCmdRing = {};

SwapChain*    pSwapChain = NULL;
RenderTarget* pDepthBuffer = NULL;

RenderTarget* pLinearDepthBuffer = NULL;

Shader*   pLinearDepthResolveShader = NULL;
Pipeline* pLinearDepthResolvePipeline = NULL;

// Shader*             pLinearDepthCompShader = NULL;
// Pipeline*           pLinearDepthCompPipeline = NULL;
// RootSignature*      pLinearDepthCompRootSignature = NULL;

RenderTarget* pTerrainResultRT = NULL;
RenderTarget* pSkydomeResultRT = NULL;

Shader*   pFXAAShader = NULL;
Pipeline* pFXAAPipeline = NULL;

DescriptorSet* pExampleDescriptorSet = NULL;
RootSignature* pExampleRootSignature = NULL;
uint32_t       gExampleRootConstantIndex = 0;

Sampler* pBilinearClampSampler = NULL;
Sampler* pNearestClampSampler = NULL;

Semaphore* pImageAcquiredSemaphore = NULL;

ProfileToken gGpuProfileToken;
UIComponent* pMainGuiWindow = NULL;

Buffer* pTransmittanceBuffer = NULL;

static uint gPrevFrameIndex = 0;

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
    vec2  ScreenSize;
    float Use;
    float Time;
};

LuaManager gLuaManager;
bool       gLuaUpdateScriptRunning = false;

FXAAINFO gFXAAinfo;

uint32_t gFrameIndex = 0;

ICameraController* pCameraController = NULL;

VolumetricClouds gVolumetricClouds;
Terrain          gTerrain;
Sky              gSky;
SpaceObjects     gSpaceObjects;

FontDrawDesc gFrameTimeDraw;
FontDrawDesc gDefaultTextDrawDesc;
uint32_t     gFontID = 0;

const char*    pPipelineCacheName = "PipelineCache.cache";
PipelineCache* pPipelineCache = NULL;

static HiresTimer gTimer;

void setDefaultQualitySettings();
void useLowQualitySettings(void* pUserData);
void useMediumQualitySettings(void* pUserData);
void useHighQualitySettings(void* pUserData);
void useUltraQualitySettings(void* pUserData);
void setGroundCamera(void* pUserData);
void setSpaceCamera(void* pUserData);
void runUpdateScript(void* pUserData);
void toggleAdvancedUI(void* pUserData);
Quat computeQuaternionFromLookAt(vec3 lookDir);

class RenderEphemeris: public IApp
{
public:
    bool Init()
    {
        initHiresTimer(&gTimer);

        // FILE PATHS
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "CompiledShaders");
        fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_PIPELINE_CACHE, "PipelineCaches");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_GPU_CONFIG, "GPUCfg");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_TEXTURES, "Textures");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_MESHES, "Meshes");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_FONTS, "Fonts");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SCRIPTS, "Scripts");
        fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_SCREENSHOTS, "Screenshots");
        fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_DEBUG, "Debug");

        CameraMotionParameters cmp{ 16000.0f, 60000.0f, 20000.0f };

        float h = 6000.0f;

        vec3 camPos{ 0.0f, h, -10.0f };
        vec3 lookAt{ 0.0f, h, 1.0f };

        pCameraController = initFpsCameraController(camPos, lookAt);
        pCameraController->setMotionParameters(cmp);

        ExtendedSettings extendedSettings = {};
        extendedSettings.mNumSettings = ESettings::Count;
        extendedSettings.pSettings = (uint32_t*)&gGpuSettings;
        extendedSettings.ppSettingNames = gSettingNames;

        RendererDesc settings;
        memset(&settings, 0, sizeof(settings));
        settings.pExtendedSettings = &extendedSettings;
        initRenderer(GetName(), &settings, &pRenderer);

        setDefaultQualitySettings();

        // check for init success
        if (!pRenderer)
            return false;

        if (gGpuSettings.mInsufficientBindlessEntries)
        {
            ShowUnsupportedMessage("Ephemeris does not run on this device. GPU does not support enough bindless texture entries");
            return false;
        }

        // set lua functions and capture cameraController
        initLuaManager();

        QueueDesc queueDesc = {};
        queueDesc.mType = QUEUE_TYPE_GRAPHICS;
        queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
        addQueue(pRenderer, &queueDesc, &pGraphicsQueue);

        GpuCmdRingDesc cmdRingDesc = {};
        cmdRingDesc.pQueue = pGraphicsQueue;
        cmdRingDesc.mPoolCount = gDataBufferCount;
        cmdRingDesc.mCmdPerPoolCount = 1;
        cmdRingDesc.mAddSyncPrimitives = true;
        addGpuCmdRing(pRenderer, &cmdRingDesc, &gGraphicsCmdRing);

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

        InputSystemDesc inputDesc = {};
        inputDesc.pRenderer = pRenderer;
        inputDesc.pWindow = pWindow;
        if (!initInputSystem(&inputDesc))
            return false;

        UIComponentDesc UIComponentDesc = {};
        float           dpiScale[2];
        const uint32_t  monitorIdx = getActiveMonitorIdx();
        getMonitorDpiScale(monitorIdx, dpiScale);

        UIComponentDesc.mStartPosition = vec2(260.0f / dpiScale[0], 80.0f / dpiScale[1]);
        UIComponentDesc.mStartSize = vec2(300.0f / dpiScale[0], 250.0f / dpiScale[1]);
        UIComponentDesc.mFontID = 0;
        UIComponentDesc.mFontSize = 16.0f;
        uiCreateComponent("Global Settings", &UIComponentDesc, &pMainGuiWindow);

        // Initialize micro profiler and its UI.
        ProfilerDesc profiler = {};
        profiler.pRenderer = pRenderer;
        profiler.mWidthUI = mSettings.mWidth;
        profiler.mHeightUI = mSettings.mHeight;
        initProfiler(&profiler);

        gGpuProfileToken = addGpuProfiler(pRenderer, pGraphicsQueue, "GpuProfiler");

#if defined(METAL)
        {
            float          dpiScale[2];
            const uint32_t monitorIdx = getActiveMonitorIdx();
            getMonitorDpiScale(monitorIdx, dpiScale);
            gFrameTimeDraw.mFontSize /= dpiScale[1];
            gFrameTimeDraw.mFontID = gFontID;
            gDefaultTextDrawDesc.mFontSize /= dpiScale[1];
        }
#endif
        // Add TransmittanceBuffer buffer
        BufferLoadDesc TransBufferDesc = {};
        TransBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_RW_BUFFER | DESCRIPTOR_TYPE_BUFFER;
        TransBufferDesc.mDesc.mElementCount = 3;
        TransBufferDesc.mDesc.mStructStride = sizeof(float4);
        TransBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        TransBufferDesc.mDesc.mSize = TransBufferDesc.mDesc.mStructStride * TransBufferDesc.mDesc.mElementCount;
        TransBufferDesc.mDesc.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
        TransBufferDesc.mDesc.pName = "Transmittance Buffer";
        // TransBufferDesc.pData = gInitializeVal.data();
        TransBufferDesc.ppBuffer = &pTransmittanceBuffer;
        addResource(&TransBufferDesc, NULL);

        gTerrain.Initialize(pCameraController, gGpuProfileToken);
        gTerrain.Init(pRenderer, pPipelineCache);

        gSky.Initialize(pCameraController, gGpuProfileToken, pTransmittanceBuffer);
        gSky.Init(pRenderer, pPipelineCache);

        gVolumetricClouds.Initialize(pCameraController, pGraphicsQueue, gGpuProfileToken, pTransmittanceBuffer);
        gVolumetricClouds.Init(pRenderer, pPipelineCache);
        gTerrain.pWeatherMap = gVolumetricClouds.GetWeatherMap();

        gSpaceObjects.Initialize(pCameraController, gGpuProfileToken, pTransmittanceBuffer);
        gSpaceObjects.Init(pRenderer, pPipelineCache);

        SamplerDesc samplerClampDesc = { FILTER_LINEAR,
                                         FILTER_LINEAR,
                                         MIPMAP_MODE_LINEAR,
                                         ADDRESS_MODE_CLAMP_TO_EDGE,
                                         ADDRESS_MODE_CLAMP_TO_EDGE,
                                         ADDRESS_MODE_CLAMP_TO_EDGE };
        addSampler(pRenderer, &samplerClampDesc, &pBilinearClampSampler);

        samplerClampDesc = { FILTER_NEAREST,
                             FILTER_NEAREST,
                             MIPMAP_MODE_NEAREST,
                             ADDRESS_MODE_CLAMP_TO_EDGE,
                             ADDRESS_MODE_CLAMP_TO_EDGE,
                             ADDRESS_MODE_CLAMP_TO_EDGE };
        addSampler(pRenderer, &samplerClampDesc, &pNearestClampSampler);

        SliderUintWidget downsampling;
        downsampling.pData = &gAppSettings.DownSampling;
        downsampling.mMin = 1;
        downsampling.mMax = 3;
        downsampling.mStep = 1;

        static const uint32_t maxWidgets = 64;
        uint32_t              widgetsCount = 0;
        UIWidget              collapseWidgets[maxWidgets] = {};
        UIWidget*             pCollapseWidgets[maxWidgets];
        for (uint32_t i = 0; i < maxWidgets; ++i)
            pCollapseWidgets[i] = &collapseWidgets[i];

        // -------------- Quality Settings
        LabelWidget presetTitle;
        UIWidget*   pPresetTitle = uiCreateComponentWidget(pMainGuiWindow, "Presets:", &presetTitle, WIDGET_TYPE_LABEL);
        luaRegisterWidget(pPresetTitle);

        ButtonWidget UseLowQuality;
        UIWidget*    pUseLowQuality = uiCreateComponentWidget(pMainGuiWindow, "Low", &UseLowQuality, WIDGET_TYPE_BUTTON);
        uiSetWidgetOnEditedCallback(pUseLowQuality, nullptr, useLowQualitySettings);
        luaRegisterWidget(pUseLowQuality);

        ButtonWidget UseMiddleQuality;
        UIWidget*    pUseMiddleQuality = uiCreateComponentWidget(pMainGuiWindow, "Medium", &UseMiddleQuality, WIDGET_TYPE_BUTTON);
        uiSetWidgetOnEditedCallback(pUseMiddleQuality, nullptr, useMediumQualitySettings);
        uiSetSameLine(pUseMiddleQuality, true);
        luaRegisterWidget(pUseMiddleQuality);

        ButtonWidget UseHighQuality;
        UIWidget*    pUseHighQuality = uiCreateComponentWidget(pMainGuiWindow, "High", &UseHighQuality, WIDGET_TYPE_BUTTON);
        uiSetWidgetOnEditedCallback(pUseHighQuality, nullptr, useHighQualitySettings);
        uiSetSameLine(pUseHighQuality, true);
        luaRegisterWidget(pUseHighQuality);

        ButtonWidget UseUltraQuality;
        UIWidget*    pUseUltraQuality = uiCreateComponentWidget(pMainGuiWindow, "Ultra", &UseUltraQuality, WIDGET_TYPE_BUTTON);
        uiSetWidgetOnEditedCallback(pUseUltraQuality, nullptr, useUltraQualitySettings);
        uiSetSameLine(pUseUltraQuality, true);
        luaRegisterWidget(pUseUltraQuality);

        ButtonWidget CameraGround;
        UIWidget*    pCameraGroundWidget = uiCreateComponentWidget(pMainGuiWindow, "Ground", &CameraGround, WIDGET_TYPE_BUTTON);
        uiSetWidgetOnEditedCallback(pCameraGroundWidget, nullptr, setGroundCamera);
        luaRegisterWidget(pCameraGroundWidget);

        ButtonWidget CameraSpace;
        UIWidget*    pCameraSpace = uiCreateComponentWidget(pMainGuiWindow, "Space", &CameraSpace, WIDGET_TYPE_BUTTON);
        uiSetWidgetOnEditedCallback(pCameraSpace, nullptr, setSpaceCamera);
        uiSetSameLine(pCameraSpace, true);
        luaRegisterWidget(pCameraSpace);

        DropdownWidget ddCameraScripts;
        ddCameraScripts.pData = &gAppSettings.gCurrentScriptIndex;
        ddCameraScripts.pNames = gCameraScripts;
        ddCameraScripts.mCount = CAMERA_SCRIPT_COUNTS;

        luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Test Scripts", &ddCameraScripts, WIDGET_TYPE_DROPDOWN));

        ButtonWidget bRunScript;
        UIWidget*    pRunScript = uiCreateComponentWidget(pMainGuiWindow, "Run", &bRunScript, WIDGET_TYPE_BUTTON);
        uiSetWidgetOnEditedCallback(pRunScript, nullptr, runUpdateScript);
        luaRegisterWidget(pRunScript);

        // -------------- Light Settings
        widgetsCount = 0;
        SliderFloatWidget AzimuthSliderFloat;
        AzimuthSliderFloat.pData = &gAppSettings.SunDirection.x;
        AzimuthSliderFloat.mMin = -180.0f;
        AzimuthSliderFloat.mMax = 180.0f;
        AzimuthSliderFloat.mStep = 0.001f;
        pCollapseWidgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
        strcpy(pCollapseWidgets[widgetsCount]->mLabel, "Light Azimuth");
        pCollapseWidgets[widgetsCount]->pWidget = &AzimuthSliderFloat;
        ++widgetsCount;

        SliderFloatWidget ElevationSliderFloat;
        ElevationSliderFloat.pData = &gAppSettings.SunDirection.y;
        ElevationSliderFloat.mMin = 0.0f;
        ElevationSliderFloat.mMax = 360.0f;
        ElevationSliderFloat.mStep = 0.001f;
        pCollapseWidgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
        strcpy(pCollapseWidgets[widgetsCount]->mLabel, "Light Elevation");
        pCollapseWidgets[widgetsCount]->pWidget = &ElevationSliderFloat;
        ++widgetsCount;

        CheckboxWidget sunMoveCheckbox;
        sunMoveCheckbox.pData = &gAppSettings.bSunMove;
        pCollapseWidgets[widgetsCount]->mType = WIDGET_TYPE_CHECKBOX;
        strcpy(pCollapseWidgets[widgetsCount]->mLabel, "Automatic Sun Moving");
        pCollapseWidgets[widgetsCount]->pWidget = &sunMoveCheckbox;
        ++widgetsCount;

        CollapsingHeaderWidget collapsingLight;
        collapsingLight.pGroupedWidgets = pCollapseWidgets;
        collapsingLight.mWidgetsCount = widgetsCount;
        collapsingLight.mCollapsed = true;

        luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Sun settings", &collapsingLight, WIDGET_TYPE_COLLAPSING_HEADER));

        // -------------- Atmospheric Scattering Settings
        widgetsCount = 0;
        SliderFloatWidget inscatterIntensity;
        inscatterIntensity.pData = &gAppSettings.SkyInfo.y;
        inscatterIntensity.mMin = 0.0f;
        inscatterIntensity.mMax = 3.0f;
        inscatterIntensity.mStep = 0.001f;
        pCollapseWidgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
        strcpy(pCollapseWidgets[widgetsCount]->mLabel, "InscatterIntensity");
        pCollapseWidgets[widgetsCount]->pWidget = &inscatterIntensity;
        ++widgetsCount;

        SliderFloatWidget inscatterDepthFallOff;
        inscatterDepthFallOff.pData = &gAppSettings.SkyInfo.z;
        inscatterDepthFallOff.mMin = 0.005f;
        inscatterDepthFallOff.mMax = 1.0f;
        inscatterDepthFallOff.mStep = 0.001f;
        pCollapseWidgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
        strcpy(pCollapseWidgets[widgetsCount]->mLabel, "InScatterDepthFallOff");
        pCollapseWidgets[widgetsCount]->pWidget = &inscatterDepthFallOff;
        ++widgetsCount;

        CollapsingHeaderWidget collapsingAtmo;
        collapsingAtmo.pGroupedWidgets = pCollapseWidgets;
        collapsingAtmo.mWidgetsCount = widgetsCount;
        collapsingAtmo.mCollapsed = false;

        luaRegisterWidget(
            uiCreateComponentWidget(pMainGuiWindow, "Atmosphere scattering setting", &collapsingAtmo, WIDGET_TYPE_COLLAPSING_HEADER));

        // -------------- Cloud Settings
        widgetsCount = 0;
        SliderFloatWidget weatherTextDistance;
        weatherTextDistance.pData = &gAppSettings.WeatherTextureDistance;
        weatherTextDistance.mMin = -300000.0f;
        weatherTextDistance.mMax = 300000.0f;
        weatherTextDistance.mStep = 0.01f;
        pCollapseWidgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
        strcpy(pCollapseWidgets[widgetsCount]->mLabel, "Weather Texture Distance");
        pCollapseWidgets[widgetsCount]->pWidget = &weatherTextDistance;
        ++widgetsCount;

        SliderFloatWidget cloudCoverageModifier;
        cloudCoverageModifier.pData = &gAppSettings.m_CloudCoverageModifier_2nd;
        cloudCoverageModifier.mMin = -1.0f;
        cloudCoverageModifier.mMax = 1.0f;
        cloudCoverageModifier.mStep = 0.001f;
        pCollapseWidgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
        strcpy(pCollapseWidgets[widgetsCount]->mLabel, "Cloud Coverage Modifier");
        pCollapseWidgets[widgetsCount]->pWidget = &cloudCoverageModifier;
        ++widgetsCount;

        SliderFloatWidget maxSampleDistance;
        maxSampleDistance.pData = &gAppSettings.m_DefaultMaxSampleDistance;
        maxSampleDistance.mMin = 200000.0f;
        maxSampleDistance.mMax = 2000000.0f;
        maxSampleDistance.mStep = 32.0f;
        pCollapseWidgets[widgetsCount]->mType = WIDGET_TYPE_SLIDER_FLOAT;
        strcpy(pCollapseWidgets[widgetsCount]->mLabel, "Cutt off distance");
        pCollapseWidgets[widgetsCount]->pWidget = &maxSampleDistance;
        ++widgetsCount;

        CheckboxWidget enableBlur;
        enableBlur.pData = &gAppSettings.m_EnableBlur;
        pCollapseWidgets[widgetsCount]->mType = WIDGET_TYPE_CHECKBOX;
        strcpy(pCollapseWidgets[widgetsCount]->mLabel, "Enabled Edge Blur");
        pCollapseWidgets[widgetsCount]->pWidget = &enableBlur;
        ++widgetsCount;

        CollapsingHeaderWidget collapsingCloud;
        collapsingCloud.pGroupedWidgets = pCollapseWidgets;
        collapsingCloud.mWidgetsCount = widgetsCount;
        ;
        collapsingCloud.mCollapsed = false;

        luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Cloud setting", &collapsingCloud, WIDGET_TYPE_COLLAPSING_HEADER));

        // -------------- Advanced

        SeparatorWidget separator;
        luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "", &separator, WIDGET_TYPE_SEPARATOR));

        CheckboxWidget advancedWindowCheckbox;
        advancedWindowCheckbox.pData = &gAppSettings.gShowAdvancedWindows;
        UIWidget* advancedWindowWidget =
            uiCreateComponentWidget(pMainGuiWindow, "Toggle advanced windows", &advancedWindowCheckbox, WIDGET_TYPE_CHECKBOX);
        uiSetWidgetOnEditedCallback(advancedWindowWidget, nullptr, toggleAdvancedUI);
        luaRegisterWidget(advancedWindowWidget);

        CheckboxWidget realTimeCheckBox;
        realTimeCheckBox.pData = &gAppSettings.TemporalFilteringEnabled;
        luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Enable temporal filtering", &realTimeCheckBox, WIDGET_TYPE_CHECKBOX));

        CheckboxWidget enableFXAACheckbox;
        enableFXAACheckbox.pData = &gAppSettings.gToggleFXAA;
        luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Enable FXAA", &enableFXAACheckbox, WIDGET_TYPE_CHECKBOX));

        CheckboxWidget showInterfaceCheckbox;
        showInterfaceCheckbox.pData = &gAppSettings.gShowInterface;
        luaRegisterWidget(uiCreateComponentWidget(pMainGuiWindow, "Show Interface (F2)", &showInterfaceCheckbox, WIDGET_TYPE_CHECKBOX));

        // App Actions
        InputActionDesc actionDesc = { DefaultInputActions::DUMP_PROFILE_DATA,
                                       [](InputActionContext* ctx)
                                       {
                                           dumpProfileData(((Renderer*)ctx->pUserData)->pName);
                                           return true;
                                       },
                                       pRenderer };
        addInputAction(&actionDesc);
        actionDesc = { DefaultInputActions::TOGGLE_FULLSCREEN,
                       [](InputActionContext* ctx)
                       {
                           WindowDesc* winDesc = ((IApp*)ctx->pUserData)->pWindow;
                           if (winDesc->fullScreen)
                               winDesc->borderlessWindow
                                   ? setBorderless(winDesc, getRectWidth(&winDesc->clientRect), getRectHeight(&winDesc->clientRect))
                                   : setWindowed(winDesc, getRectWidth(&winDesc->clientRect), getRectHeight(&winDesc->clientRect));
                           else
                               setFullscreen(winDesc);
                           return true;
                       },
                       this };
        addInputAction(&actionDesc);
        actionDesc = { DefaultInputActions::UI_KEY_F2,
                       [](InputActionContext* ctx)
                       {
                           gAppSettings.gShowInterface = !gAppSettings.gShowInterface;
                           return true;
                       },
                       this };
        addInputAction(&actionDesc);
        actionDesc = { DefaultInputActions::EXIT, [](InputActionContext* ctx)
                       {
                           requestShutdown();
                           return true;
                       } };
        addInputAction(&actionDesc);
        InputActionCallback onUIInput = [](InputActionContext* ctx)
        {
            if (ctx->mActionId > UISystemInputActions::UI_ACTION_START_ID_)
            {
                uiOnInput(ctx->mActionId, ctx->mBool, ctx->pPosition, &ctx->mFloat2);
            }
            return true;
        };

        typedef bool (*CameraInputHandler)(InputActionContext * ctx, uint32_t index);
        static CameraInputHandler onCameraInput = [](InputActionContext* ctx, uint32_t index)
        {
            if (*(ctx->pCaptured))
            {
                float2 delta = uiIsFocused() ? float2(0.f, 0.f) : ctx->mFloat2;
                index ? pCameraController->onRotate(delta) : pCameraController->onMove(delta);
            }
            return true;
        };
        actionDesc = { DefaultInputActions::CAPTURE_INPUT,
                       [](InputActionContext* ctx)
                       {
                           setEnableCaptureInput(!uiIsFocused() && INPUT_ACTION_PHASE_CANCELED != ctx->mPhase);
                           return true;
                       },
                       NULL };
        addInputAction(&actionDesc);
        actionDesc = { DefaultInputActions::ROTATE_CAMERA, [](InputActionContext* ctx) { return onCameraInput(ctx, 1); }, NULL };
        addInputAction(&actionDesc);
        actionDesc = { DefaultInputActions::TRANSLATE_CAMERA, [](InputActionContext* ctx) { return onCameraInput(ctx, 0); }, NULL };
        addInputAction(&actionDesc);
        actionDesc = { DefaultInputActions::RESET_CAMERA, [](InputActionContext* ctx)
                       {
                           if (!uiWantTextInput())
                               pCameraController->resetView();
                           return true;
                       } };
        addInputAction(&actionDesc);
        GlobalInputActionDesc globalInputActionDesc = { GlobalInputActionDesc::ANY_BUTTON_ACTION, onUIInput, this };
        setGlobalInputAction(&globalInputActionDesc);

        toggleAdvancedUI(nullptr);
        gFrameIndex = 0;

        return true;
    }

    void initLuaManager()
    {
        luaDestroyCurrentManager();
        gLuaManager.Init();

        ICameraController* cameraLocalPtr = pCameraController;
        gLuaManager.SetFunction("GetCameraPosition",
                                [cameraLocalPtr](ILuaStateWrap* state) -> int
                                {
                                    vec3 pos = cameraLocalPtr->getViewPosition();
                                    state->PushResultNumber(pos.getX());
                                    state->PushResultNumber(pos.getY());
                                    state->PushResultNumber(pos.getZ());
                                    return 3; // return amount of arguments
                                });
        gLuaManager.SetFunction("SetCameraPosition",
                                [cameraLocalPtr](ILuaStateWrap* state) -> int
                                {
                                    float x = (float)state->GetNumberArg(1); // in Lua indexing starts from 1!
                                    float y = (float)state->GetNumberArg(2);
                                    float z = (float)state->GetNumberArg(3);
                                    cameraLocalPtr->moveTo(vec3(x, y, z));
                                    return 0; // return amount of arguments
                                });
        gLuaManager.SetFunction("SetCameraLookAt",
                                [cameraLocalPtr](ILuaStateWrap* state) -> int
                                {
                                    float x = (float)state->GetNumberArg(1);
                                    float y = (float)state->GetNumberArg(2);
                                    float z = (float)state->GetNumberArg(3);
                                    cameraLocalPtr->lookAt(vec3(x, y, z));
                                    return 0; // return amount of arguments
                                });
        gLuaManager.SetFunction(
            "AnimateCamera",
            [cameraLocalPtr](ILuaStateWrap* state) -> int
            {
                vec3  cameraPos0 = vec3((float)state->GetNumberArg(1), (float)state->GetNumberArg(2), (float)state->GetNumberArg(3));
                vec3  lookAt0 = vec3((float)state->GetNumberArg(4), (float)state->GetNumberArg(5), (float)state->GetNumberArg(6));
                vec3  cameraPos1 = vec3((float)state->GetNumberArg(7), (float)state->GetNumberArg(8), (float)state->GetNumberArg(9));
                vec3  lookAt1 = vec3((float)state->GetNumberArg(10), (float)state->GetNumberArg(11), (float)state->GetNumberArg(12));
                float lerpCoef = (float)state->GetNumberArg(13);

                // compute updated position
                vec3 currentPosition = lerp(lerpCoef, cameraPos0, cameraPos1);
                // compute updated rotation matrix
                vec3 prevLookDir = normalize(lookAt0 - cameraPos0);
                vec3 nextLookDir = normalize(lookAt1 - cameraPos1);

                Quat startQuat = computeQuaternionFromLookAt(prevLookDir);
                Quat endQuat = computeQuaternionFromLookAt(nextLookDir);
                Quat deltaQuat = inverse(startQuat) * endQuat;

                Quat identity = Quat::identity();
                Quat currentQuat = startQuat * lerp(lerpCoef, identity, deltaQuat);
                vec3 currentLookDir = rotate(currentQuat, vec3(0.0f, 0.0f, 1.0f));
                vec3 currentlookAt = currentPosition + currentLookDir * 1000.0f;
                cameraLocalPtr->moveTo(currentPosition);
                cameraLocalPtr->lookAt(currentlookAt);
                return 0;
            });
        gLuaManager.SetFunction("AnimateSun",
                                [](ILuaStateWrap* state) -> int
                                {
                                    float prevAzimuth = (float)state->GetNumberArg(1);
                                    float prevElevation = (float)state->GetNumberArg(2);
                                    float nextAzimuth = (float)state->GetNumberArg(3);
                                    float nextElevation = (float)state->GetNumberArg(4);
                                    float lerpCoef = (float)state->GetNumberArg(5);

                                    // compute updated position
                                    gAppSettings.SunDirection.x = prevAzimuth + lerpCoef * (nextAzimuth - prevAzimuth);
                                    gAppSettings.SunDirection.y = prevElevation + lerpCoef * (nextElevation - prevElevation);
                                    return 0;
                                });
        gLuaManager.SetFunction("AnimateCloud",
                                [](ILuaStateWrap* state) -> int
                                {
                                    float prevDistance = (float)state->GetNumberArg(1);
                                    float nextDistance = (float)state->GetNumberArg(2);
                                    float lerpCoef = (float)state->GetNumberArg(3);

                                    // compute updated position
                                    gAppSettings.WeatherTextureDistance = prevDistance + lerpCoef * (nextDistance - prevDistance);
                                    return 0;
                                });
        gLuaManager.SetFunction("StopCurrentScript",
                                [](ILuaStateWrap* state) -> int
                                {
                                    gLuaUpdateScriptRunning = false;
                                    return 0;
                                });
        gLuaManager.SetFunction("ResetFirstFrame",
                                [](ILuaStateWrap* state) -> int
                                {
                                    gAppSettings.m_FirstFrame = true;
                                    return 0;
                                });

        luaAssignCustomManager(&gLuaManager);
    }

    void Exit()
    {
        gLuaManager.Exit();

        exitCameraController(pCameraController);
        exitInputSystem();

        gFrameIndex = 0;
        gPrevFrameIndex = 0;
        gSpaceObjects.Exit();
        gVolumetricClouds.Exit();
        gSky.Exit();
        gTerrain.Exit();
        exitProfiler();

        removeResource(pTransmittanceBuffer);
        removeSampler(pRenderer, pBilinearClampSampler);
        removeSampler(pRenderer, pNearestClampSampler);

        exitUserInterface();

        exitFontSystem();

        removeSemaphore(pRenderer, pImageAcquiredSemaphore);
        removeGpuCmdRing(pRenderer, &gGraphicsCmdRing);

        removeQueue(pRenderer, pGraphicsQueue);

        PipelineCacheSaveDesc saveDesc;
        saveDesc.pFileName = pPipelineCacheName;
        savePipelineCache(pRenderer, pPipelineCache, &saveDesc);
        removePipelineCache(pRenderer, pPipelineCache);

        exitResourceLoaderInterface(pRenderer);

        exitRenderer(pRenderer);
        pRenderer = NULL;
    }

    bool Load(ReloadDesc* pReloadDesc)
    {
        if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
        {
            addShaders();
            addRootSignatures();
            addDescriptorSets();
        }

        if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
        {
            // set frame index to 0
            gFrameIndex = 0;

            if (!addSwapChain())
                return false;

            if (!addDepthBuffer())
                return false;

            if (pReloadDesc->mType & RELOAD_TYPE_RESIZE)
            {
                gTerrain.Load(mSettings.mWidth, mSettings.mHeight);
                gSky.Load(mSettings.mWidth, mSettings.mHeight);
                gVolumetricClouds.Load(mSettings.mWidth, mSettings.mHeight);
                gSpaceObjects.Load(mSettings.mWidth, mSettings.mHeight);
            }

            gTerrain.InitializeWithLoad(pDepthBuffer);
            gSky.InitializeWithLoad(pDepthBuffer, pLinearDepthBuffer);
            gVolumetricClouds.InitializeWithLoad(pLinearDepthBuffer, pDepthBuffer);
            gSpaceObjects.InitializeWithLoad(pDepthBuffer, pLinearDepthBuffer, gVolumetricClouds.pHighResCloudTexture,
                                             gSky.GetParticleVertexBuffer(), gSky.GetParticleInstanceBuffer(), gSky.GetParticleCount(),
                                             sizeof(float) * 6, sizeof(ParticleData));

            addRenderTargets();
        }

        if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
        {
            addPipelines();
        }

        waitForAllResourceLoads();

        prepareDescriptorSets();

        UserInterfaceLoadDesc uiLoad = {};
        uiLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
        uiLoad.mHeight = mSettings.mHeight;
        uiLoad.mWidth = mSettings.mWidth;
        uiLoad.mLoadType = pReloadDesc->mType;
        loadUserInterface(&uiLoad);

        FontSystemLoadDesc fontLoad = {};
        fontLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
        fontLoad.mHeight = mSettings.mHeight;
        fontLoad.mWidth = mSettings.mWidth;
        fontLoad.mLoadType = pReloadDesc->mType;
        loadFontSystem(&fontLoad);

        initScreenshotInterface(pRenderer, pGraphicsQueue);

        return true;
    }

    void Unload(ReloadDesc* pReloadDesc)
    {
        waitQueueIdle(pGraphicsQueue);

        unloadFontSystem(pReloadDesc->mType);
        unloadUserInterface(pReloadDesc->mType);

        if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
        {
            removePipelines();
        }

        if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
        {
            removeSwapChain(pRenderer, pSwapChain);

            removeRenderTargets();
            gSky.Unload();
            gVolumetricClouds.Unload();
            gSpaceObjects.Unload();
        }

        if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
        {
            removeDescriptorSets();
            removeRootSignatures();
            removeShaders();
        }

        exitScreenshotInterface();
    }

    void Update(float deltaTime)
    {
        // Lua
        if (gLuaUpdateScriptRunning)
        {
            gLuaManager.Update(deltaTime);
        }

        updateInputSystem(deltaTime, mSettings.mWidth, mSettings.mHeight);
        /************************************************************************/
        // Input
        /************************************************************************/

        pCameraController->update(deltaTime);
        /************************************************************************/
        // Scene Update
        /************************************************************************/
        static float currentTime = 0.0f;
        currentTime += deltaTime * 1000.0f;

        if (gAppSettings.bSunMove)
        {
            gAppSettings.SunDirection.y += deltaTime * gAppSettings.sunMovingSpeed;

            if (gAppSettings.SunDirection.y < 0.0f)
                gAppSettings.SunDirection.y += 360.0f;

            gAppSettings.SunDirection.y = fmodf(gAppSettings.SunDirection.y, 360.0f);
        }

        float Azimuth = (PI / 180.0f) * gAppSettings.SunDirection.x;
        float Elevation = (PI / 180.0f) * (gAppSettings.SunDirection.y - 180.0f);
        float cosElevation = cosf(Elevation);
        vec3  sunDirection = normalize(vec3(cosf(Azimuth) * cosElevation, sinf(Elevation), sinf(Azimuth) * cosElevation));

        gSky.Azimuth = Azimuth;
        gSky.Elevation = Elevation;
        gSky.LightDirection = v3ToF3(sunDirection);
        gSky.Update(deltaTime);

        gVolumetricClouds.LightDirection = v3ToF3(sunDirection);
        gVolumetricClouds.Update(deltaTime);

        // after gVolumetricClouds.Update because we read back data it computes
        gTerrain.IsEnabledShadow = true;
        gTerrain.volumetricCloudsShadowCB.SettingInfo00 =
            vec4(1.0, gVolumetricClouds.volumetricCloudsCB.m_DataPerLayer[0].CloudCoverage,
                 gVolumetricClouds.volumetricCloudsCB.m_DataPerLayer[0].WeatherTextureSize, 0.0);
        gTerrain.volumetricCloudsShadowCB.StandardPosition = gVolumetricClouds.volumetricCloudsCB.m_DataPerLayer[0].WindDirection;
        gTerrain.volumetricCloudsShadowCB.ShadowInfo = gVolumetricClouds.g_ShadowInfo;
        gTerrain.volumetricCloudsShadowCB.WeatherDisplacement =
            vec4(gVolumetricClouds.volumetricCloudsCB.m_DataPerLayer[0].WeatherTextureOffsetX,
                 gVolumetricClouds.volumetricCloudsCB.m_DataPerLayer[0].WeatherTextureOffsetZ, 0.0f, 0.0f);
        gTerrain.LightDirection = v3ToF3(sunDirection);
        gTerrain.SunColor = gSky.GetSunColor();
        gTerrain.Update(deltaTime);

        gSpaceObjects.Azimuth = Azimuth;
        gSpaceObjects.Elevation = Elevation;
        gSpaceObjects.LightDirection = v3ToF3(sunDirection);
        gSpaceObjects.Update(deltaTime);

        gFXAAinfo.ScreenSize = vec2((float)mSettings.mWidth, (float)mSettings.mHeight);
        gFXAAinfo.Use = gAppSettings.gToggleFXAA ? 1.0f : 0.0f;
        gFXAAinfo.Time = currentTime;
    }

    void Draw()
    {
        if (pSwapChain->mEnableVsync != mSettings.mVSyncEnabled)
        {
            waitQueueIdle(pGraphicsQueue);
            ::toggleVSync(pRenderer, &pSwapChain);
        }

        uint32_t presentIndex = 0;
        acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL, &presentIndex);

        // update camera with time
        // mat4 viewMat = pCameraController->getViewMatrix();

        // Stall if CPU is running "gDataBufferCount" frames ahead of GPU
        GpuCmdRingElement elem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);
        FenceStatus       fenceStatus;
        getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
        if (fenceStatus == FENCE_STATUS_INCOMPLETE)
            waitForFences(pRenderer, 1, &elem.pFence);

        resetCmdPool(pRenderer, elem.pCmdPool);

        Cmd* cmd = elem.pCmds[0];
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
        cameraInfo.nearPlane = CAMERA_NEAR;
        cameraInfo.farPlane = CAMERA_FAR;
        ///////////////////////////////////////////////// Depth Linearization ////////////////////////////////////////////////////
        {
            cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Depth Linearization");

            RenderTargetBarrier barriersLinearDepth[] = { { pLinearDepthBuffer, RESOURCE_STATE_SHADER_RESOURCE,
                                                            RESOURCE_STATE_RENDER_TARGET } };

            cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersLinearDepth);

            BindRenderTargetsDesc bindRenderTargets = {};
            bindRenderTargets.mRenderTargetCount = 1;
            bindRenderTargets.mRenderTargets[0] = { pLinearDepthBuffer, LOAD_ACTION_CLEAR };
            cmdBindRenderTargets(cmd, &bindRenderTargets);

            cmdSetViewport(cmd, 0.0f, 0.0f, (float)pLinearDepthBuffer->mWidth, (float)pLinearDepthBuffer->mHeight, 0.0f, 1.0f);
            cmdSetScissor(cmd, 0, 0, pLinearDepthBuffer->mWidth, pLinearDepthBuffer->mHeight);

            cmdBindPipeline(cmd, pLinearDepthResolvePipeline);
            cmdBindPushConstants(cmd, pExampleRootSignature, gExampleRootConstantIndex, &cameraInfo);
            cmdBindDescriptorSet(cmd, 0, pExampleDescriptorSet);
            cmdDraw(cmd, 3, 0);

            cmdBindRenderTargets(cmd, NULL);

            RenderTargetBarrier barriersLinearDepthEnd[] = { { pLinearDepthBuffer, RESOURCE_STATE_RENDER_TARGET,
                                                               RESOURCE_STATE_SHADER_RESOURCE } };

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
            if (gAppSettings.gToggleFXAA)
                cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "FXAA");
            else
                cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "PresentPipeline");

            pRenderTarget = pSwapChain->ppRenderTargets[presentIndex];

            RenderTargetBarrier barriers[] = {
                { pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET },
            };

            cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

            BindRenderTargetsDesc bindRenderTargets = {};
            bindRenderTargets.mRenderTargetCount = 1;
            bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
            cmdBindRenderTargets(cmd, &bindRenderTargets);
            cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
            cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

            cmdBindPipeline(cmd, pFXAAPipeline);

            cmdBindDescriptorSet(cmd, 1, pExampleDescriptorSet);
            cmdBindPushConstants(cmd, pExampleRootSignature, gExampleRootConstantIndex, &gFXAAinfo);

            cmdDraw(cmd, 3, 0);

            // cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

            cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
        }

        // UI
        {
            cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw UI");

            getHiresTimerUSec(&gTimer, true);

            if (gAppSettings.gShowInterface)
            {
                if (gAppSettings.gTogglePerformance)
                {
                    gFrameTimeDraw.mFontColor = 0xff00ffff;
                    gFrameTimeDraw.mFontSize = 18.0f;
                    gFrameTimeDraw.mFontID = gFontID;
                    cmdDrawCpuProfile(cmd, float2(8.0f, 15.0f), &gFrameTimeDraw);
                    cmdDrawGpuProfile(cmd, float2(8.0f, 100.0f), gGpuProfileToken, &gFrameTimeDraw);
                }

                cmdDrawUserInterface(cmd);
            }

            cmdBindRenderTargets(cmd, NULL);

            RenderTargetBarrier barriers[] = { { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT } };
            cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

            cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
        }

        cmdEndGpuFrameProfile(cmd, gGpuProfileToken);
        endCmd(cmd);

        FlushResourceUpdateDesc flushUpdateDesc = {};
        flushUpdateDesc.mNodeIndex = 0;
        flushResourceUpdates(&flushUpdateDesc);
        Semaphore* waitSemaphores[2] = { flushUpdateDesc.pOutSubmittedSemaphore, pImageAcquiredSemaphore };

        QueueSubmitDesc submitDesc = {};
        submitDesc.mCmdCount = 1;
        submitDesc.mSignalSemaphoreCount = 1;
        submitDesc.mWaitSemaphoreCount = TF_ARRAY_COUNT(waitSemaphores);
        submitDesc.ppCmds = &cmd;
        submitDesc.ppSignalSemaphores = &elem.pSemaphore;
        submitDesc.ppWaitSemaphores = waitSemaphores;
        submitDesc.pSignalFence = elem.pFence;
        queueSubmit(pGraphicsQueue, &submitDesc);
        QueuePresentDesc presentDesc = {};
        presentDesc.mIndex = presentIndex;
        presentDesc.mWaitSemaphoreCount = 1;
        presentDesc.ppWaitSemaphores = &elem.pSemaphore;
        presentDesc.pSwapChain = pSwapChain;
        presentDesc.mSubmitDone = true;
        queuePresent(pGraphicsQueue, &presentDesc);

        flipProfiler();

        // for next frame
        gPrevFrameIndex = gFrameIndex;
        gFrameIndex = (gFrameIndex + 1) % gDataBufferCount;

        if (gVolumetricClouds.AfterSubmit(gPrevFrameIndex))
        {
            waitQueueIdle(pGraphicsQueue);

            gSpaceObjects.Unload();
            gVolumetricClouds.Unload();
            gVolumetricClouds.removeRenderTargets();

            gVolumetricClouds.InitializeWithLoad(pLinearDepthBuffer, pDepthBuffer);
            gVolumetricClouds.Load(mSettings.mWidth, mSettings.mHeight);
            gVolumetricClouds.addRenderTargets();

            gSpaceObjects.InitializeWithLoad(pDepthBuffer, pLinearDepthBuffer, gVolumetricClouds.pHighResCloudTexture,
                                             gSky.GetParticleVertexBuffer(), gSky.GetParticleInstanceBuffer(), gSky.GetParticleCount(),
                                             sizeof(float) * 6, sizeof(ParticleData));
            gSpaceObjects.Load(mSettings.mWidth, mSettings.mHeight);

            gTerrain.pWeatherMap = gVolumetricClouds.GetWeatherMap();

            RenderTarget* ppVolumetricCloudsUsedRTs[2] = { gSky.pSkyRenderTarget, gSky.pSkyRenderTarget };
            gVolumetricClouds.prepareDescriptorSets(ppVolumetricCloudsUsedRTs, 2);
            gSpaceObjects.prepareDescriptorSets(&gSky.pSkyRenderTarget);
        }
    }

    const char* GetName() { return "Ephemeris"; }

    bool addSwapChain()
    {
        SwapChainDesc swapChainDesc = {};
        swapChainDesc.mWindowHandle = pWindow->handle;
        swapChainDesc.mPresentQueueCount = 1;
        swapChainDesc.ppPresentQueues = &pGraphicsQueue;
        swapChainDesc.mWidth = mSettings.mWidth;
        swapChainDesc.mHeight = mSettings.mHeight;
        swapChainDesc.mImageCount = getRecommendedSwapchainImageCount(pRenderer, &pWindow->handle);
        swapChainDesc.mColorFormat = getSupportedSwapchainFormat(pRenderer, &swapChainDesc, COLOR_SPACE_SDR_SRGB);
        swapChainDesc.mColorSpace = COLOR_SPACE_SDR_SRGB;
        swapChainDesc.mEnableVsync = mSettings.mVSyncEnabled;
        ::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

        return pSwapChain != NULL;
    }

    void addDescriptorSets()
    {
        gTerrain.addDescriptorSets();
        gSky.addDescriptorSets();
        gVolumetricClouds.addDescriptorSets();
        gSpaceObjects.addDescriptorSets();

        DescriptorSetDesc setDesc = { pExampleRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 2 };
        addDescriptorSet(pRenderer, &setDesc, &pExampleDescriptorSet);
    }

    void removeDescriptorSets()
    {
        gSpaceObjects.removeDescriptorSets();
        gVolumetricClouds.removeDescriptorSets();
        gSky.removeDescriptorSets();
        gTerrain.removeDescriptorSets();

        removeDescriptorSet(pRenderer, pExampleDescriptorSet);
    }

    void addRootSignatures()
    {
        gTerrain.addRootSignatures();
        gSky.addRootSignatures();
        gVolumetricClouds.addRootSignatures();
        gSpaceObjects.addRootSignatures();

        RootSignatureDesc rootDesc;
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /*rootDesc = {};
        rootDesc = { 0 };
        rootDesc.mShaderCount = 1;
        rootDesc.ppShaders = &pLinearDepthCompShader;

        addRootSignature(pRenderer, &rootDesc, &pLinearDepthCompRootSignature);*/
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////

        const char* pStaticSamplerNames[] = { "g_LinearClamp", "g_NearestClamp" };
        Sampler*    pStaticSamplers[] = { pBilinearClampSampler, pNearestClampSampler };
        Shader*     shaders[] = { pFXAAShader, pLinearDepthResolveShader };
        rootDesc = {};
        rootDesc.mShaderCount = 2;
        rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
        rootDesc.ppStaticSamplers = pStaticSamplers;
        rootDesc.ppShaders = shaders;
        rootDesc.mStaticSamplerCount = 2;
        addRootSignature(pRenderer, &rootDesc, &pExampleRootSignature);
        gExampleRootConstantIndex = getDescriptorIndexFromName(pExampleRootSignature, "ExampleRootConstant");
    }

    void removeRootSignatures()
    {
        gSpaceObjects.removeRootSignatures();
        gVolumetricClouds.removeRootSignatures();
        gSky.removeRootSignatures();
        gTerrain.removeRootSignatures();

        // removeRootSignature(pRenderer, pLinearDepthCompRootSignature);
        removeRootSignature(pRenderer, pExampleRootSignature);
    }

    void addShaders()
    {
        gTerrain.addShaders();
        gSky.addShaders();
        gVolumetricClouds.addShaders();
        gSpaceObjects.addShaders();

        ShaderLoadDesc FXAAShader = {};
        FXAAShader.mStages[0].pFileName = "Triangular.vert";
        FXAAShader.mStages[1].pFileName = "FXAA.frag";
        addShader(pRenderer, &FXAAShader, &pFXAAShader);

        ShaderLoadDesc depthLinearizationResolveShader = {};
        depthLinearizationResolveShader.mStages[0].pFileName = "Triangular.vert";
        depthLinearizationResolveShader.mStages[1].pFileName = "DepthLinearization.frag";
        addShader(pRenderer, &depthLinearizationResolveShader, &pLinearDepthResolveShader);

        /*ShaderLoadDesc depthLinearizationShader = {};
        depthLinearizationShader.mStages[0] = { "depthLinearization.comp", NULL, 0 };
        addShader(pRenderer, &depthLinearizationShader, &pLinearDepthCompShader);
        */
    }

    void removeShaders()
    {
        gSpaceObjects.removeShaders();
        gVolumetricClouds.removeShaders();
        gSky.removeShaders();
        gTerrain.removeShaders();

        removeShader(pRenderer, pFXAAShader);
        // removeShader(pRenderer, pLinearDepthCompShader);
        removeShader(pRenderer, pLinearDepthResolveShader);
    }

    void addPipelines()
    {
        gTerrain.addPipelines();
        gSky.addPipelines();
        gVolumetricClouds.addPipelines();
        gSpaceObjects.addPipelines();

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

        PipelineDesc LinearDepthResolvePipelineDesc = {};
        LinearDepthResolvePipelineDesc.pCache = pPipelineCache;
        {
            LinearDepthResolvePipelineDesc.mType = PIPELINE_TYPE_GRAPHICS;
            GraphicsPipelineDesc& pipelineSettings = LinearDepthResolvePipelineDesc.mGraphicsDesc;

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
    }

    void removePipelines()
    {
        gSpaceObjects.removePipelines();
        gVolumetricClouds.removePipelines();
        gSky.removePipelines();
        gTerrain.removePipelines();

        removePipeline(pRenderer, pLinearDepthResolvePipeline);
        // removePipeline(pRenderer, pLinearDepthCompPipeline);
        removePipeline(pRenderer, pFXAAPipeline);
    }

    void prepareDescriptorSets()
    {
        gTerrain.prepareDescriptorSets();
        gSky.prepareDescriptorSets(&gTerrain.pTerrainRT);
        RenderTarget* ppVolumetricCloudsUsedRTs[2] = { gSky.pSkyRenderTarget, gSky.pSkyRenderTarget };
        gVolumetricClouds.prepareDescriptorSets(ppVolumetricCloudsUsedRTs, 2);
        gSpaceObjects.prepareDescriptorSets(&gSky.pSkyRenderTarget);

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

    void addRenderTargets()
    {
        gTerrain.addRenderTargets();
        gSky.addRenderTargets();
        gVolumetricClouds.addRenderTargets();

        RenderTargetDesc resultRT = {};
        resultRT.mArraySize = 1;
        resultRT.mDepth = 1;
        resultRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
        resultRT.mFormat = pSwapChain->mFormat;
        resultRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
        resultRT.mSampleCount = SAMPLE_COUNT_1;
        resultRT.mSampleQuality = 0;

        resultRT.mWidth = mSettings.mWidth;
        resultRT.mHeight = mSettings.mHeight;

        addRenderTarget(pRenderer, &resultRT, &pSkydomeResultRT);
    }

    void removeRenderTargets()
    {
        gVolumetricClouds.removeRenderTargets();
        gSky.removeRenderTargets();
        gTerrain.removeRenderTargets();

        removeRenderTarget(pRenderer, pDepthBuffer);
        removeRenderTarget(pRenderer, pLinearDepthBuffer);
        removeRenderTarget(pRenderer, pSkydomeResultRT);
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

void setDefaultQualitySettings()
{
    if (gGpuSettings.mQualitySettings == 0)
    {
        useLowQualitySettings(nullptr);
    }
    else if (gGpuSettings.mQualitySettings == 1)
    {
        useMediumQualitySettings(nullptr);
    }
    else
    {
        useHighQualitySettings(nullptr);
    }
}

void useLowQualitySettings(void* pUserData)
{
    gAppSettings.DownSampling = 2;
    gAppSettings.m_MinSampleCount = 54;
    gAppSettings.m_MaxSampleCount = 148;

    gAppSettings.m_MinStepSize = 500.0f;
    gAppSettings.m_MaxStepSize = 1800.0f;

    gAppSettings.m_CloudSize = 103305.805f;
    gAppSettings.m_DefaultMaxSampleDistance = 500000.0f;
    gAppSettings.m_CloudsLayerStart = 15400.0f;
    gAppSettings.m_LayerThickness = 30800.0f;
    gAppSettings.m_DetailStrength = 0.4f;
    gAppSettings.m_CloudDensity = 4.6f;
    gAppSettings.m_CloudCoverageModifier = 0.42f;
    gAppSettings.m_CloudTypeModifier = 0.3f;

    gAppSettings.m_EnabledShadow = true;
    gAppSettings.m_EnabledGodray = true;
    gAppSettings.m_EnableBlur = true;
    gAppSettings.m_Enabled2ndLayer = false;
    gAppSettings.TemporalFilteringEnabled = true;

    gAppSettings.gToggleFXAA = false;
}

void useMediumQualitySettings(void* pUserData)
{
    gAppSettings.DownSampling = 2;
    gAppSettings.m_MinSampleCount = 64;
    gAppSettings.m_MaxSampleCount = 256;

    gAppSettings.m_MinStepSize = 700.0f;
    gAppSettings.m_MaxStepSize = 1600.0f;

    gAppSettings.m_CloudSize = 103305.805f;
    gAppSettings.m_DefaultMaxSampleDistance = 750000.0f;
    gAppSettings.m_CloudsLayerStart = 8800.0f;
    gAppSettings.m_LayerThickness = 55000.0f;
    gAppSettings.m_DetailStrength = 0.25f;
    gAppSettings.m_CloudDensity = 3.4f;
    gAppSettings.m_CloudCoverageModifier = 0.0f;
    gAppSettings.m_CloudTypeModifier = 0.0f;

    gAppSettings.m_EnabledShadow = true;
    gAppSettings.m_EnabledGodray = true;
    gAppSettings.m_EnableBlur = false;
    gAppSettings.m_Enabled2ndLayer = false;
    gAppSettings.TemporalFilteringEnabled = true;

    gAppSettings.gToggleFXAA = false;
}

void useHighQualitySettings(void* pUserData)
{
    gAppSettings.DownSampling = 1;
    gAppSettings.m_MinSampleCount = 96;
    gAppSettings.m_MaxSampleCount = 384;

    gAppSettings.m_MinStepSize = 700.0f;
    gAppSettings.m_MaxStepSize = 1300.0f;

    gAppSettings.m_CloudSize = 103305.805f;
    gAppSettings.m_DefaultMaxSampleDistance = 1000000.0f;
    gAppSettings.m_CloudsLayerStart = 8800.0f;
    gAppSettings.m_LayerThickness = 55000.0f;
    gAppSettings.m_DetailStrength = 0.25f;
    gAppSettings.m_CloudDensity = 3.4f;
    gAppSettings.m_CloudCoverageModifier = 0.0f;
    gAppSettings.m_CloudTypeModifier = 0.0f;

    gAppSettings.m_EnabledShadow = true;
    gAppSettings.m_EnabledGodray = true;
    gAppSettings.m_EnableBlur = false;
    gAppSettings.m_Enabled2ndLayer = false;
    gAppSettings.TemporalFilteringEnabled = true;

    gAppSettings.gToggleFXAA = false;
}

void useUltraQualitySettings(void* pUserData)
{
    gAppSettings.DownSampling = 1;
    gAppSettings.m_MinSampleCount = 96;
    gAppSettings.m_MaxSampleCount = 384;

    gAppSettings.m_MinStepSize = 700.0f;
    gAppSettings.m_MaxStepSize = 1300.0f;

    gAppSettings.m_CloudSize = 103305.805f;
    gAppSettings.m_DefaultMaxSampleDistance = 1000000.0f;
    gAppSettings.m_CloudsLayerStart = 8800.0f;
    gAppSettings.m_LayerThickness = 55000.0f;
    gAppSettings.m_DetailStrength = 0.25f;
    gAppSettings.m_CloudDensity = 3.4f;
    gAppSettings.m_CloudCoverageModifier = 0.0f;
    gAppSettings.m_CloudTypeModifier = 0.0f;

    gAppSettings.m_EnabledShadow = true;
    gAppSettings.m_EnabledGodray = true;
    gAppSettings.m_EnableBlur = false;
    gAppSettings.m_Enabled2ndLayer = true;
    gAppSettings.TemporalFilteringEnabled = false;

    gAppSettings.gToggleFXAA = false;
}

void setGroundCamera(void* pUserData)
{
    LuaScriptDesc runDesc = {};
    runDesc.pScriptFileName = "GroundCamera.lua";
    luaQueueScriptToRun(&runDesc);
}

void setSpaceCamera(void* pUserData)
{
    LuaScriptDesc runDesc = {};
    runDesc.pScriptFileName = "SpaceCamera.lua";
    luaQueueScriptToRun(&runDesc);
}

void runUpdateScript(void* pUserData)
{
    gLuaUpdateScriptRunning = gLuaManager.SetUpdatableScript(gCameraScripts[gAppSettings.gCurrentScriptIndex], "Update", "Exit");
}

void toggleAdvancedUI(void* pUserData)
{
    if (pGuiCloudWindow)
    {
        pGuiCloudWindow->mActive = gAppSettings.gShowAdvancedWindows;
        pGuiSkyWindow->mActive = gAppSettings.gShowAdvancedWindows;
    }
}

Quat computeQuaternionFromLookAt(vec3 lookDir)
{
    float2 viewRotation;
    float  y = lookDir.getY();
    viewRotation.setX(asinf(y));

    float x = lookDir.getX();
    float z = lookDir.getZ();
    float n = sqrtf((x * x) + (z * z));
    // don't change the Y rotation if we're too close to vertical
    if (n > 0.01f)
    {
        x /= n;
        z /= n;
        // by default we are looking at the z positive, this will change the atan2 order
        // image looking from the top to your unit circle made with the default camera direction
        //    z correspond to the default unit circle direction, the x of the arctan function
        //    -x, "left side" of z axis equal the y axis of the arctan function
        viewRotation.setY(atan2f(-x, z));
    }

    float roll = 0.0f;
    float pitch = -viewRotation.getX();
    float yaw = -viewRotation.getY();

    float cr = cos(roll * 0.5f);
    float sr = sin(roll * 0.5f);
    float cp = cos(pitch * 0.5f);
    float sp = sin(pitch * 0.5f);
    float cy = cos(yaw * 0.5f);
    float sy = sin(yaw * 0.5f);

    // euler to quaternion
    Quat result;
    result.setW(cr * cp * cy + sr * sp * sy);
    result.setX(cr * sp * cy + sr * cp * sy);
    result.setY(cr * cp * sy - sr * sp * cy);
    result.setZ(sr * cp * cy - cr * sp * sy);

    return result;
}

DEFINE_APPLICATION_MAIN(RenderEphemeris)
