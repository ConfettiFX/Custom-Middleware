/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include <metal_stdlib>
using namespace metal;

#include "volumetricCloud.h"

struct Fragment_Shader
{
	texture3d<float> highFreqNoiseTexture;
    texture3d<float> lowFreqNoiseTexture;
    texture2d<float> curlNoiseTexture;
    texture2d<float> weatherTexture;
    texture2d<float> depthTexture;
    texture2d<float> LowResCloudTexture;
    texture2d<float> g_PrevFrameTexture;
    texture2d<float> g_LinearDepthTexture;

    sampler g_LinearClampSampler;
    sampler g_LinearWrapSampler;
    sampler g_PointClampSampler;
    sampler g_LinearBorderSampler;

    constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer;
	
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

	   bool GetStartEndPointForRayMarching(float3 ws_origin, float3 ws_ray, float3 earthCenter,
	float layerStart2, float LayerEnd2, thread float3(& start), thread float3(& end))
	   {
		   float ot1, ot2, it1, it2;
		   (start = (end = float3(0.0, 0.0, 0.0)));

		   if (!ray_trace_sphere(ws_origin, ws_ray, earthCenter, LayerEnd2, ot1, ot2))
		   {
			   return false;
		   }
		   bool inIntersected = ray_trace_sphere(ws_origin, ws_ray, earthCenter, layerStart2, it1, it2);
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

	  // Get the point projected to the inner atmosphere shell
	   float3 getProjectedShellPoint(float earthRadiusAddCloudLayerStart, float3 pt, float3 center)
	   {
		   return earthRadiusAddCloudLayerStart * normalize(pt - center) + center;
	   }

	   float getRelativeHeight(float3 pt, float3 projectedPt, float layer_thickness)
	   {
		   return saturate((length((pt - projectedPt)) / layer_thickness));
	   };

	   // Get the relative height accurately
	   float getRelativeHeightAccurate(float3 earthCenter, float earthRadiusAddCloudLayerStart,
									   float3 pt, float3 projectedPt, float layer_thickness)
	   {
		   float t = distance(pt, earthCenter);
		   t -= earthRadiusAddCloudLayerStart;
		   return saturate(max(t, 0.0f) / layer_thickness);
	   }

	   float PackFloat16(float value)
	   {
		   return value * 0.001f;
	   }

	   static float UnPackFloat16(float value)
	   {
		   return value * 1000.0f;
	   }

	   float getAtmosphereBlendForComposite(float distance)
	   {
		   float rate = mix(0.75f, 0.4f, saturate(VolumetricCloudsCBuffer.g_VolumetricClouds.m_MaxSampleDistance / MAX_SAMPLE_STEP_DISTANCE));
		   float Threshold = VolumetricCloudsCBuffer.g_VolumetricClouds.m_MaxSampleDistance * rate;
		   float InvThreshold = VolumetricCloudsCBuffer.g_VolumetricClouds.m_MaxSampleDistance - Threshold;
		   return saturate(max(distance * MAX_SAMPLE_STEP_DISTANCE - Threshold, 0.0f) / InvThreshold);
	   }

	   float2 Rotation(float2 pos, float theta)
	   {
		   float cosTheta = cos(theta);
		   float sinTheta = sin(theta);
		   return float2(pos.x * cosTheta - pos.y * sinTheta, pos.x * sinTheta + pos.y * cosTheta);
	   }

	   float GetDensityHeightGradientForPoint(float relativeHeight, float cloudType)
	   {
		   float cumulus = max(0.0, (RemapClamped(relativeHeight, 0.01, 0.15, 0.0, 1.0) * RemapClamped(relativeHeight, 0.9, 0.95, 1.0, 0.0)));
		   float stratocumulus = max(0.0, (RemapClamped(relativeHeight, 0.0, 0.15, 0.0, 1.0) * RemapClamped(relativeHeight, 0.3, 0.65, 1.0, 0.0)));
		   float stratus = max(0.0, (RemapClamped(relativeHeight, 0.0, 0.1, 0.0, 1.0) * RemapClamped(relativeHeight, 0.2, 0.3, 1.0, 0.0)));
		   float cloudType2 = (cloudType * (float)(2.0));
		   float a = mix(stratus, stratocumulus, saturate(cloudType2));
		   float b = mix(stratocumulus, cumulus, saturate((cloudType2 - (float)(1.0))));
		   return mix(a, b, round(cloudType));
	   };

	   float SampleDensity(float3 worldPos, float lod, float height_fraction, float3 currentProj, float LayerThickness,
						   float CloudSize, float BaseShapeTiling, float CloudCoverage, float CloudType, float AnvilBias,
						   float CurlStrenth, float CurlTextureTiling,
						   float DetailStrenth,
						   float RisingVaporUpDirection, float RisingVaporScale, float RisingVaporIntensity,
						   float3 cloudTopOffsetWithWindDir, float4 windWithVelocity,
						   float3 biasedCloudPos, float DetailShapeTilingDivCloudSize,
						   float WeatherTextureOffsetX, float WeatherTextureOffsetZ, float WeatherTextureSize,
						   float RotationPivotOffsetX, float RotationPivotOffsetZ,
						   float RotationAngle,
						   bool cheap)
	   {
		   float3 unwindWorldPos = worldPos;
		   (worldPos += ((float3)(height_fraction) * cloudTopOffsetWithWindDir));
		   (worldPos += biasedCloudPos);

		   float2 weatherMapUV = (unwindWorldPos.xz + windWithVelocity.xy + float2(WeatherTextureOffsetX, WeatherTextureOffsetZ)) / (WeatherTextureSize);

		   float2 RotationOffset = float2(RotationPivotOffsetX, RotationPivotOffsetZ);
		   weatherMapUV -= RotationOffset;
		   weatherMapUV = Rotation(weatherMapUV, RotationAngle);
		   weatherMapUV += RotationOffset;

		   float4 weatherData = weatherTexture.sample(g_LinearWrapSampler, weatherMapUV, level(0.0));

		   float3 worldPosDivCloudSize = (worldPos / (float3)CloudSize);

		   float4 low_freq_noises = lowFreqNoiseTexture.sample(g_LinearWrapSampler, (worldPosDivCloudSize * (float3)BaseShapeTiling), level(lod));
		   float low_freq_fBm = ((((low_freq_noises).g * (float)(0.625)) + ((low_freq_noises).b * (float)(0.25))) + ((low_freq_noises).a * (float)(0.125)));

		   float base_cloud = RemapClamped((low_freq_noises).r, (low_freq_fBm - (float)(1.0)), 1.0, 0.0, 1.0);
		   (base_cloud = saturate((base_cloud + CloudCoverage)));

		   float cloudType = saturate(((weatherData).g + CloudType));
		   float density_height_gradient = GetDensityHeightGradientForPoint(height_fraction, cloudType);
		   (base_cloud *= density_height_gradient);
		   
		   float cloud_coverage = saturate((weatherData).r);
		   (cloud_coverage = pow(cloud_coverage, RemapClamped(height_fraction, 0.2, 0.8, 1.0, mix(1.0, 0.5, AnvilBias))));

		   float base_cloud_coverage = RemapClamped(base_cloud, cloud_coverage, 1.0, 0.0, 1.0);
		   (base_cloud_coverage *= cloud_coverage);
		   float final_cloud = base_cloud_coverage;
		   if ((!cheap))
		   {
			   float2 curl_noise = curlNoiseTexture.sample(g_LinearWrapSampler, float2(((worldPosDivCloudSize).xz * (float2)(CurlTextureTiling))), level(0.0)).rg;
			   ((worldPos).xz += ((curl_noise * (float2)(((float)(1.0) - height_fraction))) * (float2)(CurlStrenth)));

			   float3 high_frequency_noises = highFreqNoiseTexture.sample(g_LinearWrapSampler, float3((worldPos * (float3)(DetailShapeTilingDivCloudSize))), level(HIGH_FREQ_LOD)).rgb;
			   float high_freq_fBm = ((((high_frequency_noises).r * (float)(0.625)) + ((high_frequency_noises).g * (float)(0.25))) + ((high_frequency_noises).b * (float)(0.125)));

			   float height_fraction_new = getRelativeHeight(worldPos, currentProj, LayerThickness);
			   float height_freq_noise_modifier = mix(high_freq_fBm, ((float)(1.0) - high_freq_fBm), saturate((height_fraction_new * (float)(10.0))));

			   (final_cloud = RemapClamped(base_cloud_coverage, (height_freq_noise_modifier * DetailStrenth), 1.0, 0.0, 1.0));
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

	   float SampleEnergy( float3 rayPos, float3 magLightDirection, float height_fraction, float3 currentProj,
						   float3 earthCenter,
						   float earthRadiusAddCloudLayerStart,
						   float layerThickness,
						   float CloudSize, float BaseShapeTiling, float CloudCoverage, float CloudType, float AnvilBias,
						   float CurlStrenth, float CurlTextureTiling,
						   float DetailStrenth,
						   float RisingVaporUpDirection, float RisingVaporScale, float RisingVaporIntensity,
						   float3 cloudTopOffsetWithWindDir, float4 windWithVelocity,
						   float3 biasedCloudPos, float DetailShapeTilingDivCloudSize,
						   float WeatherTextureOffsetX, float WeatherTextureOffsetZ, float WeatherTextureSize,
						   float RotationPivotOffsetX, float RotationPivotOffsetZ,
						   float RotationAngle,
						   float ds_loded, float stepSize, float cosTheta, float mipBias)
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
			   
			   float3 currentProj_new = getProjectedShellPoint(earthRadiusAddCloudLayerStart, samplePoint, earthCenter);
			   float height_fraction_new = getRelativeHeight(samplePoint, currentProj_new, layerThickness);

			   totalSample += SampleDensity(   samplePoint, mipmapOffset, height_fraction_new, currentProj_new, layerThickness,
											   CloudSize, BaseShapeTiling, CloudCoverage, CloudType, AnvilBias,
											   CurlStrenth, CurlTextureTiling,
											   DetailStrenth,
											   RisingVaporUpDirection, RisingVaporScale, RisingVaporIntensity,
											   cloudTopOffsetWithWindDir, windWithVelocity,
											   biasedCloudPos, DetailShapeTilingDivCloudSize,
											   WeatherTextureOffsetX, WeatherTextureOffsetZ, WeatherTextureSize,
											   RotationPivotOffsetX, RotationPivotOffsetZ,
											   RotationAngle,
											   false);

			   (mipmapOffset += (float)(0.5));
			   (step += 1.0);
		   }

		   float hg = max(HenryGreenstein((VolumetricCloudsCBuffer.g_VolumetricClouds).Eccentricity, cosTheta), (saturate(HenryGreenstein((0.99 - (VolumetricCloudsCBuffer.g_VolumetricClouds).SilverliningSpread), cosTheta)))) * (VolumetricCloudsCBuffer.g_VolumetricClouds).SilverliningIntensity;
		   float dl = totalSample * VolumetricCloudsCBuffer.g_VolumetricClouds.Precipitation;
		   hg = hg / max(dl, 0.05);

		   float energy = GetLightEnergy(height_fraction, dl, ds_loded, hg, cosTheta, stepSize, (VolumetricCloudsCBuffer.g_VolumetricClouds).Contrast);
		   return energy;
	};
	
	float GetDensityWithComparingDepth( float3 startPos, float3 worldPos, float3 dir, float raymarchOffset,
	                                    thread float(& intensity), thread float(& atmosphericBlendFactor), thread float(& depth), float2 uv)
    {
        float3 sampleStart, sampleEnd;
        (depth = (float)(0.0));
        (intensity = (float)(0.0));
        (atmosphericBlendFactor = 0.0);
       
       // If the current view direction is not intersected with cloud's layers
	    if (!GetStartEndPointForRayMarching(startPos, dir, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart2, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd2, sampleStart, sampleEnd))
		    return 0.0;

        float horizon = abs((dir).y);

        uint sample_count = (uint)mix((float)VolumetricCloudsCBuffer.g_VolumetricClouds.MAX_ITERATION_COUNT, (float)VolumetricCloudsCBuffer.g_VolumetricClouds.MIN_ITERATION_COUNT, horizon);
	    float sample_step = mix(VolumetricCloudsCBuffer.g_VolumetricClouds.m_StepSize.y, VolumetricCloudsCBuffer.g_VolumetricClouds.m_StepSize.x, horizon);

        (depth = distance(sampleEnd, startPos));
        float distCameraToStart = distance(sampleStart, startPos);
        
        atmosphericBlendFactor = distCameraToStart / MAX_SAMPLE_STEP_DISTANCE;

        float sceneDepth = depthTexture.sample(g_LinearClampSampler, uv, level(0)).r;
        if (sceneDepth < 1.0f)
        {
          atmosphericBlendFactor = 1.0f;
        }

        float linearDepth = mix(50.0f, 100000000.0f, sceneDepth);

        if (((sampleStart).y < 0.0))
        {
            (intensity = (float)(0.0));
            return 1.0;
        }

        // Atmosphere Culling
        // we don't need to render the clouds where the background should be shown 100%
        // How the clouds should be blended depends on the user
        if (distCameraToStart >= VolumetricCloudsCBuffer.g_VolumetricClouds.m_MaxSampleDistance)
        {
            return 1.0;
        }

        // Horizontal Culling
        // The most of cases, we don't need to render the clouds below the horizon
        if (sampleStart.y < 0.0f)
        {
            return 1.0;
        }

        float transStepSize = VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.a;

        //float sceneDepth = depthTexture.read(texels.xy).r;
        float alpha = 0.0;
        (intensity = 0.0);
        bool detailedSample = false;
        int missedStepCount = 0;

        float bigStep = (sample_step * (float)(2.0));

        float3 smallStepMarching = ((float3)(sample_step) * dir);
        float3 bigStepMarching = ((float3)(bigStep) * dir);

        float3 raymarchingDistance = (smallStepMarching * (float3)(raymarchOffset));

        float3 magLightDirection = ((float3)(2.0) * ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);

        bool pickedFirstHit = false;

        float cosTheta = dot(dir, ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);
        float3 rayPos = sampleStart;

		float4 windWithVelocity = ((VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerLayer[0].StandardPosition);
        float3 biasedCloudPos = ((float3)(4.5) * (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerLayer[0].WindDirection).xyz + float3(0.0, 0.1, 0.0)));
        float3 cloudTopOffsetWithWindDir = ((float3)((VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerLayer[0].CloudTopOffset) * ((VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerLayer[0].WindDirection).xyz);

        float DetailShapeTilingDivCloudSize = ((VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerLayer[0].DetailShapeTiling / (VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerLayer[0].CloudSize);

       
        for (uint j = 0; (j < sample_count); (j++))
        {
            (rayPos += raymarchingDistance);
            if ((linearDepth < distance(startPos, rayPos)))
            {
                break;
            }

            float3 currentProj = getProjectedShellPoint(VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart, rayPos, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz);
		    float height_fraction = getRelativeHeightAccurate(VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart, rayPos, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness);

            if ((!detailedSample))
            {
                // Get the density from current rayPos
                float sampleResult = SampleDensity(rayPos, LOW_FREQ_LOD, height_fraction, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudSize, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].BaseShapeTiling, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudCoverage, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudType, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].AnvilBias,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CurlStrenth, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CurlTextureTiling,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].DetailStrenth,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporUpDirection, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporScale, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporIntensity,
                    cloudTopOffsetWithWindDir, windWithVelocity,
                    biasedCloudPos, DetailShapeTilingDivCloudSize,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureOffsetZ, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureSize,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationPivotOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationPivotOffsetZ,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationAngle,
                    true);

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
                // Get the density from current rayPos
                float sampleResult = SampleDensity(rayPos, LOW_FREQ_LOD, height_fraction, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudSize, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].BaseShapeTiling, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudCoverage, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudType, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].AnvilBias,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CurlStrenth, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CurlTextureTiling,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].DetailStrenth,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporUpDirection, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporScale, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporIntensity,
                    cloudTopOffsetWithWindDir, windWithVelocity,
                    biasedCloudPos, DetailShapeTilingDivCloudSize,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureOffsetZ, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureSize,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationPivotOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationPivotOffsetZ,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationAngle,
                    false);

                if (sampleResult == 0.0)
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
                    // If it hit the clouds, get the light enery from current rayPos and accumulate it
                    float sampledAlpha = sampleResult * VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudDensity;
                    float sampledEnergy = SampleEnergy(
                        rayPos, magLightDirection, height_fraction, currentProj,
                        VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz,
                        VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart,
                        VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness,
                        VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudSize, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].BaseShapeTiling, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudCoverage, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudType, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].AnvilBias,
                        VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CurlStrenth, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CurlTextureTiling,
                        VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].DetailStrenth,
                        VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporUpDirection, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporScale, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporIntensity,
                        cloudTopOffsetWithWindDir, windWithVelocity,
                        biasedCloudPos, DetailShapeTilingDivCloudSize,
                        VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureOffsetZ, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureSize,
                        VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationPivotOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationPivotOffsetZ,
                        VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationAngle,
                        sampleResult, transStepSize, cosTheta, LOW_FREQ_LOD);

                    float oneMinusAlpha = ((float)(1.0) - alpha);
                    (sampledAlpha *= oneMinusAlpha);
                    (intensity += (sampledAlpha * sampledEnergy));
                    (alpha += sampledAlpha);
                    if ((alpha >= 1.0))
                    {
                        (intensity /= alpha);
                        (depth = PackFloat16(depth));
                        return 1.0;
                    }
                }
                (raymarchingDistance = smallStepMarching);
            }
        }

        (depth = PackFloat16(depth));
        return alpha;
    };
	
    struct PSIn
    {
        float4 Position [[position]];
        float2 TexCoord;
        float2 VSray;
    };

    float4 main(PSIn input)
    {
        float2 db_uvs = (input).TexCoord;
        float2 ScreenUV = (input).TexCoord;
        ((ScreenUV).x += (VolumetricCloudsCBuffer.g_VolumetricClouds).m_CorrectU);
        ((ScreenUV).y += (VolumetricCloudsCBuffer.g_VolumetricClouds).m_CorrectV);
        float3 ScreenNDC;
        ((ScreenNDC).x = (((ScreenUV).x * (float)(2.0)) - (float)(1.0)));
        ((ScreenNDC).y = ((((float)(1.0) - (ScreenUV).y) * (float)(2.0)) - (float)(1.0)));

        float3 projectedPosition = float3((ScreenNDC).xy, 0.0);

        float4 worldPos = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerEye[0].m_ProjToWorldMat * float4(projectedPosition, 1.0);
	    float4 CameraPosition = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerEye[0].cameraPosition;

        (worldPos /= (float4)((worldPos).w));

        float3 viewDir = normalize(((worldPos).xyz - (CameraPosition).xyz));

        float intensity;
        float atmosphereBlendFactor;
        float depth;

        float randomSeed = mix(fract(randomFromScreenUV(db_uvs * VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.zw)), (VolumetricCloudsCBuffer.g_VolumetricClouds).Random00, (VolumetricCloudsCBuffer.g_VolumetricClouds).m_UseRandomSeed);
        float density = GetDensityWithComparingDepth(CameraPosition.xyz, worldPos.xyz, viewDir, randomSeed, intensity, atmosphereBlendFactor, depth, db_uvs);
        return float4(intensity, atmosphereBlendFactor, depth, density);
    };

    Fragment_Shader(
    texture3d<float> highFreqNoiseTexture,
    texture3d<float> lowFreqNoiseTexture,
    texture2d<float> curlNoiseTexture,
    texture2d<float> weatherTexture,
    texture2d<float> depthTexture,
	texture2d<float> g_LinearDepthTexture,
	sampler g_LinearClampSampler,
	sampler g_LinearWrapSampler,
	sampler g_PointClampSampler,
	sampler g_LinearBorderSampler,
	constant volumetricCloud::Uniforms_VolumetricCloudsCBuffer & VolumetricCloudsCBuffer) :
    highFreqNoiseTexture(highFreqNoiseTexture),
    lowFreqNoiseTexture(lowFreqNoiseTexture),
    curlNoiseTexture(curlNoiseTexture),
    weatherTexture(weatherTexture),
    depthTexture(depthTexture),
	g_LinearDepthTexture(g_LinearDepthTexture),
    g_LinearClampSampler(g_LinearClampSampler),
    g_LinearWrapSampler(g_LinearWrapSampler),
    g_PointClampSampler(g_PointClampSampler),
    g_LinearBorderSampler(g_LinearBorderSampler),
    VolumetricCloudsCBuffer(VolumetricCloudsCBuffer) {}
};

fragment float4 stageMain(
    Fragment_Shader::PSIn input [[stage_in]],
    constant volumetricCloud::GraphicsArgData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant volumetricCloud::GraphicsArgDataPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    Fragment_Shader::PSIn input0;
    input0.Position = float4(input.Position.xyz, 1.0 / input.Position.w);
    input0.TexCoord = input.TexCoord;
    input0.VSray = input.VSray;
    Fragment_Shader main(
    argBufferStatic.highFreqNoiseTexture,
    argBufferStatic.lowFreqNoiseTexture,
    argBufferStatic.curlNoiseTexture,
    argBufferStatic.weatherTexture,
    argBufferStatic.depthTexture,
	argBufferStatic.g_LinearDepthTexture,
	argBufferStatic.g_LinearClampSampler,
    argBufferStatic.g_LinearWrapSampler,
    argBufferStatic.g_PointClampSampler,
    argBufferStatic.g_LinearBorderSampler,
    argBufferPerFrame.VolumetricCloudsCBuffer);
    return main.main(input0);
}
