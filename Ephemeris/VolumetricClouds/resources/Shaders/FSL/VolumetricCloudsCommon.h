/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#ifndef VOLUMETRIC_CLOUDS_COMMON_H
#define VOLUMETRIC_CLOUDS_COMMON_H

#define TRANSMITTANCE_SAMPLE_STEP_COUNT 5         // Number of energy sample
#define MAX_SAMPLE_STEP_DISTANCE        200000.0f // When the ray-marching distance exceeds this number, it stops.
#define PI                              3.1415926535897932384626433832795f
#define PI2                             6.283185307179586476925286766559f
#define ONE_OVER_FOURPI                 0.07957747154594767f
#define THREE_OVER_16PI                 0.05968310365946075091333141126469f
#define USE_Ray_Distance                1
#define ALLOW_CONTROL_CLOUD             1
#define STEREO_INSTANCED                0
#define FLOAT16_MAX                     65500.0f
#define LOW_FREQ_LOD                    1.0f
#define HIGH_FREQ_LOD                   0.0f

STRUCT(DataPerEye)
{
	DATA(f4x4,   m_WorldToProjMat,     None); // Matrix for converting World to Projected Space for the first eye
	DATA(f4x4,   m_ProjToWorldMat,     None); // Matrix for converting Projected Space to World Matrix for the first eye
	DATA(f4x4,   m_ViewToWorldMat,     None); // Matrix for converting View Space to World for the first eye
	DATA(f4x4,   m_PrevWorldToProjMat, None); // Matrix for converting Previous Projected Space to World for the first eye. It is used for reprojection
	DATA(f4x4,   m_LightToProjMat,     None); // Matrix for converting Light to Projected Space Matrix for the first eye
	DATA(float4, cameraPosition,       None);
};

STRUCT(DataPerLayer)
{
	DATA(float,  CloudsLayerStart,                None);
	DATA(float,  EarthRadiusAddCloudsLayerStart,  None);
	DATA(float,  EarthRadiusAddCloudsLayerStart2, None);
	DATA(float,  EarthRadiusAddCloudsLayerEnd,    None);
	DATA(float,  EarthRadiusAddCloudsLayerEnd2,   None);
	DATA(float,  LayerThickness,                  None);
	//Cloud
	DATA(float,  CloudDensity,                    None); // The overall density of clouds. Using bigger value makes more dense clouds, but it also makes ray-marching artifact worse.
	DATA(float,  CloudCoverage,                   None); // The overall coverage of clouds. Using bigger value makes more parts of the sky be covered by clouds. But, it does not make clouds more dense
	DATA(float,  CloudType,                       None); // Add this value to control the overall clouds' type. 0.0 is for Stratus, 0.5 is for Stratocumulus, and 1.0 is for Cumulus.
	DATA(float,  CloudTopOffset,                  None); // Intensity of skewing clouds along the wind direction.
	DATA(float,  CloudSize,                       None); // Overall size of the clouds. Using bigger value generates larger chunks of clouds.
	DATA(float,  BaseShapeTiling,                 None); // Control the base shape of the clouds. Using bigger value makes smaller chunks of base clouds.
	DATA(float,  DetailShapeTiling,               None); // Control the detail shape of the clouds. Using bigger value makes smaller chunks of detail clouds.
	DATA(float,  DetailStrenth,                   None); // Intensify the detail of the clouds. It is possible to lose whole shape of the clouds if the user uses too high value of it.
	DATA(float,  CurlTextureTiling,               None); // Control the curl size of the clouds. Using bigger value makes smaller curl shapes.
	DATA(float,  CurlStrenth,                     None); // Intensify the curl effect.
	DATA(float,  AnvilBias,                       None); // Using lower value makes anvil shape.
	DATA(float,  PadA,                            None);
	DATA(float,  PadB,                            None);
	DATA(float,  PadC,                            None);
	DATA(float4, WindDirection,                   None);
	DATA(float4, StandardPosition,                None); // The current center location for applying wind
	DATA(float,  WeatherTextureSize,              None); // Control the size of Weather map, bigger value makes the world to be covered by larger clouds pattern.
	DATA(float,  WeatherTextureOffsetX,           None);
	DATA(float,  WeatherTextureOffsetZ,           None);
	DATA(float,  RotationPivotOffsetX,            None);
	DATA(float,  RotationPivotOffsetZ,            None);
	DATA(float,  RotationAngle,                   None);
	DATA(float,  RisingVaporScale,                None);
	DATA(float,  RisingVaporUpDirection,          None);
	DATA(float,  RisingVaporIntensity,            None);
	DATA(float,  PadD,                            None);
	DATA(float,  PadE,                            None);
	DATA(float,  PadF,                            None);
};

CBUFFER(VolumetricCloudsCBuffer, UPDATE_FREQ_PER_FRAME, b4, binding = 110)
{
	DATA(uint,         m_JitterX,              None); // the X offset of Re-projection
	DATA(uint,         m_JitterY,              None); // the Y offset of Re-projection
	DATA(uint,         MIN_ITERATION_COUNT,    None); // Minimum iteration number of ray-marching
	DATA(uint,         MAX_ITERATION_COUNT,    None); // Maximum iteration number of ray-marching
	DATA(DataPerEye,   m_DataPerEye[2],        None);
	DATA(DataPerLayer, m_DataPerLayer[2],      None);
	DATA(float4,       m_StepSize,             None); // Cap of the step size X: min, Y: max
	DATA(float4,       TimeAndScreenSize,      None); // X: EplasedTime, Y: RealTime, Z: FullWidth, W: FullHeight
	DATA(float4,       lightDirection,         None);
	DATA(float4,       lightColorAndIntensity, None);
	DATA(float4,       EarthCenter,            None);
	DATA(float,        EarthRadius,            None);
	DATA(float,        m_MaxSampleDistance,    None);
	DATA(float,        m_CorrectU,             None); // m_JitterX / FullWidth
	DATA(float,        m_CorrectV,             None); // m_JitterX / FullHeight			
	//Lighting
	DATA(float,        BackgroundBlendFactor,  None); // Blend clouds with the background, more background will be shown if this value is close to 0.0
	DATA(float,        Contrast,               None); // Contrast of the clouds' color 
	DATA(float,        Eccentricity,           None); // The bright highlights around the sun that the user needs at sunset
	DATA(float,        CloudBrightness,        None); // The brightness for clouds
	DATA(float,        Precipitation,          None);
	DATA(float,        SilverliningIntensity,  None); // Intensity of silver-lining
	DATA(float,        SilverliningSpread,     None); // Using bigger value spreads more silver-lining, but the intesity of it
	DATA(float,        Random00,               None); // Random seed for the first ray-marching offset
	DATA(float,        CameraFarClip,          None);
	DATA(uint,         EnabledDepthCulling,    None);
	DATA(uint,         EnabledLodDepthCulling, None);
	DATA(uint,         DepthMapWidth,          None);
	DATA(uint,         DepthMapHeight,         None);
	// VolumetricClouds' Light shaft
	DATA(uint,         GodNumSamples,          None); // Number of godray samples
	DATA(float,        GodrayMaxBrightness,    None);
	DATA(float,        GodrayExposure,         None); // Intensity of godray
	DATA(float,        GodrayDecay,            None); // Using smaller value, the godray brightness applied to each iteration is reduced. The level of reduction is also reduced per iteration.
	DATA(float,        GodrayDensity,          None); // The distance between each interation.
	DATA(float,        GodrayWeight,           None); // Using smaller value, the godray brightness applied to each iteration is reduced. The level of reduction is not changed.
	DATA(float,        m_UseRandomSeed,        None);
	DATA(float,        Test00,                 None);
	DATA(float,        Test01,                 None);
	DATA(float,        Test02,                 None);
	DATA(float,        Test03,                 None);
};

RES(Tex3D(float4),    highFreqNoiseTexture,         UPDATE_FREQ_NONE, t0,  binding = 0); //for detail
RES(Tex3D(float4),    lowFreqNoiseTexture,          UPDATE_FREQ_NONE, t1,  binding = 1); //for basic shape // R: Perlin-Worley, G: Worley, B: Worley, A: Worley
RES(Tex2D(float4),    curlNoiseTexture,             UPDATE_FREQ_NONE, t2,  binding = 2);
RES(Tex2D(float4),    weatherTexture,               UPDATE_FREQ_NONE, t3,  binding = 3);
RES(Tex2D(float4),    depthTexture,                 UPDATE_FREQ_NONE, t4,  binding = 4);
RES(Tex2D(float4),    LowResCloudTexture,           UPDATE_FREQ_NONE, t5,  binding = 5);
RES(Tex2D(float4),    g_PrevFrameTexture,           UPDATE_FREQ_NONE, t6,  binding = 6);
RES(Tex2D(float4),    g_LinearDepthTexture,         UPDATE_FREQ_NONE, t7,  binding = 7);
RES(Tex2D(float4),    g_PostProcessedTexture,       UPDATE_FREQ_NONE, t8,  binding = 8);
RES(Tex2D(float4),    g_PrevVolumetricCloudTexture, UPDATE_FREQ_NONE, t9,  binding = 9);
RES(Tex2D(float4),    g_GodrayTexture,              UPDATE_FREQ_NONE, t10, binding = 10);
RES(Tex2D(float4),    g_SrcTexture2D,               UPDATE_FREQ_NONE, t11, binding = 11);
RES(Tex2D(float4),    g_SkyBackgroudTexture,        UPDATE_FREQ_NONE, t12, binding = 12);
RES(Tex2D(float4),    g_BlurTexture,                UPDATE_FREQ_NONE, t13, binding = 13);
RES(Tex2D(float4),    InputTex,                     UPDATE_FREQ_NONE, t14, binding = 14);
RES(Tex2D(float4),    SrcTexture,                   UPDATE_FREQ_NONE, t15, binding = 15);
RES(RWTex2D(float4),  volumetricCloudsDstTexture,   UPDATE_FREQ_NONE, u0,  binding = 16);
RES(RWTex2D(float4),  OutputTex,                    UPDATE_FREQ_NONE, u1,  binding = 17);
RES(RWTex2D(float4),  SavePrevTexture,              UPDATE_FREQ_NONE, u2,  binding = 18);
RES(RWTex2D(float),   DstTexture,                   UPDATE_FREQ_NONE, u3,  binding = 19);
RES(RWBuffer(float4), TransmittanceColor,           UPDATE_FREQ_NONE, u4,  binding = 20);
RES(SamplerState,     g_LinearClampSampler,         UPDATE_FREQ_NONE, s0,  binding = 21);
RES(SamplerState,     g_LinearWrapSampler,          UPDATE_FREQ_NONE, s1,  binding = 22);
RES(SamplerState,     g_PointClampSampler,          UPDATE_FREQ_NONE, s2,  binding = 23);
RES(SamplerState,     g_LinearBorderSampler,        UPDATE_FREQ_NONE, s3,  binding = 24);

STATIC const float3 rand[TRANSMITTANCE_SAMPLE_STEP_COUNT + 1] = {
	{  0.0f,       0.0f,       0.0f      },
	{  0.612305f, -0.187500f,  0.28125f  },
	{  0.648437f,  0.026367f, -0.792969f },
	{ -0.636719f,  0.449219f, -0.539062f },
	{ -0.808594f,  0.748047f,  0.456055f },
	{  0.542969f,  0.351563f,  0.048462f }
};

float randomFromScreenUV(float2 uv) 
{
	return frac(sin(dot(uv.xy, float2(12.9898f, 78.233f))) * 43758.5453f);
}

// Reference from https://area.autodesk.com/blogs/game-dev-blog/volumetric-clouds/.
bool ray_trace_sphere(float3 center, float3 rd, float3 offset, float radius2, out(float) t1, out(float) t2)
{
	t1 = 0.0f;
	t2 = 0.0f;

	float3 p = center - offset;
	float  b = dot(p, rd);
	float  c = dot(p, p) - radius2;
	float  f = b * b - c;

	if (f >= 0.0f)
	{
		float sqrtF = sqrt(f);
		t1 = -b - sqrtF;
		t2 = -b + sqrtF;
	}

	return t1 > 0.0f || t2 > 0.0f;
}

// Find the intesections from start and end layers of clouds
bool GetStartEndPointForRayMarching(float3 ws_origin, float3 ws_ray, float3 earthCenter, float layerStart2, float LayerEnd2, out(float3) start, out(float3) end)
{
	float ot1, ot2, it1, it2;

	start = end = f3(0.0f);

	if (!ray_trace_sphere(ws_origin, ws_ray, earthCenter, LayerEnd2, ot1, ot2))
		return false;

	bool inIntersected = ray_trace_sphere(ws_origin, ws_ray, earthCenter, layerStart2, it1, it2);

	if (inIntersected)
	{
		float branchFactor = saturate(floor(it1 + 1.0f));

		start = ws_origin + lerp(max(it2, 0.0f), max(ot1, 0.0f), branchFactor) * ws_ray;
		end   = ws_origin + lerp(ot2, it1, branchFactor) * ws_ray;
	}
	else
	{
		start = ws_origin + max(ot1, 0.0f) * ws_ray;
		end   = ws_origin + ot2 * ws_ray;
	}

	return true;
}

// Remap the current value with new min/max values
float Remap(float original_value, float original_min, float original_max, float new_min, float new_max)
{
	return new_min + ((original_value - original_min) / (original_max - original_min)) * (new_max - new_min);
}

// The result of Remap function will be Clamped by [0 ... 1]
float RemapClamped(float original_value, float original_min, float original_max, float new_min, float new_max)
{
	return new_min + saturate((original_value - original_min) / (original_max - original_min)) * (new_max - new_min);
}

// Phase function for Volumetric Clouds
float HenryGreenstein(float g, float cosTheta) 
{
	float g2 = g * g;
	float numerator = 1.0f - g2;
	float denominator = pow(abs(1.0f + g2 - 2.0f * g * cosTheta), 1.5f);

	return ONE_OVER_FOURPI * numerator / denominator;
}

float Phase(float g, float g2, float cosTheta, float y)
{
	return lerp(HenryGreenstein(g, cosTheta), HenryGreenstein(g2, cosTheta), y);
}

// Get the point projected to the inner atmosphere shell
float3 getProjectedShellPoint(float earthRadiusAddCloudLayerStart, float3 pt, float3 center)
{
	return earthRadiusAddCloudLayerStart * normalize(pt - center) + center;
}

// Get the relative height approximately
float getRelativeHeight(float3 pt, float3 projectedPt, float layer_thickness)
{
	return saturate(length(pt - projectedPt) / layer_thickness);
}

// Get the relative height accurately
float getRelativeHeightAccurate(float3 earthCenter, float earthRadiusAddCloudLayerStart, float3 pt, float3 projectedPt, float layer_thickness)
{
	float t = distance(pt, earthCenter);
	t -= earthRadiusAddCloudLayerStart;

	return saturate(max(t, 0.0f) / layer_thickness);
}

float PackFloat16(float value)
{
	return value * 0.001f;
}

float UnPackFloat16(float value)
{
	return value * 1000.0f;
}

float getAtmosphereBlendForComposite(float distance)
{
	float rate = lerp(0.75f, 0.4f, saturate(Get(m_MaxSampleDistance) / MAX_SAMPLE_STEP_DISTANCE));
	float Threshold = Get(m_MaxSampleDistance) * rate;
	float InvThreshold = Get(m_MaxSampleDistance) - Threshold;

	return saturate(max(distance * MAX_SAMPLE_STEP_DISTANCE - Threshold, 0.0f) / InvThreshold);
}

float GetLODBias(float distance)
{
	float factor = 50000.0f;

	return saturate(max(distance - factor, 0.0f) / (MAX_SAMPLE_STEP_DISTANCE - factor));
}

float2 Rotation(float2 pos, float theta)
{
	float cosTheta = cos(theta);
	float sinTheta = sin(theta);

	return float2(pos.x * cosTheta - pos.y * sinTheta, pos.x * sinTheta + pos.y * cosTheta);
}

// Determine which type of clouds is going to be used
float GetDensityHeightGradientForPoint(float relativeHeight, float cloudType)
{
	float cumulus = max(0.0f, RemapClamped(relativeHeight, 0.01f, 0.15f, 0.0f, 1.0f) * RemapClamped(relativeHeight, 0.9f, 0.95f, 1.0f, 0.0f));
	float stratocumulus = max(0.0f, RemapClamped(relativeHeight, 0.0f, 0.15f, 0.0f, 1.0f) * RemapClamped(relativeHeight, 0.3f, 0.65f, 1.0f, 0.0f));
	float stratus = max(0.0f, RemapClamped(relativeHeight, 0.0f, 0.1f, 0.0f, 1.0f) * RemapClamped(relativeHeight, 0.2f, 0.3f, 1.0f, 0.0f));
	float cloudType2 = cloudType * 2.0f;

	float a = lerp(stratus, stratocumulus, saturate(cloudType2));
	float b = lerp(stratocumulus, cumulus, saturate(cloudType2 - 1.0f));

	return lerp(a, b, round(cloudType));
}

// Get the density of clouds from current ray-marched position
float SampleDensity(float3 worldPos, float lod, float height_fraction, float3 currentProj, float LayerThickness,
	float CloudSize, float BaseShapeTiling, float CloudCoverage, float CloudType, 
	float AnvilBias, float CurlStrenth, float CurlTextureTiling, float DetailStrenth,
	float RisingVaporUpDirection, float RisingVaporScale, float RisingVaporIntensity, float3 cloudTopOffsetWithWindDir,
	float4 windWithVelocity, float3 biasedCloudPos, float DetailShapeTilingDivCloudSize, float WeatherTextureOffsetX, 
	float WeatherTextureOffsetZ, float WeatherTextureSize, float RotationPivotOffsetX, float RotationPivotOffsetZ,
	float RotationAngle, bool cheap)
{
	// Skew in wind direction	
	// Unwind position only for weatherData
	float3 unwindWorldPos = worldPos;

	worldPos += height_fraction * cloudTopOffsetWithWindDir;
	worldPos += biasedCloudPos;

	float2 RotationOffset = float2(RotationPivotOffsetX, RotationPivotOffsetZ);

	float2 weatherMapUV = (unwindWorldPos.xz + windWithVelocity.xy + float2(WeatherTextureOffsetX, WeatherTextureOffsetZ)) / WeatherTextureSize;
	weatherMapUV -= RotationOffset;
	weatherMapUV  = Rotation(weatherMapUV, RotationAngle);
	weatherMapUV += RotationOffset;

	float4 weatherData = SampleLvlTex2D(Get(weatherTexture), Get(g_LinearWrapSampler), weatherMapUV, 0);

	float3 worldPosDivCloudSize = worldPos / CloudSize;

	// Get the density of base cloud 
	float4 low_freq_noises = SampleLvlTex3D(Get(lowFreqNoiseTexture), Get(g_LinearWrapSampler), worldPosDivCloudSize * BaseShapeTiling, lod);
	float  low_freq_fBm    = (low_freq_noises.g * 0.625f) + (low_freq_noises.b * 0.25f) + (low_freq_noises.a * 0.125f);

	float base_cloud = RemapClamped(low_freq_noises.r, low_freq_fBm - 1.0f, 1.0f, 0.0f, 1.0f);
	base_cloud = saturate(base_cloud + CloudCoverage);

	// Get the density from current height and type of clouds
	float cloudType = saturate(weatherData.g + CloudType);
	float density_height_gradient = GetDensityHeightGradientForPoint(height_fraction, cloudType);

	// Apply the height function to the base cloud shape
	base_cloud *= density_height_gradient;

	float cloud_coverage = saturate(weatherData.r);
	cloud_coverage = pow(cloud_coverage, RemapClamped(height_fraction, 0.2f, 0.8f, 1.0f, lerp(1.0f, 0.5f, AnvilBias)));

	float base_cloud_coverage = RemapClamped(base_cloud, cloud_coverage, 1.0f, 0.0f, 1.0f);
	base_cloud_coverage *= cloud_coverage;

	float final_cloud = base_cloud_coverage;

	if (!cheap)
	{
		// Apply the curl noise
		float2 curl_noise = SampleLvlTex2D(Get(curlNoiseTexture), Get(g_LinearWrapSampler), float2(worldPosDivCloudSize.xz * CurlTextureTiling), 1.0f).rg;

		worldPos.xz += curl_noise * (1.0f - height_fraction) * CurlStrenth;
		worldPos.y  -= RisingVaporUpDirection * ((Get(TimeAndScreenSize).x * RisingVaporIntensity) / (CloudSize * 0.0657f * RisingVaporScale));

		float3 uvw = float3((worldPos + float3(windWithVelocity.z, 0.0f, windWithVelocity.w)) * DetailShapeTilingDivCloudSize);

		// Get the density of base cloud 
		float3 high_frequency_noises = SampleLvlTex3D(Get(highFreqNoiseTexture), Get(g_LinearWrapSampler), uvw, HIGH_FREQ_LOD).rgb;
			
		float high_freq_fBm = (high_frequency_noises.r * 0.625f) + (high_frequency_noises.g * 0.25f) + (high_frequency_noises.b * 0.125f);

		float height_fraction_new = getRelativeHeight(worldPos, currentProj, LayerThickness);
		float height_freq_noise_modifier = lerp(high_freq_fBm, 1.0f - high_freq_fBm, saturate(height_fraction_new * 10.0f));

		final_cloud = RemapClamped(base_cloud_coverage, height_freq_noise_modifier * DetailStrenth, 1.0f, 0.0f, 1.0f);
	}

	return final_cloud;
}

// Get the light energy
float GetLightEnergy(float height_fraction, float dl, float ds_loded, float phase_probability, float cos_angle, float step_size, float contrast)
{
	//attenuation (Beer's Law)
	float primary_att   = exp(-dl);
	float secondary_att = exp(-dl * 0.25f) * 0.7f;

	float attenuation_probability = max(primary_att, secondary_att * 0.25f);

	//in-scattering
	float depth_probability = lerp(0.05f + pow(ds_loded, RemapClamped(height_fraction, 0.3f, 0.85f, 0.5f, 2.0f)), 1.0f, saturate(dl));
	float vertical_probability = pow(RemapClamped(height_fraction, 0.07f, 0.14f, 0.1f, 1.0f), 0.8f);
	float in_scatter_probability = vertical_probability * depth_probability;
	float light_energy = attenuation_probability + in_scatter_probability * phase_probability;
	light_energy = pow(abs(light_energy), contrast);

	return light_energy;
}

// Sample the positions to get the light energy
float SampleEnergy(float3 rayPos, float3 magLightDirection, float height_fraction, float3 currentProj, float3 earthCenter,
	float earthRadiusAddCloudLayerStart, float layerThickness, float CloudSize, float BaseShapeTiling, 
	float CloudCoverage, float CloudType, float AnvilBias, float CurlStrenth, 
	float CurlTextureTiling, float DetailStrenth, float RisingVaporUpDirection, float RisingVaporScale, 
	float RisingVaporIntensity, float3 cloudTopOffsetWithWindDir, float4 windWithVelocity, float3 biasedCloudPos, 
	float DetailShapeTilingDivCloudSize, float WeatherTextureOffsetX, float WeatherTextureOffsetZ, float WeatherTextureSize,
	float RotationPivotOffsetX, float RotationPivotOffsetZ, float RotationAngle, float ds_loded, 
	float stepSize, float cosTheta, float mipBias)
{
	float totalSample  = 0.0f;
	float mipmapOffset = mipBias;
	float step = 0.5f;

	UNROLL
	for (int i = 0; i < TRANSMITTANCE_SAMPLE_STEP_COUNT; ++i)
	{
		float3 rand3 = rand[i];

		float3 direction = magLightDirection + normalize(rand3);
		direction = normalize(direction);

		float3 samplePoint = rayPos + (step * stepSize * direction);

		float3 currentProj_new = getProjectedShellPoint(earthRadiusAddCloudLayerStart, samplePoint, earthCenter);
		float height_fraction_new = getRelativeHeight(samplePoint, currentProj_new, layerThickness);

		totalSample += SampleDensity(samplePoint, mipmapOffset, height_fraction_new, currentProj_new, layerThickness, CloudSize, BaseShapeTiling, 
			CloudCoverage, CloudType, AnvilBias, CurlStrenth, CurlTextureTiling,
			DetailStrenth, RisingVaporUpDirection, RisingVaporScale, RisingVaporIntensity, cloudTopOffsetWithWindDir, 
			windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, WeatherTextureOffsetX, WeatherTextureOffsetZ, 
			WeatherTextureSize, RotationPivotOffsetX, RotationPivotOffsetZ, RotationAngle, false);

		// Increase the mipmap offset to gain performance
		mipmapOffset += 0.5f;
		step += 1.0f;
	}

	float hg = max(HenryGreenstein(Get(Eccentricity), cosTheta), saturate(HenryGreenstein(0.99f - Get(SilverliningSpread), cosTheta))) * Get(SilverliningIntensity);
	float dl = totalSample * Get(Precipitation);
	hg /= max(dl, 0.05f);

	float energy = GetLightEnergy(height_fraction, dl, ds_loded, hg, cosTheta, stepSize, Get(Contrast));

	return energy;
}

// Get the final density, light intensirt, atmophereBlendFactor, and distance between hit points against clouds and view position
float GetDensity(float3 startPos, float3 worldPos, float3 dir, float raymarchOffset, out(float) intensity, out(float) atmosphericBlendFactor, out(float) depth, float2 uv)
{
	float3 sampleStart, sampleEnd;

	depth = 0.0f;
	intensity = 0.0f;
	atmosphericBlendFactor = 0.0f;

	// If the current view direction is not intersected with cloud's layers
	if (!GetStartEndPointForRayMarching(startPos, dir, Get(EarthCenter).xyz, Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart2, Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerEnd2, sampleStart, sampleEnd))
		return 0.0f;

	// Determine the sample count and its step size along the cosTheta of view direction with Up vector 
	float horizon = abs(dir.y);
	uint  sample_count = uint(lerp(float(Get(MAX_ITERATION_COUNT)), float(Get(MIN_ITERATION_COUNT)), horizon));
	float sample_step  = lerp(Get(m_StepSize).y, Get(m_StepSize).x, horizon);

	// Update the distance between hit points against clouds and view position
	depth = distance(sampleEnd, startPos);

	float distCameraToStart = distance(sampleStart, startPos);
	atmosphericBlendFactor  = distCameraToStart / MAX_SAMPLE_STEP_DISTANCE;

	// Atmosphere Culling		
	// we don't need to render the clouds where the background should be shown 100%
	// How the clouds should be blended depends on the user 
	if (distCameraToStart >= Get(m_MaxSampleDistance))
	{
		return 1.0f;
	}

	// Horizontal Culling		
	// The most of cases, we don't need to render the clouds below the horizon
	if (sampleStart.y < 0.0f)
	{
		return 1.0f;
	}

	float transStepSize = Get(lightDirection).a;

	// Depth Culling
	// Get texel coordinates for Depth culling
	uint2 texels = uint2(float2(Get(DepthMapWidth), Get(DepthMapHeight)) * uv);

	// Get the lodded depth, if it is not using, use far distance instead to pass the culling
	float sceneDepth;

	if (float(Get(EnabledLodDepthCulling)) > 0.5f)
	{
		sceneDepth = LoadTex2D(Get(depthTexture), NO_SAMPLER, texels, 0).r;

		if (sceneDepth < 1.0f)
		{
			return 1.0f;
		}
	}

	// Prepare to do raymarching
	float alpha = 0.0f;
	bool detailedSample = false; // start with cheap raymarching
	int missedStepCount = 0;

	float bigStep = sample_step * 2.0f; // lerp(2.0f, 2.0f, horizon); // pow(2.0f, cheapLOD);

	float3 smallStepMarching = sample_step * dir;
	float3 bigStepMarching   = bigStep * dir;

	// To prevent raymarching artifact, use a random value
	float3 raymarchingDistance = smallStepMarching * raymarchOffset;

	float3 magLightDirection = 2.0f * Get(lightDirection).xyz;

	bool pickedFirstHit = false;

	float cosTheta = dot(dir, Get(lightDirection).xyz);
	float3 rayPos = sampleStart;

	float4 windWithVelocity = Get(m_DataPerLayer)[0].StandardPosition;
	float3 biasedCloudPos = 4.5f * (Get(m_DataPerLayer)[0].WindDirection.xyz + float3(0.0f, 0.1f, 0.0f));
	float3 cloudTopOffsetWithWindDir = Get(m_DataPerLayer)[0].CloudTopOffset * Get(m_DataPerLayer)[0].WindDirection.xyz;

	float DetailShapeTilingDivCloudSize = Get(m_DataPerLayer)[0].DetailShapeTiling / Get(m_DataPerLayer)[0].CloudSize;

	LOOP
	for (uint j = 0; j < sample_count; ++j)
	{
		rayPos += raymarchingDistance;

		float3 currentProj = getProjectedShellPoint(Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart, rayPos, Get(EarthCenter).xyz);
		float height_fraction = getRelativeHeightAccurate(Get(EarthCenter).xyz, Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart, rayPos, currentProj, Get(m_DataPerLayer)[0].LayerThickness);

		if (!detailedSample)
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(rayPos, LOW_FREQ_LOD, height_fraction, currentProj, Get(m_DataPerLayer)[0].LayerThickness,
				Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
				Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
				Get(m_DataPerLayer)[0].DetailStrenth,
				Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
				cloudTopOffsetWithWindDir, windWithVelocity,
				biasedCloudPos, DetailShapeTilingDivCloudSize,
				Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
				Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[0].RotationAngle,
				true);

			if (sampleResult > 0.0f)
			{
				// If it hit the clouds, change to the expensive raymarching
				detailedSample = true;
				raymarchingDistance = -bigStepMarching;
				missedStepCount = 0;
				continue;
			}
			else
			{
				raymarchingDistance = bigStepMarching;
			}
		}
		else
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(rayPos, LOW_FREQ_LOD, height_fraction, currentProj, Get(m_DataPerLayer)[0].LayerThickness,
				Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
				Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
				Get(m_DataPerLayer)[0].DetailStrenth,
				Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
				cloudTopOffsetWithWindDir, windWithVelocity,
				biasedCloudPos, DetailShapeTilingDivCloudSize,
				Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
				Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[0].RotationAngle,
				false);

			if (sampleResult == 0.0f)
			{
				missedStepCount++;

				// If expensive raymarching failed more 10 times, go back to cheap raymarching
				if (missedStepCount > 10)
				{
					detailedSample = false;
				}
			}
			else
			{
				// Get the first hit position against clouds to use it for reprojection
				if (!pickedFirstHit)
				{
					depth = distance(rayPos, startPos);
					pickedFirstHit = true;
				}

				// If it hit the clouds, get the light enery from current rayPos and accumulate it
				float sampledAlpha = sampleResult * Get(m_DataPerLayer)[0].CloudDensity;
				float sampledEnergy = SampleEnergy(rayPos, magLightDirection, height_fraction, currentProj,
					Get(EarthCenter).xyz,
					Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart,
					Get(m_DataPerLayer)[0].LayerThickness,
					Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
					Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
					Get(m_DataPerLayer)[0].DetailStrenth,
					Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
					cloudTopOffsetWithWindDir, windWithVelocity,
					biasedCloudPos, DetailShapeTilingDivCloudSize,
					Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
					Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
					Get(m_DataPerLayer)[0].RotationAngle,
					sampleResult, transStepSize, cosTheta, LOW_FREQ_LOD);

				float oneMinusAlpha = 1.0f - alpha;
				sampledAlpha *= oneMinusAlpha;

				intensity += sampledAlpha * sampledEnergy;
				alpha += sampledAlpha;

				if (alpha >= 1.0f)
				{
					intensity /= alpha;
					depth = PackFloat16(depth);

					return 1.0f;
				}
			}

			raymarchingDistance = smallStepMarching;
		}
	}

	depth = PackFloat16(depth);

	return alpha;
}

// Get the final density, light intensirt, atmophereBlendFactor, and distance between hit points against clouds and view position
float GetDensityWithComparingDepth(float3 startPos, float3 worldPos, float3 dir, float raymarchOffset, out(float) intensity, out(float) atmosphericBlendFactor, out(float) depth, float2 uv)
{
	float3 sampleStart, sampleEnd;

	depth = 0.0f;
	intensity = 0.0f;
	atmosphericBlendFactor = 0.0f;

	// If the current view direction is not intersected with cloud's layers	
	if (!GetStartEndPointForRayMarching(startPos, dir, Get(EarthCenter).xyz, Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart2, Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerEnd2, sampleStart, sampleEnd))
		return 0.0f;

	// Determine the sample count and its step size along the cosTheta of view direction with Up vector 
	float horizon = abs(dir.y);

	uint sample_count = uint(lerp(float(Get(MAX_ITERATION_COUNT)), float(Get(MIN_ITERATION_COUNT)), horizon));
	float sample_step = lerp(Get(m_StepSize).y, Get(m_StepSize).x, horizon);

	// Update the distance between hit points against clouds and view position
	depth = distance(sampleEnd, startPos);
	float distCameraToStart = distance(sampleStart, startPos);

	atmosphericBlendFactor = distCameraToStart / MAX_SAMPLE_STEP_DISTANCE;

	//atmosphericBlendFactor = PackFloat16(distCameraToStart);
	float sceneDepth = SampleLvlTex2D(Get(depthTexture), Get(g_LinearClampSampler), uv, 0).r;

	if (sceneDepth < 1.0f)
	{
		atmosphericBlendFactor = 1.0f;
	}

	float linearDepth = lerp(50.0f, 100000000.0f, sceneDepth);

	// Horizontal Culling		
	// The most of cases, we don't need to render the clouds below the horizon
	if (sampleStart.y < 0.0f)
	{
		return 1.0f;
	}

	// Atmosphere Culling		
	// we don't need to render the clouds where the background should be shown 100%
	// How the clouds should be blended depends on the user 
	if (distCameraToStart >= Get(m_MaxSampleDistance))
	{
		return 1.0f;
	}

	float transStepSize = Get(lightDirection).a;

	// Prepare to do raymarching
	float alpha = 0.0f;
	bool detailedSample = false; // start with cheap raymarching
	int missedStepCount = 0;

	float bigStep = sample_step * 2.0f; //pow(2.0f, cheapLOD);

	float3 smallStepMarching = sample_step * dir;
	float3 bigStepMarching   = bigStep * dir;

	// To prevent raymarching artifact, use a random value
	float3 raymarchingDistance = smallStepMarching * raymarchOffset;

	float3 magLightDirection = 2.0f * Get(lightDirection).xyz;

	bool pickedFirstHit = false;

	float cosTheta = dot(dir, Get(lightDirection).xyz);
	float3 rayPos = sampleStart;

	float4 windWithVelocity = Get(m_DataPerLayer)[0].StandardPosition;
	float3 biasedCloudPos = 4.5f * (Get(m_DataPerLayer)[0].WindDirection.xyz + float3(0.0f, 0.1f, 0.0f));
	float3 cloudTopOffsetWithWindDir = Get(m_DataPerLayer)[0].CloudTopOffset * Get(m_DataPerLayer)[0].WindDirection.xyz;

	float DetailShapeTilingDivCloudSize = Get(m_DataPerLayer)[0].DetailShapeTiling / Get(m_DataPerLayer)[0].CloudSize;

	LOOP
	for (uint j = 0; j < sample_count; ++j)
	{
		rayPos += raymarchingDistance;
		if (linearDepth < distance(startPos, rayPos))
		{
			break;
		}

		float3 currentProj = getProjectedShellPoint(Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart, rayPos, Get(EarthCenter).xyz);
		float height_fraction = getRelativeHeightAccurate(Get(EarthCenter).xyz, Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart, rayPos, currentProj, Get(m_DataPerLayer)[0].LayerThickness);

		if (!detailedSample)
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(rayPos, LOW_FREQ_LOD, height_fraction, currentProj, Get(m_DataPerLayer)[0].LayerThickness,
				Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
				Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
				Get(m_DataPerLayer)[0].DetailStrenth,
				Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
				cloudTopOffsetWithWindDir, windWithVelocity,
				biasedCloudPos, DetailShapeTilingDivCloudSize,
				Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
				Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[0].RotationAngle,
				true);

			if (sampleResult > 0.0f)
			{
				// If it hit the clouds, change to the expensive raymarching
				detailedSample = true;
				raymarchingDistance = -bigStepMarching;
				missedStepCount = 0;
				continue;
			}
			else
			{
				raymarchingDistance = bigStepMarching;
			}
		}
		else
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(rayPos, LOW_FREQ_LOD, height_fraction, currentProj, Get(m_DataPerLayer)[0].LayerThickness,
				Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
				Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
				Get(m_DataPerLayer)[0].DetailStrenth,
				Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
				cloudTopOffsetWithWindDir, windWithVelocity,
				biasedCloudPos, DetailShapeTilingDivCloudSize,
				Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
				Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[0].RotationAngle,
				false);

			if (sampleResult == 0.0f)
			{
				missedStepCount++;

				// If expensive raymarching failed more 10 times, go back to cheap raymarching
				if (missedStepCount > 10)
				{
					detailedSample = false;
				}
			}
			else
			{
				// Get the first hit position against clouds to use it for reprojection
				if (!pickedFirstHit)
				{
					depth = distance(rayPos, startPos);
					pickedFirstHit = true;
				}

				// If it hit the clouds, get the light enery from current rayPos and accumulate it
				float sampledAlpha = sampleResult * Get(m_DataPerLayer)[0].CloudDensity;
				float sampledEnergy = SampleEnergy(
					rayPos, magLightDirection, height_fraction, currentProj,
					Get(EarthCenter).xyz,
					Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart,
					Get(m_DataPerLayer)[0].LayerThickness,
					Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
					Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
					Get(m_DataPerLayer)[0].DetailStrenth,
					Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
					cloudTopOffsetWithWindDir, windWithVelocity,
					biasedCloudPos, DetailShapeTilingDivCloudSize,
					Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
					Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
					Get(m_DataPerLayer)[0].RotationAngle,
					sampleResult, transStepSize, cosTheta, LOW_FREQ_LOD);

				float oneMinusAlpha = 1.0f - alpha;
				sampledAlpha *= oneMinusAlpha;

				intensity += sampledAlpha * sampledEnergy;
				alpha += sampledAlpha;

				if (alpha >= 1.0f)
				{
					intensity /= alpha;
					depth = PackFloat16(depth);

					return 1.0f;
				}
			}

			raymarchingDistance = smallStepMarching;
		}
	}

	depth = PackFloat16(depth);

	return alpha;
}


// Get the final density, light intensirt, atmophereBlendFactor, and distance between hit points against clouds and view position
float GetDensity_Double_Layers(float3 startPos, float3 worldPos, float3 dir, float raymarchOffset, out(float) intensity, out(float) atmosphericBlendFactor, out(float) depth, float2 uv)
{
	float3 sampleStart, sampleEnd;

	depth = 0.0f;
	intensity = 0.0f;
	atmosphericBlendFactor = 0.0f;

	// If the current view direction is not intersected with cloud's layers
	if (!GetStartEndPointForRayMarching(startPos, dir, Get(EarthCenter).xyz, 
		    min(Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart2, Get(m_DataPerLayer)[1].EarthRadiusAddCloudsLayerStart2),
		    max(Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerEnd2,   Get(m_DataPerLayer)[1].EarthRadiusAddCloudsLayerEnd2), sampleStart, sampleEnd))
		return 0.0f;

	// Determine the sample count and its step size along the cosTheta of view direction with Up vector 
	float horizon = abs(dir.y);

	uint sample_count = uint(lerp(float(Get(MAX_ITERATION_COUNT)), float(Get(MIN_ITERATION_COUNT)), horizon));
	//float sample_step = min(length(sampleEnd - sampleStart) / float(sample_count) * 0.43f, lerp(Get(m_StepSize).y, Get(m_StepSize).x, pow(horizon, 0.33f)));
	float sample_step = lerp(Get(m_StepSize).y, Get(m_StepSize).x, horizon);

	// Update the distance between hit points against clouds and view position
	depth = distance(startPos, sampleEnd);

	float distCameraToStart = distance(sampleStart, startPos);
	atmosphericBlendFactor  = distCameraToStart / MAX_SAMPLE_STEP_DISTANCE;

	// Horizontal Culling		
	// The most of cases, we don't need to render the clouds below the horizon
	if (sampleStart.y < 0.0f)
	{
		return 1.0f;
	}

	// Atmosphere Culling		
	// we don't need to render the clouds where the background should be shown 100%
	// How the clouds should be blended depends on the user 
	if (distCameraToStart >= Get(m_MaxSampleDistance))
	{
		return 1.0f;
	}

	float transStepSize = Get(lightDirection).a;

	// Depth Culling
	// Get texel coordinates for Depth culling
	uint2 texels = uint2(float2(Get(DepthMapWidth), Get(DepthMapHeight)) * uv);

	//Get the lodded depth, if it is not using, use far distance instead to pass the culling
	float sceneDepth;

	if (float(Get(EnabledLodDepthCulling)) > 0.5f)
	{
		sceneDepth = LoadTex2D(Get(depthTexture), NO_SAMPLER, texels, 0).r;

		if (sceneDepth < 1.0f)
		{
			return 1.0f;
		}
	}

	// Prepare to do raymarching
	float alpha = 0.0f;
	bool detailedSample = false; // start with cheap raymarching
	int missedStepCount = 0;

	float bigStep = sample_step * 2.0f; //pow(2.0f, cheapLOD);

	float3 smallStepMarching = sample_step * dir;
	float3 bigStepMarching = bigStep * dir;

	// To prevent raymarching artifact, use a random value
	float3 raymarchingDistance = smallStepMarching * raymarchOffset;

	float3 magLightDirection = 2.0f * Get(lightDirection).xyz;

	bool pickedFirstHit = false;

	float cosTheta = dot(dir, Get(lightDirection).xyz);
	float3 rayPos = sampleStart;

	float4 windWithVelocity     = Get(m_DataPerLayer)[0].StandardPosition;
	float4 windWithVelocity_2nd = Get(m_DataPerLayer)[1].StandardPosition;

	float3 biasedCloudPos     = 4.5f * (Get(m_DataPerLayer)[0].WindDirection.xyz + float3(0.0f, 0.1f, 0.0f));
	float3 biasedCloudPos_2nd = 4.5f * (Get(m_DataPerLayer)[1].WindDirection.xyz + float3(0.0f, 0.1f, 0.0f));

	float3 cloudTopOffsetWithWindDir     = Get(m_DataPerLayer)[0].CloudTopOffset * Get(m_DataPerLayer)[0].WindDirection.xyz;
	float3 cloudTopOffsetWithWindDir_2nd = Get(m_DataPerLayer)[1].CloudTopOffset * Get(m_DataPerLayer)[1].WindDirection.xyz;

	float DetailShapeTilingDivCloudSize     = Get(m_DataPerLayer)[0].DetailShapeTiling / Get(m_DataPerLayer)[0].CloudSize;
	float DetailShapeTilingDivCloudSize_2nd = Get(m_DataPerLayer)[1].DetailShapeTiling / Get(m_DataPerLayer)[1].CloudSize;

	LOOP
	for (uint j = 0; j < sample_count; ++j)
	{
		rayPos += raymarchingDistance;

		float3 currentProj    = getProjectedShellPoint(Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart, rayPos, Get(EarthCenter).xyz);
		float height_fraction = getRelativeHeightAccurate(Get(EarthCenter).xyz, Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart, rayPos, currentProj, Get(m_DataPerLayer)[0].LayerThickness);

		float3 currentProj_2nd    = getProjectedShellPoint(Get(m_DataPerLayer)[1].EarthRadiusAddCloudsLayerStart, rayPos, Get(EarthCenter).xyz);
		float height_fraction_2nd = getRelativeHeightAccurate(Get(EarthCenter).xyz, Get(m_DataPerLayer)[1].EarthRadiusAddCloudsLayerStart, rayPos, currentProj_2nd, Get(m_DataPerLayer)[1].LayerThickness);

		if (!detailedSample)
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction, currentProj, Get(m_DataPerLayer)[0].LayerThickness,
				Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
				Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
				Get(m_DataPerLayer)[0].DetailStrenth,
				Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
				cloudTopOffsetWithWindDir, windWithVelocity,
				biasedCloudPos, DetailShapeTilingDivCloudSize,
				Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
				Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[0].RotationAngle,
				true);

			float sampleResult_2nd = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction_2nd, currentProj_2nd, Get(m_DataPerLayer)[1].LayerThickness,
				Get(m_DataPerLayer)[1].CloudSize, Get(m_DataPerLayer)[1].BaseShapeTiling, Get(m_DataPerLayer)[1].CloudCoverage, Get(m_DataPerLayer)[1].CloudType, Get(m_DataPerLayer)[1].AnvilBias,
				Get(m_DataPerLayer)[1].CurlStrenth, Get(m_DataPerLayer)[1].CurlTextureTiling,
				Get(m_DataPerLayer)[1].DetailStrenth,
				Get(m_DataPerLayer)[1].RisingVaporUpDirection, Get(m_DataPerLayer)[1].RisingVaporScale, Get(m_DataPerLayer)[1].RisingVaporIntensity,
				cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
				biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
				Get(m_DataPerLayer)[1].WeatherTextureOffsetX, Get(m_DataPerLayer)[1].WeatherTextureOffsetZ, Get(m_DataPerLayer)[1].WeatherTextureSize,
				Get(m_DataPerLayer)[1].RotationPivotOffsetX, Get(m_DataPerLayer)[1].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[1].RotationAngle,
				true);

			if (sampleResult > 0.0f || sampleResult_2nd > 0.0f)
			{
				// If it hit the clouds, change to the expensive raymarching
				detailedSample = true;
				raymarchingDistance = -bigStepMarching;
				missedStepCount = 0;
				continue;
			}
			else
			{
				raymarchingDistance = bigStepMarching;
			}
		}
		else
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction, currentProj, Get(m_DataPerLayer)[0].LayerThickness,
				Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
				Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
				Get(m_DataPerLayer)[0].DetailStrenth,
				Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
				cloudTopOffsetWithWindDir, windWithVelocity,
				biasedCloudPos, DetailShapeTilingDivCloudSize,
				Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
				Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[0].RotationAngle,
				false);

			float sampleResult_2nd = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction_2nd, currentProj_2nd, Get(m_DataPerLayer)[1].LayerThickness,
				Get(m_DataPerLayer)[1].CloudSize, Get(m_DataPerLayer)[1].BaseShapeTiling, Get(m_DataPerLayer)[1].CloudCoverage, Get(m_DataPerLayer)[1].CloudType, Get(m_DataPerLayer)[1].AnvilBias,
				Get(m_DataPerLayer)[1].CurlStrenth, Get(m_DataPerLayer)[1].CurlTextureTiling,
				Get(m_DataPerLayer)[1].DetailStrenth,
				Get(m_DataPerLayer)[1].RisingVaporUpDirection, Get(m_DataPerLayer)[1].RisingVaporScale, Get(m_DataPerLayer)[1].RisingVaporIntensity,
				cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
				biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
				Get(m_DataPerLayer)[1].WeatherTextureOffsetX, Get(m_DataPerLayer)[1].WeatherTextureOffsetZ, Get(m_DataPerLayer)[1].WeatherTextureSize,
				Get(m_DataPerLayer)[1].RotationPivotOffsetX, Get(m_DataPerLayer)[1].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[1].RotationAngle,
				false);

			if (sampleResult == 0.0f && sampleResult_2nd == 0.0f)
			{
				missedStepCount++;

				// If expensive raymarching failed more 10 times, go back to cheap raymarching
				if (missedStepCount > 10)
				{
					detailedSample = false;
				}
			}
			else
			{
				// Get the first hit position against clouds to use it for reprojection
				if (!pickedFirstHit)
				{
					depth = distance(rayPos, startPos);
					pickedFirstHit = true;
				}

				if (sampleResult > 0.0f)
				{
					// If it hit the clouds, get the light enery from current rayPos and accumulate it
					float sampledAlpha = sampleResult * Get(m_DataPerLayer)[0].CloudDensity;
					float sampledEnergy = SampleEnergy(
						rayPos, magLightDirection, height_fraction, currentProj,
						Get(EarthCenter).xyz,
						Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart,
						Get(m_DataPerLayer)[0].LayerThickness,
						Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
						Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
						Get(m_DataPerLayer)[0].DetailStrenth,
						Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
						cloudTopOffsetWithWindDir, windWithVelocity,
						biasedCloudPos, DetailShapeTilingDivCloudSize,
						Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
						Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
						Get(m_DataPerLayer)[0].RotationAngle,
						sampleResult, transStepSize, cosTheta, LOW_FREQ_LOD);

					float oneMinusAlpha = 1.0f - alpha;
					sampledAlpha *= oneMinusAlpha;

					intensity += sampledAlpha * sampledEnergy;
					alpha += sampledAlpha;
				}

				if (sampleResult_2nd > 0.0f)
				{
					// If it hit the clouds, get the light enery from current rayPos and accumulate it
					float sampledAlpha = sampleResult_2nd * Get(m_DataPerLayer)[1].CloudDensity;
					float sampledEnergy = SampleEnergy(
						rayPos, magLightDirection, height_fraction_2nd, currentProj_2nd,
						Get(EarthCenter).xyz,
						Get(m_DataPerLayer)[1].EarthRadiusAddCloudsLayerStart,
						Get(m_DataPerLayer)[1].LayerThickness,
						Get(m_DataPerLayer)[1].CloudSize, Get(m_DataPerLayer)[1].BaseShapeTiling, Get(m_DataPerLayer)[1].CloudCoverage, Get(m_DataPerLayer)[1].CloudType, Get(m_DataPerLayer)[1].AnvilBias,
						Get(m_DataPerLayer)[1].CurlStrenth, Get(m_DataPerLayer)[1].CurlTextureTiling,
						Get(m_DataPerLayer)[1].DetailStrenth,
						Get(m_DataPerLayer)[1].RisingVaporUpDirection, Get(m_DataPerLayer)[1].RisingVaporScale, Get(m_DataPerLayer)[1].RisingVaporIntensity,
						cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
						biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
						Get(m_DataPerLayer)[1].WeatherTextureOffsetX, Get(m_DataPerLayer)[1].WeatherTextureOffsetZ, Get(m_DataPerLayer)[1].WeatherTextureSize,
						Get(m_DataPerLayer)[1].RotationPivotOffsetX, Get(m_DataPerLayer)[1].RotationPivotOffsetZ,
						Get(m_DataPerLayer)[1].RotationAngle,
						sampleResult_2nd, transStepSize, cosTheta, LOW_FREQ_LOD);

					float oneMinusAlpha = 1.0f - alpha;
					sampledAlpha *= oneMinusAlpha;

					intensity += sampledAlpha * sampledEnergy;
					alpha += sampledAlpha;
				}

				if (alpha >= 1.0f)
				{
					intensity /= alpha;
					depth = PackFloat16(depth);

					return 1.0f;
				}
			}

			raymarchingDistance = smallStepMarching;
		}
	}

	depth = PackFloat16(depth);

	return alpha;
}

// Get the final density, light intensirt, atmophereBlendFactor, and distance between hit points against clouds and view position
float GetDensity_Double_Layers_WithComparingDepth(float3 startPos, float3 worldPos, float3 dir, float raymarchOffset, out(float) intensity, out(float) atmosphericBlendFactor, out(float) depth, float2 uv)
{
	float3 sampleStart, sampleEnd;

	depth = 0.0f;
	intensity = 0.0f;
	atmosphericBlendFactor = 0.0f;

	// If the current view direction is not intersected with cloud's layers
	if (!GetStartEndPointForRayMarching(startPos, dir, Get(EarthCenter).xyz,
		    min(Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart2, Get(m_DataPerLayer)[1].EarthRadiusAddCloudsLayerStart2),
		    max(Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerEnd2,   Get(m_DataPerLayer)[1].EarthRadiusAddCloudsLayerEnd2), sampleStart, sampleEnd))
		return 0.0f;

	// Determine the sample count and its step size along the cosTheta of view direction with Up vector 
	float horizon = abs(dir.y);

	uint sample_count = uint(lerp(float(Get(MAX_ITERATION_COUNT)), float(Get(MIN_ITERATION_COUNT)), horizon));
	//float sample_step = min(length(sampleEnd - sampleStart) / (float)sample_count * 0.43f, lerp(Get(m_StepSize).y, Get(m_StepSize).x, pow(horizon, 0.33f)));
	float sample_step = lerp(Get(m_StepSize).y, Get(m_StepSize).x, horizon);

	// Update the distance between hit points against clouds and view position
	depth = distance(startPos, sampleEnd);

	float distCameraToStart = distance(sampleStart, startPos);
	atmosphericBlendFactor  = distCameraToStart / MAX_SAMPLE_STEP_DISTANCE;

	float sceneDepth = SampleLvlTex2D(Get(depthTexture), Get(g_LinearClampSampler), uv, 0).r;

	if (sceneDepth < 1.0f)
	{
		atmosphericBlendFactor = 1.0f;
	}

	float linearDepth = lerp(50.0f, 100000000.0f, sceneDepth);

	// Horizontal Culling		
	// The most of cases, we don't need to render the clouds below the horizon
	if (sampleStart.y < 0.0f)
	{
		return 1.0f;
	}

	// Atmosphere Culling		
	// we don't need to render the clouds where the background should be shown 100%
	// How the clouds should be blended depends on the user 
	if (distCameraToStart >= Get(m_MaxSampleDistance))
	{
		return 1.0f;
	}

	float transStepSize = Get(lightDirection).a;

	// Prepare to do raymarching
	float alpha = 0.0f;
	bool detailedSample = false; // start with cheap raymarching
	int missedStepCount = 0;

	float bigStep = sample_step * 2.0f; //pow(2.0f, cheapLOD);

	float3 smallStepMarching = sample_step * dir;
	float3 bigStepMarching   = bigStep * dir;

	// To prevent raymarching artifact, use a random value
	float3 raymarchingDistance = smallStepMarching * raymarchOffset;

	float3 magLightDirection = 2.0f * Get(lightDirection).xyz;

	bool pickedFirstHit = false;

	float cosTheta = dot(dir, Get(lightDirection).xyz);
	float3 rayPos  = sampleStart;

	float4 windWithVelocity     = Get(m_DataPerLayer)[0].StandardPosition;
	float4 windWithVelocity_2nd = Get(m_DataPerLayer)[1].StandardPosition;

	float3 biasedCloudPos     = 4.5f * (Get(m_DataPerLayer)[0].WindDirection.xyz + float3(0.0f, 0.1f, 0.0f));
	float3 biasedCloudPos_2nd = 4.5f * (Get(m_DataPerLayer)[1].WindDirection.xyz + float3(0.0f, 0.1f, 0.0f));

	float3 cloudTopOffsetWithWindDir     = Get(m_DataPerLayer)[0].CloudTopOffset * Get(m_DataPerLayer)[0].WindDirection.xyz;
	float3 cloudTopOffsetWithWindDir_2nd = Get(m_DataPerLayer)[1].CloudTopOffset * Get(m_DataPerLayer)[1].WindDirection.xyz;

	float DetailShapeTilingDivCloudSize     = Get(m_DataPerLayer)[0].DetailShapeTiling / Get(m_DataPerLayer)[0].CloudSize;
	float DetailShapeTilingDivCloudSize_2nd = Get(m_DataPerLayer)[1].DetailShapeTiling / Get(m_DataPerLayer)[1].CloudSize;

	LOOP
	for (uint j = 0; j < sample_count; ++j)
	{
		rayPos += raymarchingDistance;

		if (linearDepth < distance(startPos, rayPos))
		{
			break;
		}

		float3 currentProj    = getProjectedShellPoint(Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart, rayPos, Get(EarthCenter).xyz);
		float height_fraction = getRelativeHeightAccurate(Get(EarthCenter).xyz, Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart, rayPos, currentProj, Get(m_DataPerLayer)[0].LayerThickness);

		float3 currentProj_2nd    = getProjectedShellPoint(Get(m_DataPerLayer)[1].EarthRadiusAddCloudsLayerStart, rayPos, Get(EarthCenter).xyz);
		float height_fraction_2nd = getRelativeHeightAccurate(Get(EarthCenter).xyz, Get(m_DataPerLayer)[1].EarthRadiusAddCloudsLayerStart, rayPos, currentProj_2nd, Get(m_DataPerLayer)[1].LayerThickness);

		if (!detailedSample)
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction, currentProj, Get(m_DataPerLayer)[0].LayerThickness,
				Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
				Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
				Get(m_DataPerLayer)[0].DetailStrenth,
				Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
				cloudTopOffsetWithWindDir, windWithVelocity,
				biasedCloudPos, DetailShapeTilingDivCloudSize,
				Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
				Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[0].RotationAngle,
				true);

			float sampleResult_2nd = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction_2nd, currentProj_2nd, Get(m_DataPerLayer)[1].LayerThickness,
				Get(m_DataPerLayer)[1].CloudSize, Get(m_DataPerLayer)[1].BaseShapeTiling, Get(m_DataPerLayer)[1].CloudCoverage, Get(m_DataPerLayer)[1].CloudType, Get(m_DataPerLayer)[1].AnvilBias,
				Get(m_DataPerLayer)[1].CurlStrenth, Get(m_DataPerLayer)[1].CurlTextureTiling,
				Get(m_DataPerLayer)[1].DetailStrenth,
				Get(m_DataPerLayer)[1].RisingVaporUpDirection, Get(m_DataPerLayer)[1].RisingVaporScale, Get(m_DataPerLayer)[1].RisingVaporIntensity,
				cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
				biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
				Get(m_DataPerLayer)[1].WeatherTextureOffsetX, Get(m_DataPerLayer)[1].WeatherTextureOffsetZ, Get(m_DataPerLayer)[1].WeatherTextureSize,
				Get(m_DataPerLayer)[1].RotationPivotOffsetX, Get(m_DataPerLayer)[1].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[1].RotationAngle,
				true);

			if (sampleResult > 0.0f || sampleResult_2nd > 0.0f)
			{
				// If it hit the clouds, change to the expensive raymarching
				detailedSample = true;
				raymarchingDistance = -bigStepMarching;
				missedStepCount = 0;
				continue;
			}
			else
			{
				raymarchingDistance = bigStepMarching;
			}
		}
		else
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction, currentProj, Get(m_DataPerLayer)[0].LayerThickness,
				Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
				Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
				Get(m_DataPerLayer)[0].DetailStrenth,
				Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
				cloudTopOffsetWithWindDir, windWithVelocity,
				biasedCloudPos, DetailShapeTilingDivCloudSize,
				Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
				Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[0].RotationAngle,
				false);

			float sampleResult_2nd = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction_2nd, currentProj_2nd, Get(m_DataPerLayer)[1].LayerThickness,
				Get(m_DataPerLayer)[1].CloudSize, Get(m_DataPerLayer)[1].BaseShapeTiling, Get(m_DataPerLayer)[1].CloudCoverage, Get(m_DataPerLayer)[1].CloudType, Get(m_DataPerLayer)[1].AnvilBias,
				Get(m_DataPerLayer)[1].CurlStrenth, Get(m_DataPerLayer)[1].CurlTextureTiling,
				Get(m_DataPerLayer)[1].DetailStrenth,
				Get(m_DataPerLayer)[1].RisingVaporUpDirection, Get(m_DataPerLayer)[1].RisingVaporScale, Get(m_DataPerLayer)[1].RisingVaporIntensity,
				cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
				biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
				Get(m_DataPerLayer)[1].WeatherTextureOffsetX, Get(m_DataPerLayer)[1].WeatherTextureOffsetZ, Get(m_DataPerLayer)[1].WeatherTextureSize,
				Get(m_DataPerLayer)[1].RotationPivotOffsetX, Get(m_DataPerLayer)[1].RotationPivotOffsetZ,
				Get(m_DataPerLayer)[1].RotationAngle,
				false);

			if (sampleResult == 0.0f && sampleResult_2nd == 0.0f)
			{
				missedStepCount++;

				// If expensive raymarching failed more 10 times, go back to cheap raymarching
				if (missedStepCount > 10)
				{
					detailedSample = false;
				}
			}
			else
			{
				// Get the first hit position against clouds to use it for reprojection
				if (!pickedFirstHit)
				{
					depth = distance(rayPos, startPos);
					pickedFirstHit = true;
				}

				if (sampleResult > 0.0f)
				{
					// If it hit the clouds, get the light enery from current rayPos and accumulate it
					float sampledAlpha = sampleResult * Get(m_DataPerLayer)[0].CloudDensity;
					float sampledEnergy = SampleEnergy(
						rayPos, magLightDirection, height_fraction, currentProj,
						Get(EarthCenter).xyz,
						Get(m_DataPerLayer)[0].EarthRadiusAddCloudsLayerStart,
						Get(m_DataPerLayer)[0].LayerThickness,
						Get(m_DataPerLayer)[0].CloudSize, Get(m_DataPerLayer)[0].BaseShapeTiling, Get(m_DataPerLayer)[0].CloudCoverage, Get(m_DataPerLayer)[0].CloudType, Get(m_DataPerLayer)[0].AnvilBias,
						Get(m_DataPerLayer)[0].CurlStrenth, Get(m_DataPerLayer)[0].CurlTextureTiling,
						Get(m_DataPerLayer)[0].DetailStrenth,
						Get(m_DataPerLayer)[0].RisingVaporUpDirection, Get(m_DataPerLayer)[0].RisingVaporScale, Get(m_DataPerLayer)[0].RisingVaporIntensity,
						cloudTopOffsetWithWindDir, windWithVelocity,
						biasedCloudPos, DetailShapeTilingDivCloudSize,
						Get(m_DataPerLayer)[0].WeatherTextureOffsetX, Get(m_DataPerLayer)[0].WeatherTextureOffsetZ, Get(m_DataPerLayer)[0].WeatherTextureSize,
						Get(m_DataPerLayer)[0].RotationPivotOffsetX, Get(m_DataPerLayer)[0].RotationPivotOffsetZ,
						Get(m_DataPerLayer)[0].RotationAngle,
						sampleResult, transStepSize, cosTheta, LOW_FREQ_LOD);

					float oneMinusAlpha = 1.0f - alpha;
					sampledAlpha *= oneMinusAlpha;

					intensity += sampledAlpha * sampledEnergy;
					alpha += sampledAlpha;
				}

				if (sampleResult_2nd > 0.0f)
				{
					// If it hit the clouds, get the light enery from current rayPos and accumulate it
					float sampledAlpha = sampleResult_2nd * Get(m_DataPerLayer)[1].CloudDensity;
					float sampledEnergy = SampleEnergy(
						rayPos, magLightDirection, height_fraction_2nd, currentProj_2nd,
						Get(EarthCenter).xyz,
						Get(m_DataPerLayer)[1].EarthRadiusAddCloudsLayerStart,
						Get(m_DataPerLayer)[1].LayerThickness,
						Get(m_DataPerLayer)[1].CloudSize, Get(m_DataPerLayer)[1].BaseShapeTiling, Get(m_DataPerLayer)[1].CloudCoverage, Get(m_DataPerLayer)[1].CloudType, Get(m_DataPerLayer)[1].AnvilBias,
						Get(m_DataPerLayer)[1].CurlStrenth, Get(m_DataPerLayer)[1].CurlTextureTiling,
						Get(m_DataPerLayer)[1].DetailStrenth,
						Get(m_DataPerLayer)[1].RisingVaporUpDirection, Get(m_DataPerLayer)[1].RisingVaporScale, Get(m_DataPerLayer)[1].RisingVaporIntensity,
						cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
						biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
						Get(m_DataPerLayer)[1].WeatherTextureOffsetX, Get(m_DataPerLayer)[1].WeatherTextureOffsetZ, Get(m_DataPerLayer)[1].WeatherTextureSize,
						Get(m_DataPerLayer)[1].RotationPivotOffsetX, Get(m_DataPerLayer)[1].RotationPivotOffsetZ,
						Get(m_DataPerLayer)[1].RotationAngle,
						sampleResult_2nd, transStepSize, cosTheta, LOW_FREQ_LOD);

					float oneMinusAlpha = 1.0f - alpha;
					sampledAlpha *= oneMinusAlpha;

					intensity += sampledAlpha * sampledEnergy;
					alpha += sampledAlpha;
				}

				if (alpha >= 1.0f)
				{
					intensity /= alpha;
					depth = PackFloat16(depth);

					return 1.0f;
				}
			}

			raymarchingDistance = smallStepMarching;
		}
	}

	depth = PackFloat16(depth);

	return alpha;
}

// Sample previous information and return the result that projected texture coordinates are out bounded or not
float4 SamplePrev(float2 prevUV, out(float) outOfBound)
{
	prevUV.xy  = float2((prevUV.x + 1.0f) * 0.5f, (1.0f - prevUV.y) * 0.5f);
	outOfBound = step(0.0f, max(max(-prevUV.x, -prevUV.y), max(prevUV.x, prevUV.y) - 1.0f));

	return SampleTex2D(Get(g_PrevFrameTexture), Get(g_LinearClampSampler), prevUV.xy);
}

// Correct current texture coordinates and get the result of Volumetric clouds's rendering 
float4 SampleCurrent(float2 uv, float2 _Jitter)
{
	uv = uv - (_Jitter - 1.5f) * (1.0f / Get(TimeAndScreenSize).zw);

	return SampleLvlTex2D(Get(LowResCloudTexture), Get(g_PointClampSampler), uv, 0);
}

// Check whether current texture coordinates should be updated or not
float ShouldbeUpdated(float2 uv, float2 jitter)
{
	float2 texelRelativePos = fmod(uv * Get(TimeAndScreenSize).zw, 4.0f);
	texelRelativePos -= jitter;
	float2 valid = saturate(2.0f * (f2(0.5f) - abs(texelRelativePos - f2(0.5f))));

	return valid.x * valid.y;
}

#endif // VOLUMETRIC_CLOUDS_COMMON_H