/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#pragma once

#include "../../../../The-Forge/Common_3/Application/Interfaces/ICameraController.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IMiddleware.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IProfiler.h"
#include "../../../../The-Forge/Common_3/Graphics/Interfaces/IGraphics.h"

#include "../../src/AppSettings.h"

struct DataPerEye
{
    mat4 m_WorldToProjMat; // Matrix for converting World to Projected Space for the first eye
    mat4 m_ViewToWorldMat; // Matrix for converting View Space to World for the first eye
    mat4 m_LightToProjMat; // Matrix for converting Light to Projected Space Matrix for the first eye
    // we want to remove precision issue when the eye position is big:
    // - everything we draw is computed from its screen position or the view direction
    // - the floating point error got inserted to every member of viewMat * proj and inv(viewMat * proj) multiple times
    // - we convert screen space coordinate (NDC) to world space relative to the eye (world axis but the origin(0,0,0) becomes the eye)
    // - finally when sampling the cloud texture using world position we can add the eye position again, the offset is no more baked inside
    // mvp and the likes
    mat4 m_ProjToRelativeToEye; // Matrix for converting projected Space to world coordinate relative to the eye (inViewMatrix without
                                // translation part)
    mat4 m_RelativeToEyetoPreviousProj; // Matrix for converting current relative to eye coodinate to previous projected space (ViewMat +
                                        // deltaBetweenPreviousAndCurrentEye) * Proj, used for reprojection
    vec4 cameraPosition;
};

struct DataPerLayer
{
    float CloudsLayerStart;
    float EarthRadiusAddCloudsLayerStart;
    float EarthRadiusAddCloudsLayerStart2;
    float EarthRadiusAddCloudsLayerEnd;
    //======================================================
    float EarthRadiusAddCloudsLayerEnd2;
    float LayerThickness;

    // Cloud
    float CloudDensity;  // The overall density of clouds. Using bigger value makes more dense clouds, but it also makes ray-marching
                         // artifact worse.
    float CloudCoverage; // The overall coverage of clouds. Using bigger value makes more parts of the sky be covered by clouds. (But, it
                         // does not make clouds more dense)
    //======================================================

    float CloudType; // Add this value to control the overall clouds' type. 0.0 is for Stratus, 0.5 is for Stratocumulus, and 1.0 is for
                     // Cumulus.

    float CloudTopOffset; // Intensity of skewing clouds along the wind direction.

    // Modeling
    float CloudSize; // Overall size of the clouds. Using bigger value generates larger chunks of clouds.

    float BaseShapeTiling; // Control the base shape of the clouds. Using bigger value makes smaller chunks of base clouds.
    //======================================================

    float DetailShapeTiling; // Control the detail shape of the clouds. Using bigger value makes smaller chunks of detail clouds.

    float DetailStrenth; // Intensify the detail of the clouds. It is possible to lose whole shape of the clouds if the user uses too high
                         // value of it.
    float CurlTextureTiling; // Control the curl size of the clouds. Using bigger value makes smaller curl shapes.

    float CurlStrenth; // Intensify the curl effect.
    //======================================================

    float AnvilBias; // Using lower value makes anvil shape.
    float Contrast;
    float Precipitation;
    float RisingVaporIntensity;
    //======================================================

    vec4 WindDirection;
    vec4 StandardPosition; // The current center location for applying wind

    float WeatherTextureSize; // Control the size of Weather map, bigger value makes the world to be covered by larger clouds pattern.
    float WeatherTextureOffsetX;

    float WeatherTextureOffsetZ;
    float RotationPivotOffsetX;
    //======================================================

    float RotationPivotOffsetZ;
    float RotationAngle;

    float RisingVaporScale;
    float RisingVaporUpDirection;
};

struct VolumetricCloudsCB
{
    uint m_JitterX;           // the X offset of Re-projection
    uint m_JitterY;           // the Y offset of Re-projection
    uint MIN_ITERATION_COUNT; // Minimum iteration number of ray-marching
    uint MAX_ITERATION_COUNT; // Maximum iteration number of ray-marching

    DataPerEye   m_DataPerEye[2];
    DataPerLayer m_DataPerLayer[2];

    vec4 m_StepSize; // Cap of the step size X: min, Y: max

    vec4 TimeAndScreenSize; // X: EplasedTime, Y: RealTime, Z: FullWidth, W: FullHeight
    vec4 lightDirection;
    vec4 lightColorAndIntensity;

    vec4  EarthCenter;
    float EarthRadius;

    float m_MaxSampleDistance;

    float m_CorrectU; // m_JitterX / FullWidth
    float m_CorrectV; // m_JitterX / FullHeight

    // Lighting
    float BackgroundBlendFactor; // Blend clouds with the background, more background will be shown if this value is close to 0.0
    float Eccentricity;          // The bright highlights around the sun that the user needs at sunset
    float CloudBrightness;       // The brightness for clouds

    float SilverliningIntensity; // Intensity of silver-lining
    float SilverliningSpread;    // Using bigger value spreads more silver-lining, but the intesity of it
    float Random00;              // Random seed for the first ray-marching offset

    uint EnabledDepthCulling;
    uint m_HiZDepthMapWidth;
    uint m_HiZDepthMapHeight;

    // VolumetricClouds' Light shaft
    uint GodNumSamples; // Number of godray samples

    float GodrayMaxBrightness;
    float GodrayExposure; // Intensity of godray

    float GodrayDecay;   // Using smaller value, the godray brightness applied to each iteration is reduced. The level of reduction is also
                         // reduced per iteration.
    float GodrayDensity; // The distance between each interation.
    float GodrayWeight;  // Using smaller value, the godray brightness applied to each iteration is reduced. The level of reduction is not
                         // changed.
    float m_UseRandomSeed;

    float Test00;
    float Test01;
    float Test02;
    float Test03;

    float ReprojPrevFrameUnavail; // 1 when previous frame data is unavailable, 0 otherwise
    float CameraNear;
    float CameraFar;
    float PadA;
    float PadB;

    VolumetricCloudsCB()
    {
        for (int i = 0; i < 2; i++)
        {
            m_DataPerEye[i].m_WorldToProjMat = mat4::identity();
            m_DataPerEye[i].m_ViewToWorldMat = mat4::identity();
            m_DataPerEye[i].m_LightToProjMat = mat4::identity();
            m_DataPerEye[i].m_ProjToRelativeToEye = mat4::identity();
            m_DataPerEye[i].m_RelativeToEyetoPreviousProj = mat4::identity();
        }

        m_JitterX = 0;
        m_JitterY = 0;
        MIN_ITERATION_COUNT = 0;
        MAX_ITERATION_COUNT = 0;

        m_StepSize = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        TimeAndScreenSize = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        lightDirection = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        lightColorAndIntensity = vec4(0.0f, 0.0f, 0.0f, 0.0f);

        EarthRadius = 0.0f;

        m_MaxSampleDistance = 0.0f;

        for (int i = 0; i < 2; i++)
        {
            m_DataPerEye[i].cameraPosition = vec4(0.0f, 0.0f, 0.0f, 0.0f);
            m_DataPerLayer[i].WindDirection = vec4(0.0f, 0.0f, 0.0f, 0.0f);
            m_DataPerLayer[i].StandardPosition = vec4(0.0f, 0.0f, 0.0f, 0.0f);
            m_DataPerLayer[i].LayerThickness = 0.0f;

            // Cloud
            m_DataPerLayer[i].CloudDensity = 0.0f;
            m_DataPerLayer[i].CloudCoverage = 0.0f;
            m_DataPerLayer[i].CloudType = 0.0f;
            m_DataPerLayer[i].CloudTopOffset = 0.0f;

            // Modeling
            m_DataPerLayer[i].CloudSize = 0.0f;
            m_DataPerLayer[i].BaseShapeTiling = 0.0f;
            m_DataPerLayer[i].DetailShapeTiling = 0.0f;
            m_DataPerLayer[i].DetailStrenth = 0.0f;

            m_DataPerLayer[i].CurlTextureTiling = 0.0f;
            m_DataPerLayer[i].CurlStrenth = 0.0f;

            m_DataPerLayer[i].RotationPivotOffsetX = 0.0;
            m_DataPerLayer[i].RotationPivotOffsetZ = 0.0;
            m_DataPerLayer[i].RotationAngle = 0.0;

            m_DataPerLayer[i].RisingVaporScale = 1.0f;
            m_DataPerLayer[i].RisingVaporUpDirection = 1.0f;
            m_DataPerLayer[i].AnvilBias = 0.0f;
            m_DataPerLayer[i].WeatherTextureSize = 0.0f;
            m_DataPerLayer[i].WeatherTextureOffsetX = 0.0f;
            m_DataPerLayer[i].WeatherTextureOffsetZ = 0.0f;
        }

        // Wind
        m_CorrectU = 0.0f;
        m_CorrectV = 0.0f;

        // Lighting
        BackgroundBlendFactor = 0.0f;
        Eccentricity = 0.0f;
        CloudBrightness = 0.0f;

        SilverliningIntensity = 0.0f;
        SilverliningSpread = 0.0f;
        Random00 = 0.0f;

        EnabledDepthCulling = 0;
        m_HiZDepthMapWidth = 0;
        m_HiZDepthMapHeight = 0;

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

        ReprojPrevFrameUnavail = 0.0f;
        CameraNear = CAMERA_NEAR;
        CameraFar = CAMERA_FAR;
        PadA = 0.0f;
        PadB = 0.0f;
    }
};

class VolumetricClouds: public IMiddleware
{
public:
    virtual bool Init(Renderer* renderer, PipelineCache* pCache = NULL) final;
    virtual void Exit() final;
    virtual bool Load(RenderTarget** rts, uint32_t count = 1) final;
    virtual void Unload() final;
    virtual void Draw(Cmd* cmd) final;
    virtual void Update(float deltaTime) final;

    virtual void addDescriptorSets();
    virtual void removeDescriptorSets();
    virtual void addRootSignatures();
    virtual void removeRootSignatures();
    virtual void addShaders();
    virtual void removeShaders();
    virtual void addPipelines();
    virtual void removePipelines();
    virtual void addRenderTargets();
    virtual void removeRenderTargets();
    virtual void prepareDescriptorSets(RenderTarget** ppPreStageRenderTargets, uint32_t count = 1);

    void InitializeWithLoad(RenderTarget* InLinearDepthTexture, RenderTarget* InDepthTexture);

    void Initialize(ICameraController* InCameraController, Queue* InGraphicsQueue, ProfileToken InGraphicsGpuProfiler,
                    Buffer* pTransmittanceBuffer);

    bool Load(uint32_t width, uint32_t height);

    void Update(uint frameIndex);

    bool AddHiZDepthBuffer();
    void addVolumetricCloudsSaveTextures();

    void AddUniformBuffers();
    void RemoveUniformBuffers();

    bool   AfterSubmit(uint currentFrameIndex);
    float4 GetProjectionExtents(float fov, float aspect, float width, float height, float texelOffsetX, float texelOffsetY);

    Texture* GetWeatherMap();

    // Below are passed from Previous stage via Initialize()
    Renderer*      pRenderer = NULL;
    PipelineCache* pPipelineCache = NULL;

    // #NOTE: Two sets of resources (one in flight and one being used on CPU)
    static const uint32_t gDataBufferCount = 2;

    uint mWidth = 0;
    uint mHeight = 0;

    uint gFrameIndex = 0;

    RenderTarget*      pLinearDepthTexture = NULL;
    RenderTarget*      pSceneColorTexture = NULL;
    ICameraController* pCameraController = NULL;

    Queue* pGraphicsQueue = NULL;

    ProfileToken gGpuProfileToken = {};

    RenderTarget* pCastShadowRT = NULL;
    RenderTarget* pFinalRT = NULL;

    float3  LightDirection;
    Buffer* pTransmittanceBuffer = NULL;

    VolumetricCloudsCB volumetricCloudsCB;
    vec4               g_StandardPosition;
    vec4               g_StandardPosition_2nd;
    vec4               g_ShadowInfo;
    RenderTarget*      pDepthTexture = NULL;

    Texture* pHighResCloudTexture = NULL;

    uint32_t gDownsampledCloudSize = 0;

private:
    void GenerateCloudTextures();
};
