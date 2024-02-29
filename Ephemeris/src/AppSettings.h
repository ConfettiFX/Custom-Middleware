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

#include <stdint.h>

#include "../../../The-Forge/Common_3/Utilities/Math/MathTypes.h"

#define CAMERA_NEAR          50.0f
#define CAMERA_FAR           100000000.0f
#define SPACE_NEAR           100000.0f
#define SPACE_FAR            2000000000.0f
// use earth's values, note:
//  - the atmospheric system has been configured to work with earth like units and scale
//  - if you made change to this value, you should update the convertion from world space to coordinate used in Sky.cpp
//  - in the end your whole planet should fit inside the atmospheric sphere (blue horizon line), if not you need to work on Skyp.cpp
#define PLANET_RADIUS        6360000.0f

#define CAMERA_SCRIPT_COUNTS 4

extern const char* gCameraScripts[CAMERA_SCRIPT_COUNTS];

typedef struct AppSettings
{
    // -------- Lua Scripts --------
    uint32_t gCurrentScriptIndex = 1;

    // -------- UI --------
    bool gToggleFXAA = true;
    bool gShowAdvancedWindows = false;
    bool gTogglePerformance = true;
    bool gShowInterface = true;

    // -------- Sky --------
    float4 SkyInfo; // x: fExposure, y: fInscatterIntencity, z: fInscatterContrast, w: fUnitsToM
    float4 OriginLocation;
    float2 SunDirection = float2(-90.0f, 210.0f);
    float4 SunColorAndIntensity = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float  sunMovingSpeed = 2.0f;
    bool   bSunMove = false;

    // -------- VolumetricCloud --------
    bool  m_Enabled = true;
    bool  m_FirstFrame = true; // when true, no previous frame data is available
    float m_DefaultMaxSampleDistance = 800000.0f;

    float m_CloudsLayerStart = 8800.0f;
    float m_CloudsLayerStart_2nd = 72000.0f;

    // raymarching
    uint32_t m_MinSampleCount = 64;
    uint32_t m_MaxSampleCount = 192;
    float    m_MinStepSize = 256.0;
    float    m_MaxStepSize = 1024.0;

    float m_LayerThickness = 55000.0f;
    float m_LayerThickness_2nd = 12500.0f;

    bool m_EnabledTemporalRayOffset = false;

    // modeling
    float m_BaseTile = 0.621f;

    float m_DetailTile = 5.496f;
    float m_DetailStrength = 0.25f;
    float m_CurlTile = 0.1f;
    float m_CurlStrength = 2000.0f;
    float m_CloudTopOffset = 500.0f;
    float m_CloudSize = 103305.805f;

    float m_CloudDensity = 3.4f;
    float m_CloudCoverageModifier = 0.0f;

    float m_CloudTypeModifier = 0.0f;
    float m_AnvilBias = 1.0f;
    float m_WeatherTexSize = 867770.0f;

    float WeatherTextureAzimuth = 90.0f;
    float WeatherTextureDistance = 0.0f;

    // wind factors
    float m_WindAzimuth = 0.0f;
    float m_WindIntensity = 20.0f;

    bool m_EnabledRotation = false;

    float m_RotationPivotAzimuth = 0.0f;
    float m_RotationPivotDistance = 0.0f;
    float m_RotationIntensity = 1.0f;
    float m_RisingVaporScale = 1.0f;
    float m_RisingVaporUpDirection = -1.0f;
    float m_RisingVaporIntensity = 5.0f;

    float m_NoiseFlowAzimuth = 180.0f;
    float m_NoiseFlowIntensity = 7.5f;

    // second layer
    bool m_Enabled2ndLayer = true;

    float m_BaseTile_2nd = 0.65f;

    float m_DetailTile_2nd = 3.141f;
    float m_DetailStrength_2nd = 0.25f;
    float m_CurlTile_2nd = 0.1f;
    float m_CurlStrength_2nd = 2000.0f;
    float m_CloudTopOffset_2nd = 500.0f;
    float m_CloudSize_2nd = 57438.0f;

    float m_CloudDensity_2nd = 3.0f;
    float m_CloudCoverageModifier_2nd = 0.0f;

    float m_CloudTypeModifier_2nd = 0.521f;
    float m_AnvilBias_2nd = 1.0f;
    float m_WeatherTexSize_2nd = 1000000.0f;

    float WeatherTextureAzimuth_2nd = 0.0f;
    float WeatherTextureDistance_2nd = 0.0f;

    uint32_t DownSampling = 1;
    uint     prevDownSampling = 1;
    bool     TemporalFilteringEnabled = false;
    bool     prevTemporalFilteringEnabled = false;

    // Wind factors
    float m_WindAzimuth_2nd = 180.0f;
    float m_WindIntensity_2nd = 0.0f;

    bool m_EnabledRotation_2nd = false;

    float m_RotationPivotAzimuth_2nd = 0.0f;
    float m_RotationPivotDistance_2nd = 0.0f;
    float m_RotationIntensity_2nd = 1.0f;
    float m_RisingVaporScale_2nd = 0.5f;
    float m_RisingVaporUpDirection_2nd = 0.0f;
    float m_RisingVaporIntensity_2nd = 1.0f;

    float m_NoiseFlowAzimuth_2nd = 180.0f;
    float m_NoiseFlowIntensity_2nd = 10.0f;

    // lighting
    float4 m_CustomColor = float4(1.f);

    float m_CustomColorIntensity = 1.0f;
    float m_CustomColorBlendFactor = 0.0f;
    float m_Contrast = 1.33f;
    float m_Contrast_2nd = 0.8f;
    float m_TransStepSize = 1340.0f;

    float m_BackgroundBlendFactor = 1.0f;
    float m_Precipitation = 2.5f;
    float m_Precipitation_2nd = 1.4f;
    float m_SilverIntensity = 0.5f;
    float m_SilverSpread = 0.29f;

    float m_Eccentricity = 0.65f;
    float m_CloudBrightness = 1.5f;
    // shadow
    bool  m_EnabledShadow = true;
    float m_ShadowIntensity = 0.8f;
    // culling
    // use a lod depth buffer storing the max value in the neighboring depth, this will not reject pixels near the moutain's edges
    bool  m_EnabledDepthCulling = true;

    // volumetricClouds' Light shaft
    bool     m_EnabledGodray = true;
    uint32_t m_GodNumSamples = 80;
    float    m_Exposure = 0.0025f;
    float    m_Decay = 0.995f;
    float    m_Density = 0.3f;
    float    m_Weight = 0.85f;

    // test & debug
    float m_Test00 = 0.5f;
    float m_Test01 = 2.0f;
    float m_Test02 = 2.0f;
    float m_Test03 = 0.8f;

#if defined(USE_CHEAP_SETTINGS)
    bool m_EnableBlur = true;
#else
    bool m_EnableBlur = false;
#endif

} AppSettings;