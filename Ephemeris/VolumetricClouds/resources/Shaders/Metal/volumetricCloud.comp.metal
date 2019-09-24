/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include <metal_stdlib>
#include <metal_compute>
using namespace metal;

struct Compute_Shader
{
#define _EARTH_RADIUS 5.000000e+06
#define _EARTH_CENTER float3(0, (-_EARTH_RADIUS), 0)
#define _CLOUDS_LAYER_START 15000.0
#define _EARTH_RADIUS_ADD_CLOUDS_LAYER_START 5.015000e+06
#define _EARTH_RADIUS_ADD_CLOUDS_LAYER_START2 2.515023e+13
#define TRANSMITTANCE_SAMPLE_STEP_COUNT 5
#define MAX_SAMPLE_DISTANCE 200000.0
#define PI 3.14159274
#define PI2 6.2831854
#define ONE_OVER_FOURPI 0.07957747
#define THREE_OVER_16PI 0.059683104
#define USE_Ray_Distance 1
#define ALLOW_CONTROL_CLOUD 1
#define STEREO_INSTANCED 0
#define USE_DEPTH_CULLING 1
#define FLOAT16_MAX 65500.0f 

    struct VolumetricCloudsCB
    {
        float4x4 m_WorldToProjMat_1st;
        float4x4 m_ProjToWorldMat_1st;
        float4x4 m_ViewToWorldMat_1st;
        float4x4 m_PrevWorldToProjMat_1st;
        float4x4 m_WorldToProjMat_2nd;
        float4x4 m_ProjToWorldMat_2nd;
        float4x4 m_ViewToWorldMat_2nd;
        float4x4 m_PrevWorldToProjMat_2nd;
        float4x4 m_LightToProjMat_1st;
        float4x4 m_LightToProjMat_2nd;
        uint m_JitterX;
        uint m_JitterY;
        uint MIN_ITERATION_COUNT;
        uint MAX_ITERATION_COUNT;
        float4 m_StepSize;
        float4 TimeAndScreenSize;
        float4 lightDirection;
        float4 lightColorAndIntensity;
        float4 cameraPosition_1st;
        float4 cameraPosition_2nd;
        float4 WindDirection;
        float4 StandardPosition;
        float m_CorrectU;
        float m_CorrectV;
        float LayerThickness;
        float CloudDensity;
        float CloudCoverage;
        float CloudType;
        float CloudTopOffset;
        float CloudSize;
        float BaseShapeTiling;
        float DetailShapeTiling;
        float DetailStrenth;
        float CurlTextureTiling;
        float CurlStrenth;
        float AnvilBias;
        float WeatherTextureSize;
        float WeatherTextureOffsetX;
        float WeatherTextureOffsetZ;
        float BackgroundBlendFactor;
        float Contrast;
        float Eccentricity;
        float CloudBrightness;
        float Precipitation;
        float SilverliningIntensity;
        float SilverliningSpread;
        float Random00;
        float CameraFarClip;
        float Padding01;
        float Padding02;
        float Padding03;
        uint EnabledDepthCulling;
        uint EnabledLodDepthCulling;
        uint padding04;
        uint padding05;
        uint GodNumSamples;
        float GodrayMaxBrightness;
        float GodrayExposure;
        float GodrayDecay;
        float GodrayDensity;
        float GodrayWeight;
        float m_UseRandomSeed;
        float Test00;
        float Test01;
        float Test02;
        float Test03;
    };
    texture3d<float> highFreqNoiseTexture;
    texture3d<float> lowFreqNoiseTexture;
    texture2d<float> curlNoiseTexture;
    texture2d<float> weatherTexture;
    texture2d<float> depthTexture;
    //texture2d<float> LowResCloudTexture;
    //texture2d<float> g_PrevFrameTexture;
    texture2d<float> g_LinearDepthTexture;
    sampler g_LinearClampSampler;
    sampler g_LinearWrapSampler;
    sampler g_PointClampSampler;
    sampler g_LinearBorderSampler;
    struct Uniforms_VolumetricCloudsCBuffer
    {
        VolumetricCloudsCB g_VolumetricClouds;
    };
    constant Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer;
    float randomFromScreenUV(float2 uv)
    {
        return fract((sin(dot((uv).xy, float2(12.9898005, 78.23300170))) * (float)(43758.546875)));
    };
    float3 rand[(TRANSMITTANCE_SAMPLE_STEP_COUNT + 1)] = { {0.0, 0.0, 0.0}, {0.612305, (-0.1875), 0.28125}, {0.648437, 0.026367000, (-0.792969)}, {(-0.636719), 0.449219, (-0.539062)}, {(-0.8085940), 0.74804696, 0.456055}, {0.542969, 0.35156300, 0.048462} };
    bool ray_trace_sphere(float3 center, float3 rd, float3 offset, float radius2, thread float(& t1), thread float(& t2))
    {
        float3 p = (center - offset);
        float b = dot(p, rd);
        float c = (dot(p, p) - radius2);
        float f = ((b * b) - c);
        if ((f >= (float)(0.0)))
        {
            float sqrtF = sqrt(f);
            (t1 = ((-b) - sqrtF));
            (t2 = ((-b) + sqrtF));
            if (((t2 <= (float)(0.0)) && (t1 <= (float)(0.0))))
            {
                return false;
            }
            return true;
        }
        else
        {
            (t1 = (float)(0.0));
            (t2 = (float)(0.0));
            return false;
        }
    };
    bool GetStartEndPointForRayMarching(float3 ws_origin, float3 ws_ray, float EARTH_RADIUS_ADD_CLOUDS_LAYER_END2, thread float3(& start), thread float3(& end))
    {
        float ot1, ot2, it1, it2;
        (start = (end = float3(0.0, 0.0, 0.0)));
        if ((!ray_trace_sphere(ws_origin, ws_ray, _EARTH_CENTER, EARTH_RADIUS_ADD_CLOUDS_LAYER_END2, ot1, ot2)))
        {
            return false;
        }
        bool inIntersected = ray_trace_sphere(ws_origin, ws_ray, _EARTH_CENTER, _EARTH_RADIUS_ADD_CLOUDS_LAYER_START2, it1, it2);
        if (inIntersected)
        {
            float branchFactor = saturate(floor((it1 + (float)(1.0))));
            (start = (ws_origin + ((float3)(mix(max(it2, 0.0), max(ot1, 0.0), branchFactor)) * ws_ray)));
            (end = (ws_origin + ((float3)(mix(ot2, it1, branchFactor)) * ws_ray)));
        }
        else
        {
            (end = (ws_origin + ((float3)(ot2) * ws_ray)));
            (start = (ws_origin + ((float3)(max(ot1, 0.0)) * ws_ray)));
        }
        return true;
    };
    float Remap(float original_value, float original_min, float original_max, float new_min, float new_max)
    {
        return (new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min)));
    };
    float RemapClamped(float original_value, float original_min, float original_max, float new_min, float new_max)
    {
        return (new_min + (saturate(((original_value - original_min) / (original_max - original_min))) * (new_max - new_min)));
    };
    float HenryGreenstein(float g, float cosTheta)
    {
        float g2 = (g * g);
        float numerator = ((float)(1.0) - g2);
        float denominator = pow(abs((((float)(1.0) + g2) - (((float)(2.0) * g) * cosTheta))), 1.5);
        return (((float)(ONE_OVER_FOURPI) * numerator) / denominator);
    };
    float Phase(float g, float g2, float cosTheta, float y)
    {
        return mix(HenryGreenstein(g, cosTheta), HenryGreenstein(g2, cosTheta), y);
    };
    float3 getProjectedShellPoint(float3 pt, float3 center)
    {
        return (((float3)(_EARTH_RADIUS_ADD_CLOUDS_LAYER_START) * normalize((pt - center))) + center);
    };
    float getRelativeHeight(float3 pt, float3 projectedPt, float layer_thickness)
    {
        return saturate((length((pt - projectedPt)) / layer_thickness));
    };
    float getRelativeHeightAccurate(float3 pt, float3 projectedPt, float layer_thickness)
    {
        float t = distance(pt, _EARTH_CENTER);
        (t -= (float)(_EARTH_RADIUS_ADD_CLOUDS_LAYER_START));
        return saturate((max(t, 0.0) / layer_thickness));
    };
    float PackFloat16(float value)
    {
        return (value * 0.001);
    };
    float UnPackFloat16(float value)
    {
        return (value * 1000.0);
    };
    float getAtmosphereBlendForComposite(float distance)
    {
        return saturate((max((distance - 150000.0), 0.0) / 50000.0));
    };
    float GetLODBias(float distance)
    {
        float factor = 50000.0;
        return saturate((max((distance - factor), 0.0) / ((float)(MAX_SAMPLE_DISTANCE) - factor)));
    };
    float GetDensityHeightGradientForPoint(float relativeHeight, float cloudType)
    {
        float cumulus = max(0.0, (RemapClamped(relativeHeight, 0.010000000, 0.15, 0.0, 1.0) * RemapClamped(relativeHeight, 0.9, 0.95, 1.0, 0.0)));
        float stratocumulus = max(0.0, (RemapClamped(relativeHeight, 0.0, 0.15, 0.0, 1.0) * RemapClamped(relativeHeight, 0.3, 0.65, 1.0, 0.0)));
        float stratus = max(0.0, (RemapClamped(relativeHeight, 0.0, 0.1, 0.0, 1.0) * RemapClamped(relativeHeight, 0.2, 0.3, 1.0, 0.0)));
        float cloudType2 = (cloudType * (float)(2.0));
        float a = mix(stratus, stratocumulus, saturate(cloudType2));
        float b = mix(stratocumulus, cumulus, saturate((cloudType2 - (float)(1.0))));
        return mix(a, b, round(cloudType));
    };
    float SampleDensity(float3 worldPos, float lod, float height_fraction, float3 currentProj, float3 cloudTopOffsetWithWindDir, float2 windWithVelocity, float3 biasedCloudPos, float DetailShapeTilingDivCloudSize, bool cheap)
    {
        float3 unwindWorldPos = worldPos;
        (worldPos += ((float3)(height_fraction) * cloudTopOffsetWithWindDir));
        (worldPos += biasedCloudPos);
        float3 weatherData = weatherTexture.sample(g_LinearWrapSampler, ((((unwindWorldPos).xz + windWithVelocity) + float2((VolumetricCloudsCBuffer.g_VolumetricClouds).WeatherTextureOffsetX, (VolumetricCloudsCBuffer.g_VolumetricClouds).WeatherTextureOffsetZ)) / (float2)((VolumetricCloudsCBuffer.g_VolumetricClouds).WeatherTextureSize)), level(0.0)).rgb;
        float3 worldPosDivCloudSize = (worldPos / (float3)((VolumetricCloudsCBuffer.g_VolumetricClouds).CloudSize));
        float4 low_freq_noises = lowFreqNoiseTexture.sample(g_LinearWrapSampler, (worldPosDivCloudSize * (float3)((VolumetricCloudsCBuffer.g_VolumetricClouds).BaseShapeTiling)), level(lod));
        float low_freq_fBm = ((((low_freq_noises).g * (float)(0.625)) + ((low_freq_noises).b * (float)(0.25))) + ((low_freq_noises).a * (float)(0.125)));
        float base_cloud = RemapClamped((low_freq_noises).r, (low_freq_fBm - (float)(1.0)), 1.0, 0.0, 1.0);
        (base_cloud = saturate((base_cloud + (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudCoverage)));
        float cloudType = saturate(((weatherData).b + (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudType));
        float density_height_gradient = GetDensityHeightGradientForPoint(height_fraction, cloudType);
        (base_cloud *= density_height_gradient);
        float cloud_coverage = saturate((weatherData).g);
        (cloud_coverage = pow(cloud_coverage, RemapClamped(height_fraction, 0.2, 0.8, 1.0, mix(1.0, 0.5, (VolumetricCloudsCBuffer.g_VolumetricClouds).AnvilBias))));
        float base_cloud_coverage = RemapClamped(base_cloud, cloud_coverage, 1.0, 0.0, 1.0);
        (base_cloud_coverage *= cloud_coverage);
        float final_cloud = base_cloud_coverage;
        if ((!cheap))
        {
            float2 curl_noise = curlNoiseTexture.sample(g_LinearWrapSampler, float2(((worldPosDivCloudSize).xz * (float2)((VolumetricCloudsCBuffer.g_VolumetricClouds).CurlTextureTiling))), level(0.0)).rg;
            ((worldPos).xz += ((curl_noise * (float2)(((float)(1.0) - height_fraction))) * (float2)((VolumetricCloudsCBuffer.g_VolumetricClouds).CurlStrenth)));
            float3 high_frequency_noises = highFreqNoiseTexture.sample(g_LinearWrapSampler, float3((worldPos * (float3)(DetailShapeTilingDivCloudSize))), level(lod)).rgb;
            float high_freq_fBm = ((((high_frequency_noises).r * (float)(0.625)) + ((high_frequency_noises).g * (float)(0.25))) + ((high_frequency_noises).b * (float)(0.125)));
            float height_fraction_new = getRelativeHeight(worldPos, currentProj, (VolumetricCloudsCBuffer.g_VolumetricClouds).LayerThickness);
            float height_freq_noise_modifier = mix(high_freq_fBm, ((float)(1.0) - high_freq_fBm), saturate((height_fraction_new * (float)(10.0))));
            (final_cloud = RemapClamped(base_cloud_coverage, (height_freq_noise_modifier * (VolumetricCloudsCBuffer.g_VolumetricClouds).DetailStrenth), 1.0, 0.0, 1.0));
        }
        return final_cloud;
    };
    float GetLightEnergy(float height_fraction, float dl, float ds_loded, float phase_probability, float cos_angle, float step_size, float contrast)
    {
        float primary_att = exp((-dl));
        float secondary_att = (exp(((-dl) * (float)(0.25))) * (float)(0.7));
        float attenuation_probability = max(primary_att, (secondary_att * (float)(0.25)));
        float depth_probability = mix(((float)(0.05) + pow(ds_loded, RemapClamped(height_fraction, 0.3, 0.85, 0.5, 2.0))), 1.0, saturate(dl));
        float vertical_probability = pow(RemapClamped(height_fraction, 0.07, 0.14, 0.1, 1.0), 0.8);
        float in_scatter_probability = (vertical_probability * depth_probability);
        float light_energy = (attenuation_probability + (in_scatter_probability * phase_probability));
        (light_energy = pow(abs(light_energy), contrast));
        return light_energy;
    };
    float SampleEnergy(float3 rayPos, float3 magLightDirection, float height_fraction, float3 currentProj, float3 cloudTopOffsetWithWindDir, float2 windWithVelocity, float3 biasedCloudPos, float DetailShapeTilingDivCloudSize, float ds_loded, float stepSize, float cosTheta, float mipBias)
    {
        float totalSample = 0;
        float mipmapOffset = mipBias;
        float step = 0.5;
        for (int i = 0; (i < TRANSMITTANCE_SAMPLE_STEP_COUNT); (i++))
        {
            float3 rand3 = rand[i];
            float3 direction = (magLightDirection + normalize(rand3));
            (direction = normalize(direction));
            float3 samplePoint = (rayPos + ((float3)((step * stepSize)) * direction));
            float3 currentProj_new = getProjectedShellPoint(samplePoint, _EARTH_CENTER);
            float height_fraction_new = getRelativeHeight(samplePoint, currentProj_new, (VolumetricCloudsCBuffer.g_VolumetricClouds).LayerThickness);
            (totalSample += SampleDensity(samplePoint, mipmapOffset, height_fraction_new, currentProj_new, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, false));
            (mipmapOffset += (float)(0.5));
            (step += 1.0);
        }
        float hg = max(HenryGreenstein((VolumetricCloudsCBuffer.g_VolumetricClouds).Eccentricity, cosTheta), (saturate(HenryGreenstein((0.99 - (VolumetricCloudsCBuffer.g_VolumetricClouds).SilverliningSpread), cosTheta)))) * (VolumetricCloudsCBuffer.g_VolumetricClouds).SilverliningIntensity;
		float dl = totalSample * VolumetricCloudsCBuffer.g_VolumetricClouds.Precipitation;
		hg = hg / max(dl, 0.05);

		float lodded_density = saturate(SampleDensity(rayPos, mipBias, height_fraction, currentProj, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, true));
        float energy = GetLightEnergy(height_fraction, dl, ds_loded, hg, cosTheta, stepSize, (VolumetricCloudsCBuffer.g_VolumetricClouds).Contrast);
        return energy;
    };
    float GetDensity(float3 startPos, float3 worldPos, float3 dir, float maxSampleDistance, float raymarchOffset, float EARTH_RADIUS_ADD_CLOUDS_LAYER_END, float EARTH_RADIUS_ADD_CLOUDS_LAYER_END2, thread float(& intensity), thread float(& atmosphericBlendFactor), thread float(& depth), float2 uv)
    {
        float3 sampleStart, sampleEnd;
        (depth = (float)(0.0));
        (intensity = (float)(0.0));
        (atmosphericBlendFactor = 0.0);
        if ((!GetStartEndPointForRayMarching(startPos, dir, EARTH_RADIUS_ADD_CLOUDS_LAYER_END2, sampleStart, sampleEnd)))
        {
            return 0.0;
        }
        float horizon = abs((dir).y);
        uint sample_count = (uint)(mix((float)(VolumetricCloudsCBuffer.g_VolumetricClouds.MAX_ITERATION_COUNT), (float)(VolumetricCloudsCBuffer.g_VolumetricClouds.MIN_ITERATION_COUNT), horizon));
        float sample_step = min((length((sampleEnd - sampleStart)) / (float)((float)(sample_count) * 0.43)), mix(((VolumetricCloudsCBuffer.g_VolumetricClouds).m_StepSize).y, ((VolumetricCloudsCBuffer.g_VolumetricClouds).m_StepSize).x, pow(horizon, 0.33)));
        (depth = distance(sampleEnd, startPos));
        float distCameraToStart = distance(sampleStart, startPos);
        //(atmosphericBlendFactor = PackFloat16(distCameraToStart));

        if (UnPackFloat16(g_LinearDepthTexture.sample(g_LinearClampSampler, uv, level(0.0)).r) < FLOAT16_MAX)
        {
          atmosphericBlendFactor = 1.0f;
        }

        if (((sampleStart).y + VolumetricCloudsCBuffer.g_VolumetricClouds.Test01 < 0.0))
        {
            (intensity = (float)(0.0));
            return 0.0;
        }
        //if ((distCameraToStart >= (float)(MAX_SAMPLE_DISTANCE)))
        //{
        //    return 0.0;
        //}
        float scaleFactor = mix(0.125, 1.0, saturate((distCameraToStart / (float)(_CLOUDS_LAYER_START))));
        (sample_count = (uint)((float)((float)(sample_count) / scaleFactor)));
        (sample_step *= scaleFactor);
        float transStepSize = (((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).a * scaleFactor);
#if STEREO_INSTANCED
        float2 textureSize = (((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw * float2(0.5, 0.25));
#else
        float2 textureSize = (((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw * (float2)(0.25));
#endif

        uint2 texels = (uint2)(float2((VolumetricCloudsCBuffer.g_VolumetricClouds).Padding01, (VolumetricCloudsCBuffer.g_VolumetricClouds).Padding03) * uv);
        float sceneDepth;
        if (((float)((VolumetricCloudsCBuffer.g_VolumetricClouds).EnabledLodDepthCulling) > 0.5))
        {
            (sceneDepth = UnPackFloat16((float)(depthTexture.read(texels, 0).r)));
            if ((sceneDepth < FLOAT16_MAX))
            {
                return 1.0;
            }
        }
        float alpha = 0.0;
        (intensity = 0.0);
        bool detailedSample = false;
        int missedStepCount = 0;
        float ds = 0.0;
        float bigStep = (sample_step * (float)(2.0));
        float3 smallStepMarching = ((float3)(sample_step) * dir);
        float3 bigStepMarching = ((float3)(bigStep) * dir);
        float3 raymarchingDistance = (smallStepMarching * (float3)(raymarchOffset));
        float3 magLightDirection = ((float3)(2.0) * ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);
        bool pickedFirstHit = false;
        float cosTheta = dot(dir, ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);
        float3 rayPos = sampleStart;
        float2 windWithVelocity = ((VolumetricCloudsCBuffer.g_VolumetricClouds).StandardPosition).xz;
        float3 biasedCloudPos = ((float3)(4.5) * (((VolumetricCloudsCBuffer.g_VolumetricClouds).WindDirection).xyz + float3(0.0, 0.1, 0.0)));
        float3 cloudTopOffsetWithWindDir = ((float3)((VolumetricCloudsCBuffer.g_VolumetricClouds).CloudTopOffset) * ((VolumetricCloudsCBuffer.g_VolumetricClouds).WindDirection).xyz);
        float DetailShapeTilingDivCloudSize = ((VolumetricCloudsCBuffer.g_VolumetricClouds).DetailShapeTiling / (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudSize);
        float LODbias = 0.0;
        float cheapLOD = (LODbias + 2.0);
        for (uint j = 0; (j < sample_count); (j++))
        {
            (rayPos += raymarchingDistance);
            float3 currentProj = getProjectedShellPoint(rayPos, _EARTH_CENTER);
            float height_fraction = getRelativeHeightAccurate(rayPos, currentProj, (VolumetricCloudsCBuffer.g_VolumetricClouds).LayerThickness);
            if ((!detailedSample))
            {
                float sampleResult = SampleDensity(rayPos, cheapLOD, height_fraction, currentProj, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, true);
                if ((sampleResult > 0.0))
                {
                    (detailedSample = true);
                    (raymarchingDistance = (-bigStepMarching));
                    (missedStepCount = 0);
                    continue;
                }
                else
                {
                    (raymarchingDistance = bigStepMarching);
                }
            }
            else
            {
                float sampleResult = SampleDensity(rayPos, LODbias, height_fraction, currentProj, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, false);
                if ((sampleResult == (float)(0.0)))
                {
                    (missedStepCount++);
                    if ((missedStepCount > 10))
                    {
                        (detailedSample = false);
                    }
                }
                else
                {
                    if ((!pickedFirstHit))
                    {
                        (depth = distance(rayPos, startPos));
                        (pickedFirstHit = true);
                    }
                    float sampledAlpha = (sampleResult * (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudDensity);
                    float sampledEnergy = SampleEnergy(rayPos, magLightDirection, height_fraction, currentProj, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, sampleResult, transStepSize, cosTheta, 0.0);
                    float oneMinusAlpha = ((float)(1.0) - alpha);
                    (sampledAlpha *= oneMinusAlpha);
                    (intensity += (sampledAlpha * sampledEnergy));
                    (alpha += sampledAlpha);
                    if ((alpha >= 1.0))
                    {
                        (intensity /= alpha);
                        (depth = PackFloat16(depth));
                         atmosphericBlendFactor = 1.0f;
                        return 1.0;
                    }
                }
                (raymarchingDistance = smallStepMarching);
            }
        }
        (depth = PackFloat16(depth));
        atmosphericBlendFactor = max(atmosphericBlendFactor, alpha);
        return alpha;
    };
    float GetDensityWithComparingDepth(float3 startPos, float3 worldPos, float3 dir, float maxSampleDistance, float raymarchOffset, float EARTH_RADIUS_ADD_CLOUDS_LAYER_END, float EARTH_RADIUS_ADD_CLOUDS_LAYER_END2, thread float(& intensity), thread float(& atmosphericBlendFactor), thread float(& depth), float2 uv)
    {
        float3 sampleStart, sampleEnd;
        (depth = (float)(0.0));
        (intensity = (float)(0.0));
        (atmosphericBlendFactor = 0.0);
        if ((!GetStartEndPointForRayMarching(startPos, dir, EARTH_RADIUS_ADD_CLOUDS_LAYER_END2, sampleStart, sampleEnd)))
        {
            return 0.0;
        }
        float horizon = abs((dir).y);
        uint sample_count = (uint)(mix((float)(VolumetricCloudsCBuffer.g_VolumetricClouds.MAX_ITERATION_COUNT), (float)(VolumetricCloudsCBuffer.g_VolumetricClouds.MIN_ITERATION_COUNT), horizon));
        float sample_step = min((length((sampleEnd - sampleStart)) / (float)((float)(sample_count) * 0.43)), mix(((VolumetricCloudsCBuffer.g_VolumetricClouds).m_StepSize).y, ((VolumetricCloudsCBuffer.g_VolumetricClouds).m_StepSize).x, pow(horizon, 0.33)));
        (depth = distance(sampleEnd, startPos));
        float distCameraToStart = distance(sampleStart, startPos);
        //(atmosphericBlendFactor = PackFloat16(distCameraToStart));
        float sceneDepth = UnPackFloat16(depthTexture.sample(g_LinearClampSampler, uv, level(0)).r);
        if (sceneDepth < FLOAT16_MAX)
        {
          atmosphericBlendFactor = 1.0f;
        }

        if (((sampleStart).y + VolumetricCloudsCBuffer.g_VolumetricClouds.Test01 < 0.0))
        {
            (intensity = (float)(0.0));
            return 0.0;
        }
        //if ((distCameraToStart >= (float)(MAX_SAMPLE_DISTANCE)))
        //{
        //    return 0.0;
        //}
        float scaleFactor = mix(0.125, 1.0, saturate((distCameraToStart / (float)(_CLOUDS_LAYER_START))));
        (sample_count = (uint)((float)((float)(sample_count) / scaleFactor)));
        (sample_step *= scaleFactor);
        float transStepSize = (((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).a * scaleFactor);
#if STEREO_INSTANCED
        float2 textureSize = (((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw * float2(0.5, 0.25));
#else
        float2 textureSize = (((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw * (float2)(0.25));
#endif

        uint2 texels = (uint2)(float2((VolumetricCloudsCBuffer.g_VolumetricClouds).Padding01, (VolumetricCloudsCBuffer.g_VolumetricClouds).Padding03) * uv);
        //float sceneDepth = depthTexture.read(texels, 0).r;
        float alpha = 0.0;
        (intensity = 0.0);
        bool detailedSample = false;
        int missedStepCount = 0;
        float ds = 0.0;
        float bigStep = (sample_step * (float)(2.0));
        float3 smallStepMarching = ((float3)(sample_step) * dir);
        float3 bigStepMarching = ((float3)(bigStep) * dir);
        float3 raymarchingDistance = (smallStepMarching * (float3)(raymarchOffset));
        float3 magLightDirection = ((float3)(2.0) * ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);
        bool pickedFirstHit = false;
        float cosTheta = dot(dir, ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);
        float3 rayPos = sampleStart;
        float2 windWithVelocity = ((VolumetricCloudsCBuffer.g_VolumetricClouds).StandardPosition).xz;
        float3 biasedCloudPos = ((float3)(4.5) * (((VolumetricCloudsCBuffer.g_VolumetricClouds).WindDirection).xyz + float3(0.0, 0.1, 0.0)));
        float3 cloudTopOffsetWithWindDir = ((float3)((VolumetricCloudsCBuffer.g_VolumetricClouds).CloudTopOffset) * ((VolumetricCloudsCBuffer.g_VolumetricClouds).WindDirection).xyz);
        float DetailShapeTilingDivCloudSize = ((VolumetricCloudsCBuffer.g_VolumetricClouds).DetailShapeTiling / (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudSize);
        float LODbias = 0.0;
        float cheapLOD = (LODbias + 2.0);
        for (uint j = 0; (j < sample_count); (j++))
        {
            (rayPos += raymarchingDistance);
            if ((sceneDepth < distance(startPos, rayPos)))
            {
                break;
            }
            float3 currentProj = getProjectedShellPoint(rayPos, _EARTH_CENTER);
            float height_fraction = getRelativeHeightAccurate(rayPos, currentProj, (VolumetricCloudsCBuffer.g_VolumetricClouds).LayerThickness);
            if ((!detailedSample))
            {
                float sampleResult = SampleDensity(rayPos, cheapLOD, height_fraction, currentProj, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, true);
                if ((sampleResult > 0.0))
                {
                    (detailedSample = true);
                    (raymarchingDistance = (-bigStepMarching));
                    (missedStepCount = 0);
                    continue;
                }
                else
                {
                    (raymarchingDistance = bigStepMarching);
                }
            }
            else
            {
                float sampleResult = SampleDensity(rayPos, LODbias, height_fraction, currentProj, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, false);
                if ((sampleResult == (float)(0.0)))
                {
                    (missedStepCount++);
                    if ((missedStepCount > 10))
                    {
                        (detailedSample = false);
                    }
                }
                else
                {
                    if ((!pickedFirstHit))
                    {
                        (depth = distance(rayPos, startPos));
                        (pickedFirstHit = true);
                    }
                    float sampledAlpha = (sampleResult * (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudDensity);
                    float sampledEnergy = SampleEnergy(rayPos, magLightDirection, height_fraction, currentProj, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, sampleResult, transStepSize, cosTheta, 0.0);
                    float oneMinusAlpha = ((float)(1.0) - alpha);
                    (sampledAlpha *= oneMinusAlpha);
                    (intensity += (sampledAlpha * sampledEnergy));
                    (alpha += sampledAlpha);
                    if ((alpha >= 1.0))
                    {
                        (intensity /= alpha);
                        (depth = PackFloat16(depth));
                        atmosphericBlendFactor = 1.0f;
                        return 1.0;
                    }
                }
                (raymarchingDistance = smallStepMarching);
            }
        }
        (depth = PackFloat16(depth));
        atmosphericBlendFactor = max(atmosphericBlendFactor, alpha);
		return alpha;
    };
    
    texture2d<float, access::read_write> DstTexture;
	
    void main(uint3 GTid, uint3 Gid, uint3 DTid)
    {
		if(DTid.x >= VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.z * 0.25 ||
		   DTid.y >= VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.w * 0.25 )
        {
            return;
        }
        float2 db_uvs = float2(((float)((float)(DTid.x * (uint)(4) + (VolumetricCloudsCBuffer.g_VolumetricClouds).m_JitterX) + 0.5) / ((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).z), ((float)((float)(DTid.y * (uint)(4) + (VolumetricCloudsCBuffer.g_VolumetricClouds).m_JitterY) + 0.5) / ((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).w));
        float2 ScreenUV = db_uvs;
        float3 ScreenNDC;
        ((ScreenNDC).x = (((ScreenUV).x * (float)(2.0)) - (float)(1.0)));
        ((ScreenNDC).y = ((((float)(1.0) - (ScreenUV).y) * (float)(2.0)) - (float)(1.0)));
        float3 projectedPosition = float3((ScreenNDC).xy, 0.0);
        float4 worldPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_ProjToWorldMat_1st)*(float4(projectedPosition, 1.0)));
        float4 CameraPosition = (VolumetricCloudsCBuffer.g_VolumetricClouds).cameraPosition_1st;
        (worldPos /= (float4)((worldPos).w));
        float3 viewDir = normalize(((worldPos).xyz - (CameraPosition).xyz));
        float intensity;
        float atmosphereBlendFactor;
        float depth;
        float EARTH_RADIUS_ADD_CLOUDS_LAYER_END = ((float)(_EARTH_RADIUS_ADD_CLOUDS_LAYER_START) + (VolumetricCloudsCBuffer.g_VolumetricClouds).LayerThickness);
        float EARTH_RADIUS_ADD_CLOUDS_LAYER_END2 = (EARTH_RADIUS_ADD_CLOUDS_LAYER_END * EARTH_RADIUS_ADD_CLOUDS_LAYER_END);
        float randomSeed = mix(0.0, (VolumetricCloudsCBuffer.g_VolumetricClouds).Random00, (VolumetricCloudsCBuffer.g_VolumetricClouds).m_UseRandomSeed);
        float dentisy = GetDensity((CameraPosition).xyz, (worldPos).xyz, viewDir, MAX_SAMPLE_DISTANCE, randomSeed, EARTH_RADIUS_ADD_CLOUDS_LAYER_END, EARTH_RADIUS_ADD_CLOUDS_LAYER_END2, intensity, atmosphereBlendFactor, depth, db_uvs);
        DstTexture.write(float4(intensity, atmosphereBlendFactor, depth, dentisy), uint2(int2((DTid).xy)));
		
		
		
    };

    Compute_Shader(
texture3d<float> highFreqNoiseTexture,texture3d<float> lowFreqNoiseTexture,
				   texture2d<float> curlNoiseTexture,texture2d<float> weatherTexture,
				   texture2d<float> depthTexture,
				   //texture2d<float> LowResCloudTexture,
				   //texture2d<float> g_PrevFrameTexture,
           texture2d<float> g_LinearDepthTexture,
				   sampler g_LinearClampSampler,sampler g_LinearWrapSampler,sampler g_PointClampSampler,sampler g_LinearBorderSampler,constant Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer,texture2d<float, access::read_write> DstTexture) :
highFreqNoiseTexture(highFreqNoiseTexture),
	lowFreqNoiseTexture(lowFreqNoiseTexture),
	curlNoiseTexture(curlNoiseTexture),
	weatherTexture(weatherTexture),
	depthTexture(depthTexture),
	//LowResCloudTexture(LowResCloudTexture),
	//g_PrevFrameTexture(g_PrevFrameTexture),
  g_LinearDepthTexture(g_LinearDepthTexture),
	g_LinearClampSampler(g_LinearClampSampler),g_LinearWrapSampler(g_LinearWrapSampler),g_PointClampSampler(g_PointClampSampler),g_LinearBorderSampler(g_LinearBorderSampler),VolumetricCloudsCBuffer(VolumetricCloudsCBuffer),DstTexture(DstTexture) {}
};

struct ArgsData
{
    texture3d<float> highFreqNoiseTexture;
    texture3d<float> lowFreqNoiseTexture;
    texture2d<float> curlNoiseTexture;
    texture2d<float> weatherTexture;
    texture2d<float> depthTexture;
    //texture2d<float> LowResCloudTexture;
    //texture2d<float> g_PrevFrameTexture;
    texture2d<float> g_LinearDepthTexture;
    sampler g_LinearClampSampler;
    sampler g_LinearWrapSampler;
    sampler g_PointClampSampler;
    sampler g_LinearBorderSampler;
    texture2d<float, access::read_write> volumetricCloudsDstTexture;
};

struct ArgsPerFrame
{
    constant Compute_Shader::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer;
};

//[numthreads(8, 8, 1)]
kernel void stageMain(
uint3 GTid [[thread_position_in_threadgroup]],
uint3 Gid [[threadgroup_position_in_grid]],
uint3 DTid [[thread_position_in_grid]],
    constant ArgsData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant ArgsPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    uint3 GTid0;
    GTid0 = GTid;
    uint3 Gid0;
    Gid0 = Gid;
    uint3 DTid0;
    DTid0 = DTid;
    Compute_Shader main(
    argBufferStatic.highFreqNoiseTexture,
    argBufferStatic.lowFreqNoiseTexture,
    argBufferStatic.curlNoiseTexture,
    argBufferStatic.weatherTexture,
    argBufferStatic.depthTexture,
    //argBufferStatic.LowResCloudTexture,
    //argBufferStatic.g_PrevFrameTexture,
    argBufferStatic.g_LinearDepthTexture,
    argBufferStatic.g_LinearClampSampler,
    argBufferStatic.g_LinearWrapSampler,
    argBufferStatic.g_PointClampSampler,
    argBufferStatic.g_LinearBorderSampler,
    argBufferPerFrame.VolumetricCloudsCBuffer,
    argBufferStatic.volumetricCloudsDstTexture);
    return main.main(GTid0, Gid0, DTid0);
}
