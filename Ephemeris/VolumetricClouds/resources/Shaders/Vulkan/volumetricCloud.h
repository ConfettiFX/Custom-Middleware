/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#define TRANSMITTANCE_SAMPLE_STEP_COUNT							5													// Number of energy sample
#define MAX_SAMPLE_STEP_DISTANCE								200000.0											// When the ray-marching distance exceeds this number, it stops.

#define PI														3.1415926535897932384626433832795
#define PI2														6.283185307179586476925286766559
#define ONE_OVER_FOURPI											0.07957747154594767
#define THREE_OVER_16PI											0.05968310365946075091333141126469

#define USE_Ray_Distance										1
#define ALLOW_CONTROL_CLOUD									1
#define STEREO_INSTANCED										0

#define FLOAT16_MAX												65500.0f 
#define LOW_FREQ_LOD											1.0f
#define HIGH_FREQ_LOD											0.0f

struct DataPerEye
{
	mat4			m_WorldToProjMat;			// Matrix for converting World to Projected Space for the first eye
	mat4			m_ProjToWorldMat;			// Matrix for converting Projected Space to World Matrix for the first eye
	mat4			m_ViewToWorldMat;			// Matrix for converting View Space to World for the first eye
	mat4			m_PrevWorldToProjMat;	// Matrix for converting Previous Projected Space to World for the first eye (it is used for reprojection)
	mat4			m_LightToProjMat;			// Matrix for converting Light to Projected Space Matrix for the first eye
	vec4			cameraPosition;
};

struct DataPerLayer
{
	float					CloudsLayerStart;
	float					EarthRadiusAddCloudsLayerStart;
	float					EarthRadiusAddCloudsLayerStart2;
	float					EarthRadiusAddCloudsLayerEnd;
	float					EarthRadiusAddCloudsLayerEnd2;
	float					LayerThickness;

	//Cloud
	float					CloudDensity;							// The overall density of clouds. Using bigger value makes more dense clouds, but it also makes ray-marching artifact worse.
	float					CloudCoverage;						// The overall coverage of clouds. Using bigger value makes more parts of the sky be covered by clouds. (But, it does not make clouds more dense)
	float					CloudType;								// Add this value to control the overall clouds' type. 0.0 is for Stratus, 0.5 is for Stratocumulus, and 1.0 is for Cumulus.

	float					CloudTopOffset;						// Intensity of skewing clouds along the wind direction.

	//Modeling
	float					CloudSize;								// Overall size of the clouds. Using bigger value generates larger chunks of clouds.

	float					BaseShapeTiling;					// Control the base shape of the clouds. Using bigger value makes smaller chunks of base clouds.
	float					DetailShapeTiling;				// Control the detail shape of the clouds. Using bigger value makes smaller chunks of detail clouds.

	float					DetailStrenth;						// Intensify the detail of the clouds. It is possible to lose whole shape of the clouds if the user uses too high value of it.
	float					CurlTextureTiling;				// Control the curl size of the clouds. Using bigger value makes smaller curl shapes.

	float					CurlStrenth;							// Intensify the curl effect.
	float					AnvilBias;								// Using lower value makes anvil shape.

	vec4					WindDirection;
	vec4					StandardPosition;					// The current center location for applying wind

	float					WeatherTextureSize;				// Control the size of Weather map, bigger value makes the world to be covered by larger clouds pattern.
	float					WeatherTextureOffsetX;

	float					WeatherTextureOffsetZ;
	float					RotationPivotOffsetX;

	float					RotationPivotOffsetZ;
	float					RotationAngle;

	float					RisingVaporScale;
	float					RisingVaporUpDirection;
	float					RisingVaporIntensity;
};

struct VolumetricCloudsCB
{
  uint					m_JitterX;								// the X offset of Re-projection
	uint					m_JitterY;								// the Y offset of Re-projection
	uint					MIN_ITERATION_COUNT;			// Minimum iteration number of ray-marching
	uint					MAX_ITERATION_COUNT;			// Maximum iteration number of ray-marching

	DataPerEye		m_DataPerEye[2];
	DataPerLayer	m_DataPerLayer[2];

	vec4					m_StepSize;								// Cap of the step size X: min, Y: max
	vec4					TimeAndScreenSize;				// X: EplasedTime, Y: RealTime, Z: FullWidth, W: FullHeight
	vec4					lightDirection;
	vec4					lightColorAndIntensity;

	vec4					EarthCenter;
	float					EarthRadius;

	float					m_MaxSampleDistance;

	float					m_CorrectU;								// m_JitterX / FullWidth
	float					m_CorrectV;								// m_JitterX / FullHeight			

	//Lighting
	float					BackgroundBlendFactor;		// Blend clouds with the background, more background will be shown if this value is close to 0.0
	float					Contrast;									// Contrast of the clouds' color 
	float					Eccentricity;							// The bright highlights around the sun that the user needs at sunset
	float					CloudBrightness;					// The brightness for clouds

	float					Precipitation;
	float					SilverliningIntensity;		// Intensity of silver-lining
	float					SilverliningSpread;				// Using bigger value spreads more silver-lining, but the intesity of it
	float					Random00;									// Random seed for the first ray-marching offset

	float					CameraFarClip;
	uint					EnabledDepthCulling;
	uint					EnabledLodDepthCulling;
	uint					DepthMapWidth;

	uint					DepthMapHeight;
	// VolumetricClouds' Light shaft
	uint					GodNumSamples;						// Number of godray samples

	float					GodrayMaxBrightness;
	float					GodrayExposure;						// Intensity of godray

	float					GodrayDecay;							// Using smaller value, the godray brightness applied to each iteration is reduced. The level of reduction is also reduced per iteration.
	float					GodrayDensity;						// The distance between each interation.
	float					GodrayWeight;							// Using smaller value, the godray brightness applied to each iteration is reduced. The level of reduction is not changed.
	float					m_UseRandomSeed;

	float					Test00;
	float					Test01;
	float					Test02;
	float					Test03;
};
layout(set = 0, binding = 0) uniform texture3D highFreqNoiseTexture;
layout(set = 0, binding = 1) uniform texture3D lowFreqNoiseTexture;
layout(set = 0, binding = 2) uniform texture2D curlNoiseTexture;
layout(set = 0, binding = 3) uniform texture2D weatherTexture;
layout(set = 0, binding = 4) uniform texture2D depthTexture;
layout(set = 0, binding = 5) uniform texture2D LowResCloudTexture;
layout(set = 0, binding = 6) uniform texture2D g_PrevFrameTexture;
layout(set = 0, binding = 7) uniform texture2D g_LinearDepthTexture;

layout(set = 0, binding = 8) uniform texture2D g_PostProcessedTexture;
layout(set = 0, binding = 9) uniform texture2D g_PrevVolumetricCloudTexture;
layout(set = 0, binding = 10) uniform texture2D g_GodrayTexture;

layout(set = 0, binding = 11) uniform texture2D g_SrcTexture2D;
layout(set = 0, binding = 12) uniform texture2D g_SkyBackgroudTexture;
layout(set = 0, binding = 13) uniform texture2D g_BlurTexture;
layout(set = 0, binding = 14) buffer TransmittanceColor
{
	vec4 TransmittanceColor_Data[];
};

layout(set = 0, binding = 15) uniform texture2D InputTex;

layout(set = 0, binding = 30, rgba32f) uniform image2D OutputTex;
layout(set = 0, binding = 31, rgba32f) uniform image2D volumetricCloudsDstTexture;
layout(set = 0, binding = 32, rgba32f) uniform image2D SavePrevTexture;

layout(set = 0, binding = 100) uniform sampler g_LinearClampSampler;
layout(set = 0, binding = 101) uniform sampler g_LinearWrapSampler;
layout(set = 0, binding = 102) uniform sampler g_PointClampSampler;
layout(set = 0, binding = 103) uniform sampler g_LinearBorderSampler;

layout(set = 1, binding = 110) uniform VolumetricCloudsCBuffer_Block
{
    VolumetricCloudsCB g_VolumetricClouds;
}VolumetricCloudsCBuffer;

float randomFromScreenUV(vec2 uv)
{
    return fract((sin(dot((uv).xy, vec2(12.9898005, 78.23300170))) * float (43758.546875)));
}
const vec3 rand[(TRANSMITTANCE_SAMPLE_STEP_COUNT + 1)] = {{0.0, 0.0, 0.0}, {0.612305, (-0.1875), 0.28125}, {0.648437, 0.026367000, (-0.792969)}, {(-0.636719), 0.449219, (-0.539062)}, {(-0.8085940), 0.74804696, 0.456055}, {0.542969, 0.35156300, 0.048462}};
bool ray_trace_sphere(vec3 center, vec3 rd, vec3 offset, float radius2, out float t1, out float t2)
{
    vec3 p = (center - offset);
    float b = dot(p, rd);
    float c = (dot(p, p) - radius2);
    float f = ((b * b) - c);
    if((f >= float (0.0)))
    {
        float sqrtF = sqrt(f);
        (t1 = ((-b) - sqrtF));
        (t2 = ((-b) + sqrtF));
        if(((t2 <= float (0.0)) && (t1 <= float (0.0))))
        {
            return false;
        }
        return true;
    }
    else
    {
        (t1 = float (0.0));
        (t2 = float (0.0));
        return false;
    }
}

bool GetStartEndPointForRayMarching(vec3 ws_origin, vec3 ws_ray, vec3 earthCenter,
 float layerStart2, float LayerEnd2, out vec3 start, out vec3 end)
 {

	float ot1, ot2, it1, it2;
	start = end = vec3(0.0, 0.0, 0.0);

	if (!ray_trace_sphere(ws_origin, ws_ray, earthCenter, LayerEnd2, ot1, ot2))
		return false;

	bool inIntersected = ray_trace_sphere(ws_origin, ws_ray, earthCenter, layerStart2, it1, it2);

	if (inIntersected)
	{
		float branchFactor = clamp(floor(it1 + 1.0), 0.0, 1.0);

		start = ws_origin + mix(max(it2, 0), max(ot1, 0), branchFactor) * ws_ray;
		end = ws_origin + mix(ot2, it1, branchFactor) * ws_ray;
	}
	else
	{
		end = ws_origin + ot2 * ws_ray;
		start = ws_origin + max(ot1, 0) * ws_ray;
	}

	return true;
}

float Remap(float original_value, float original_min, float original_max, float new_min, float new_max)
{
    return (new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min)));
}

float RemapClamped(float original_value, float original_min, float original_max, float new_min, float new_max)
{
    return (new_min + (clamp(((original_value - original_min) / (original_max - original_min)), 0.0, 1.0) * (new_max - new_min)));
}

float HenryGreenstein(float g, float cosTheta)
{
    float g2 = (g * g);
    float numerator = (float (1.0) - g2);
    float denominator = pow(abs(((float (1.0) + g2) - ((float (2.0) * g) * cosTheta))),float(1.5));
    return ((float (ONE_OVER_FOURPI) * numerator) / denominator);
}

float Phase(float g, float g2, float cosTheta, float y)
{
    return mix(HenryGreenstein(g, cosTheta), HenryGreenstein(g2, cosTheta), y);
}

vec3 getProjectedShellPoint(float earthRadiusAddCloudLayerStart, in vec3 pt, in vec3 center)
{
    return ((vec3 (earthRadiusAddCloudLayerStart) * normalize((pt - center))) + center);
}

float getRelativeHeight(in vec3 pt, in vec3 projectedPt, float layer_thickness)
{
    return clamp((length((pt - projectedPt)) / float (layer_thickness)), 0.0, 1.0);
}

// Get the relative height accurately
float getRelativeHeightAccurate(vec3 earthCenter, float earthRadiusAddCloudLayerStart,
 vec3 pt, vec3 projectedPt, float layer_thickness)
{
	float t = distance(pt, earthCenter);
	t -= earthRadiusAddCloudLayerStart;
	return clamp(max(t, 0.0f) / layer_thickness, 0.0, 1.0);
}

float PackFloat16(float value)
{
    return (value * 0.001f);
}
float UnPackFloat16(float value)
{
    return (value * 1000.0f);
}

float getAtmosphereBlendForComposite(float distance)
{
	float rate = mix(0.75f, 0.4f, clamp(VolumetricCloudsCBuffer.g_VolumetricClouds.m_MaxSampleDistance / MAX_SAMPLE_STEP_DISTANCE, 0.0, 1.0));
	float Threshold = VolumetricCloudsCBuffer.g_VolumetricClouds.m_MaxSampleDistance * rate;
	float InvThreshold = VolumetricCloudsCBuffer.g_VolumetricClouds.m_MaxSampleDistance - Threshold;
	return clamp(max(distance * MAX_SAMPLE_STEP_DISTANCE - Threshold, 0.0f) / InvThreshold, 0.0, 1.0);
}

float GetLODBias(float distance)
{
    float factor = 50000.0;
    return clamp((max((distance - factor), 0.0) / (float (MAX_SAMPLE_STEP_DISTANCE) - factor)), 0.0, 1.0);
}

vec2 Rotation(vec2 pos, float theta)
{
	float cosTheta = cos(theta);
	float sinTheta = sin(theta);
	return vec2(pos.x * cosTheta - pos.y * sinTheta, pos.x * sinTheta + pos.y * cosTheta);
}

float GetDensityHeightGradientForPoint(in float relativeHeight, in float cloudType)
{
    float cumulus = max(float (0.0), (RemapClamped(relativeHeight, float (0.010000000), float (0.15), float (0.0), float (1.0)) * RemapClamped(relativeHeight, float (0.9), float (0.95), float (1.0), float (0.0))));
    float stratocumulus = max(float (0.0), (RemapClamped(relativeHeight, float (0.0), float (0.15), float (0.0), float (1.0)) * RemapClamped(relativeHeight, float (0.3), float (0.65), float (1.0), float (0.0))));
    float stratus = max(float (0.0), (RemapClamped(relativeHeight, float (0.0), float (0.1), float (0.0), float (1.0)) * RemapClamped(relativeHeight, float (0.2), float (0.3), float (1.0), float (0.0))));
    float cloudType2 = (cloudType * float (2.0));
    float a = mix(stratus, stratocumulus, clamp(cloudType2, 0.0, 1.0));
    float b = mix(stratocumulus, cumulus, clamp((cloudType2 - float (1.0)), 0.0, 1.0));
    return mix(a, b, round(cloudType));
}

// Get the density of clouds from current ray-marched position
float SampleDensity(vec3 worldPos, float lod, float height_fraction, vec3 currentProj, float LayerThickness,
	float CloudSize, float BaseShapeTiling, float CloudCoverage, float CloudType, float AnvilBias,
	float CurlStrenth, float CurlTextureTiling,
	float DetailStrenth,
	float RisingVaporUpDirection, float RisingVaporScale, float RisingVaporIntensity,
	vec3 cloudTopOffsetWithWindDir, vec4 windWithVelocity,
	vec3 biasedCloudPos, float DetailShapeTilingDivCloudSize,
	float WeatherTextureOffsetX, float WeatherTextureOffsetZ, float WeatherTextureSize,
	float RotationPivotOffsetX, float RotationPivotOffsetZ,
	float RotationAngle,
	bool cheap)
{
    float height_bias = VolumetricCloudsCBuffer.g_VolumetricClouds.Test03;
		float height_fraction_biased = clamp(height_fraction + height_bias, -1.0, 1.0);

    vec3 unwindWorldPos = worldPos;
    (worldPos += (vec3 (height_fraction) * cloudTopOffsetWithWindDir));
    (worldPos += biasedCloudPos);

		vec2 weatherMapUV = (unwindWorldPos.xz + windWithVelocity.xy + vec2(WeatherTextureOffsetX, WeatherTextureOffsetZ)) / (WeatherTextureSize);

		vec2 RotationOffset = vec2(RotationPivotOffsetX, RotationPivotOffsetZ);
		weatherMapUV -= RotationOffset;
		weatherMapUV = Rotation(weatherMapUV, RotationAngle);
		weatherMapUV += RotationOffset;

    vec4 weatherData = textureLod(sampler2D( weatherTexture, g_LinearWrapSampler), weatherMapUV, 0.0);

    vec3 worldPosDivCloudSize = (worldPos / vec3 (CloudSize));

    vec4 low_freq_noises = textureLod(sampler3D( lowFreqNoiseTexture, g_LinearWrapSampler), vec3((worldPosDivCloudSize * vec3 (BaseShapeTiling))), lod);
    
    float low_freq_fBm = ((((low_freq_noises).g * float (0.625)) + ((low_freq_noises).b * float (0.25))) + ((low_freq_noises).a * float (0.125)));
    float base_cloud = RemapClamped((low_freq_noises).r, (low_freq_fBm - float (1.0)), float (1.0), float (0.0), float (1.0));
    (base_cloud = clamp((base_cloud + CloudCoverage), 0.0, 1.0));

    float cloudType = clamp(((weatherData).g + CloudType), 0.0, 1.0);
    float density_height_gradient = GetDensityHeightGradientForPoint(height_fraction, cloudType);
    (base_cloud *= density_height_gradient);
    float cloud_coverage = clamp((weatherData).r, 0.0, 1.0);
    (cloud_coverage = pow(cloud_coverage,float(RemapClamped(height_fraction, float (0.2), float (0.8), float (1.0), mix(float (1.0), float (0.5), AnvilBias)))));
    
    float base_cloud_coverage = RemapClamped(base_cloud, cloud_coverage, float (1.0), float (0.0), float (1.0));
    (base_cloud_coverage *= cloud_coverage);
    
    float final_cloud = base_cloud_coverage;

    if((!cheap))
    {
        vec2 curl_noise = vec2 ((textureLod(sampler2D( curlNoiseTexture, g_LinearWrapSampler), vec2(vec2(((worldPosDivCloudSize).xz * vec2 (CurlTextureTiling)))), 1.0f)).rg);
        ((worldPos).xz += ((curl_noise * vec2 ((float (1.0) - height_fraction))) * vec2 (CurlStrenth)));
        worldPos.y -= ((VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.x * RisingVaporUpDirection) / (CloudSize * 0.0657 * RisingVaporScale));

        vec3 high_frequency_noises = vec3 ((textureLod(sampler3D( highFreqNoiseTexture, g_LinearWrapSampler), vec3(vec3(((worldPos + vec3(windWithVelocity.z, 0.0, windWithVelocity.w))   * vec3 (DetailShapeTilingDivCloudSize)))), HIGH_FREQ_LOD)).rgb);
        float high_freq_fBm = ((((high_frequency_noises).r * float (0.625)) + ((high_frequency_noises).g * float (0.25))) + ((high_frequency_noises).b * float (0.125)));
        
        float height_fraction_new = getRelativeHeight(worldPos, currentProj, LayerThickness);
        float height_freq_noise_modifier = mix(high_freq_fBm, (float (1.0) - high_freq_fBm), clamp((height_fraction_new * float (10.0)), 0.0, 1.0));
        
        (final_cloud = RemapClamped(base_cloud_coverage, (height_freq_noise_modifier * DetailStrenth), float (1.0), float (0.0), float (1.0)));
    }
    return final_cloud;
}
float GetLightEnergy(float height_fraction, float dl, float ds_loded, float phase_probability, float cos_angle, float step_size, float contrast)
{
    float primary_att = exp((-dl));
    float secondary_att = (exp(((-dl) * float (0.25))) * float (0.7));
    float attenuation_probability = max(primary_att, (secondary_att * float (0.25)));
    float depth_probability = mix((float (0.05) + pow(ds_loded,float(RemapClamped(height_fraction, float (0.3), float (0.85), float (0.5), float (2.0))))), float (1.0), clamp(dl, 0.0, 1.0));
    float vertical_probability = pow(RemapClamped(height_fraction, float (0.07), float (0.14), float (0.1), float (1.0)),float(0.8));
    float in_scatter_probability = (vertical_probability * depth_probability);
    float light_energy = (attenuation_probability + (in_scatter_probability * phase_probability));
    (light_energy = pow(abs(light_energy),float(contrast)));
    return light_energy;
}

float SampleEnergy(vec3 rayPos, vec3 magLightDirection, float height_fraction, vec3 currentProj,
	vec3 earthCenter,
	float earthRadiusAddCloudLayerStart,
	float layerThickness,
	float CloudSize, float BaseShapeTiling, float CloudCoverage, float CloudType, float AnvilBias,
	float CurlStrenth, float CurlTextureTiling,
	float DetailStrenth,
	float RisingVaporUpDirection, float RisingVaporScale, float RisingVaporIntensity,
	vec3 cloudTopOffsetWithWindDir, vec4 windWithVelocity,
	vec3 biasedCloudPos, float DetailShapeTilingDivCloudSize,
	float WeatherTextureOffsetX, float WeatherTextureOffsetZ, float WeatherTextureSize,
	float RotationPivotOffsetX, float RotationPivotOffsetZ,
	float RotationAngle,
	float ds_loded, float stepSize, float cosTheta, float mipBias)
{
    float totalSample = float (0);
    float mipmapOffset = mipBias;
    float step = float (0.5);

    for (int i = 0; (i < TRANSMITTANCE_SAMPLE_STEP_COUNT); (i++))
    {
        vec3 rand3 = rand[i];
        vec3 direction = (magLightDirection + normalize(rand3));
        (direction = normalize(direction));
        vec3 samplePoint = (rayPos + (vec3 ((step * stepSize)) * direction));

        vec3 currentProj_new = getProjectedShellPoint(earthRadiusAddCloudLayerStart, samplePoint, earthCenter);

        float height_fraction_new = getRelativeHeight(samplePoint, currentProj_new, layerThickness);
        totalSample += SampleDensity(samplePoint, mipmapOffset, height_fraction_new, currentProj_new, layerThickness,
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
        (mipmapOffset += float (0.5));
        (step += 1.0);
    }
    float hg = max(HenryGreenstein((VolumetricCloudsCBuffer.g_VolumetricClouds).Eccentricity, cosTheta), (clamp(HenryGreenstein((0.99 - (VolumetricCloudsCBuffer.g_VolumetricClouds).SilverliningSpread), cosTheta), 0.0, 1.0))) * (VolumetricCloudsCBuffer.g_VolumetricClouds).SilverliningIntensity;
	
	float dl = totalSample * VolumetricCloudsCBuffer.g_VolumetricClouds.Precipitation;
	hg = hg / max(dl, 0.05);	
	
	float lodded_density = clamp(SampleDensity(rayPos, mipBias, height_fraction, currentProj, layerThickness,
		CloudSize, BaseShapeTiling, CloudCoverage, CloudType, AnvilBias,
		CurlStrenth, CurlTextureTiling,
		DetailStrenth,
		RisingVaporUpDirection, RisingVaporScale, RisingVaporIntensity,
		cloudTopOffsetWithWindDir, windWithVelocity,
		biasedCloudPos, DetailShapeTilingDivCloudSize,
		WeatherTextureOffsetX, WeatherTextureOffsetZ, WeatherTextureSize,
		RotationPivotOffsetX, RotationPivotOffsetZ,
		RotationAngle,
		true), 0.0, 1.0);
    float energy = GetLightEnergy(height_fraction, dl, ds_loded, hg, cosTheta, stepSize, (VolumetricCloudsCBuffer.g_VolumetricClouds).Contrast);
    return energy;
}

// Get the final density, light intensirt, atmophereBlendFactor, and distance between hit points against clouds and view position
float GetDensity(vec3 startPos, vec3 worldPos, vec3 dir, float raymarchOffset,
	out float intensity, out float atmosphericBlendFactor, out float depth, vec2 uv)
{
	vec3 sampleStart, sampleEnd;

	depth = 0.0;
	intensity = 0.0;
	atmosphericBlendFactor = 0.0f;

	// If the current view direction is not intersected with cloud's layers
	if (!GetStartEndPointForRayMarching(startPos, dir, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz,
		VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart2,
		VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd2, sampleStart, sampleEnd))
		return 0.0;

	// Determine the sample count and its step size along the cosTheta of view direction with Up vector 
	float horizon = abs(dir.y);

	uint sample_count = uint(mix(float(VolumetricCloudsCBuffer.g_VolumetricClouds.MAX_ITERATION_COUNT), float(VolumetricCloudsCBuffer.g_VolumetricClouds.MIN_ITERATION_COUNT), horizon));
	float sample_step = mix(float(VolumetricCloudsCBuffer.g_VolumetricClouds.m_StepSize.y), float(VolumetricCloudsCBuffer.g_VolumetricClouds.m_StepSize.x), horizon);

	// Update the distance between hit points against clouds and view position
	depth = distance(startPos, sampleEnd);

	float distCameraToStart = distance(sampleStart, startPos);
	atmosphericBlendFactor = distCameraToStart / MAX_SAMPLE_STEP_DISTANCE;

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

#if STEREO_INSTANCED
	vec2 textureSize = VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.zw * vec2(0.5, 0.25);
#else	
	vec2 textureSize = VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.zw * 0.25;
#endif

	// Depth Culling
	// Get texel coordinates for Depth culling
	uvec2 texels = uvec2((vec2(VolumetricCloudsCBuffer.g_VolumetricClouds.DepthMapWidth, VolumetricCloudsCBuffer.g_VolumetricClouds.DepthMapHeight) * uv));

	//Get the lodded depth, if it is not using, use far distance instead to pass the culling
	float sceneDepth;

	if (float(VolumetricCloudsCBuffer.g_VolumetricClouds.EnabledLodDepthCulling) > 0.5f)
	{
		sceneDepth = texelFetch(depthTexture, ivec2(ivec2(texels)).xy + ivec2(0, 0), 0).r;

		if (sceneDepth < 1.0f)
		{
			return 1.0;
		}
	}

	// Prepare to do raymarching
	float alpha = 0.0f;
	intensity = 0.0f;
	bool detailedSample = false; // start with cheap raymarching
	int missedStepCount = 0;
	float ds = 0.0f;	
	float bigStep = sample_step * 2.0f;
	vec3 smallStepMarching = sample_step * dir;
	vec3 bigStepMarching = bigStep * dir;

	// To prevent raymarching artifact, use a random value
	vec3 raymarchingDistance = smallStepMarching * raymarchOffset;

	vec3 magLightDirection = 2.0 * VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.xyz;

	bool pickedFirstHit = false;

	float cosTheta = dot(dir, VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.xyz);
	vec3 rayPos = sampleStart;

	vec4 windWithVelocity = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].StandardPosition;

	vec3 biasedCloudPos = 4.5f * (VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WindDirection.xyz + vec3(0.0, 0.1, 0.0));

	vec3 cloudTopOffsetWithWindDir = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudTopOffset * VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WindDirection.xyz;

	float DetailShapeTilingDivCloudSize = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].DetailShapeTiling / VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudSize;

	for (uint j = 0; j < sample_count; j++)
	{
		rayPos += raymarchingDistance;

		vec3 currentProj = getProjectedShellPoint(VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart, rayPos, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz);
		float height_fraction = getRelativeHeightAccurate(VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart, rayPos, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness);

		if (!detailedSample)
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness,
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

			if ( sampleResult > 0.0f )
			{
				// If it hit the clouds, change to the expensive raymarching
				detailedSample = true;

				raymarchingDistance = -bigStepMarching;
				missedStepCount = 0;
				continue;
			}
			else
				raymarchingDistance = bigStepMarching;
		}
		else
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness,
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

			if (sampleResult == 0.0 )
			{
				missedStepCount++;

				// If expensive raymarching failed more 10 times, go back to cheap raymarching
				if (missedStepCount > 10)
					detailedSample = false;
			}
			else
			{
				// Get the first hit position against clouds to use it for reprojection
				if (!pickedFirstHit)
				{
					depth = distance(rayPos, startPos);
					pickedFirstHit = true;
				}

				if (sampleResult > 0.0)
				{
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

					float oneMinusAlpha = 1.0 - alpha;
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

float GetDensityWithComparingDepth(vec3 startPos, vec3 worldPos, vec3 dir, float raymarchOffset,
  out float intensity, out float atmosphericBlendFactor, out float depth, vec2 uv)
{
  vec3 sampleStart, sampleEnd;

  (depth = float(0.0));
  (intensity = float(0.0));
  (atmosphericBlendFactor = 0.0);

  if (!GetStartEndPointForRayMarching(startPos, dir, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz,
  VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart2, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd2,
  sampleStart, sampleEnd))
  {
    return float(0.0);
  }

  float horizon = abs((dir).y);
  
  uint sample_count = uint(mix(float(VolumetricCloudsCBuffer.g_VolumetricClouds.MAX_ITERATION_COUNT), float(VolumetricCloudsCBuffer.g_VolumetricClouds.MIN_ITERATION_COUNT), horizon));
  float sample_step = mix(float(VolumetricCloudsCBuffer.g_VolumetricClouds.m_StepSize.y), float(VolumetricCloudsCBuffer.g_VolumetricClouds.m_StepSize.x), horizon);

  (depth = distance(sampleEnd, startPos));

  float distCameraToStart = distance(sampleStart, startPos);
  atmosphericBlendFactor = distCameraToStart / MAX_SAMPLE_STEP_DISTANCE;

  float sceneDepth = textureLod(sampler2D(depthTexture, g_LinearClampSampler), vec2(uv), float(0)).r;
  if (sceneDepth < 1.0f)
  {
    atmosphericBlendFactor = 1.0f;
  }

  float linearDepth = mix(50.0f, 100000000.0f, sceneDepth);

  if (((sampleStart).y < 0.0))
  {
    intensity = 1.0f;
    return 1.0f;
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
		intensity = 0.0;
		return 1.0;
}

	float transStepSize = VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.a;

#if STEREO_INSTANCED
  vec2 textureSize = (((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw * vec2(0.5, 0.25));
#else
  vec2 textureSize = (((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw * vec2(0.25));
#endif

  vec2 Corrected_UV = (uv - (vec2(1.0, 1.0) / textureSize));
  uvec2 texels = uvec2((vec2((VolumetricCloudsCBuffer.g_VolumetricClouds).DepthMapWidth, (VolumetricCloudsCBuffer.g_VolumetricClouds).DepthMapHeight) * uv));
  //float sceneDepth = texelFetch(depthTexture, ivec2(ivec2(texels)).xy + ivec2(0, 0), 0).r;


  float alpha = 0.0;
  (intensity = 0.0);
  bool detailedSample = false;
  int missedStepCount = 0;
  float ds = 0.0;

	float bigStep = sample_step * 2.0f;

  vec3 smallStepMarching = (vec3(sample_step) * dir);
  vec3 bigStepMarching = (vec3(bigStep) * dir);

  vec3 raymarchingDistance = (smallStepMarching * vec3(raymarchOffset));
  vec3 magLightDirection = (vec3(2.0) * ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);

  bool pickedFirstHit = false;
  
  float cosTheta = dot(dir, ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);
  vec3 rayPos = sampleStart;
  
  vec4 windWithVelocity = ((VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0]).StandardPosition);
  vec3 biasedCloudPos = (vec3(4.5) * (((VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0]).WindDirection).xyz + vec3(0.0, 0.1, 0.0)));
  vec3 cloudTopOffsetWithWindDir = (vec3((VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0]).CloudTopOffset) * ((VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0]).WindDirection).xyz);
  
  float DetailShapeTilingDivCloudSize = ((VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0]).DetailShapeTiling / (VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0]).CloudSize);
 
  for (uint j = uint(0); (j < sample_count); (j++))
  {
    (rayPos += raymarchingDistance);

    if (linearDepth < distance(startPos, rayPos))
    {
      break;
    }

    vec3 currentProj = getProjectedShellPoint(VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart, rayPos, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz);
    float height_fraction = getRelativeHeightAccurate(VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz,
    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart, rayPos, currentProj,
    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness);

    if ((!detailedSample))
    {
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
      float sampleResult = SampleDensity(rayPos, LOW_FREQ_LOD, height_fraction, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudSize, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].BaseShapeTiling,
                VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudCoverage, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudType, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].AnvilBias,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CurlStrenth, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CurlTextureTiling,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].DetailStrenth,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporUpDirection, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporScale, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporIntensity,
				cloudTopOffsetWithWindDir, windWithVelocity,
				biasedCloudPos, DetailShapeTilingDivCloudSize,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureOffsetZ,
                VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureSize,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationPivotOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationPivotOffsetZ,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationAngle,
				false);

      if ((sampleResult == float(0.0)))
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
        float sampledAlpha = (sampleResult * (VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerLayer[0].CloudDensity);
        float sampledEnergy = SampleEnergy(
					rayPos, magLightDirection, height_fraction, currentProj,
					VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz,
					VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart,
					VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness,
					VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudSize, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].BaseShapeTiling,
                    VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudCoverage, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudType, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].AnvilBias,
					VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CurlStrenth, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CurlTextureTiling,
					VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].DetailStrenth,
					VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporUpDirection, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporScale, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RisingVaporIntensity,
					cloudTopOffsetWithWindDir, windWithVelocity,
					biasedCloudPos, DetailShapeTilingDivCloudSize,
					VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureOffsetZ, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WeatherTextureSize,
					VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationPivotOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationPivotOffsetZ,
					VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].RotationAngle,
					sampleResult, transStepSize, cosTheta, LOW_FREQ_LOD);

        float oneMinusAlpha = (float(1.0) - alpha);
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
}

// Get the final density, light intensirt, atmophereBlendFactor, and distance between hit points against clouds and view position
float GetDensity_Double_Layers(vec3 startPos, vec3 worldPos, vec3 dir, float raymarchOffset,
	out float intensity, out float atmosphericBlendFactor, out float depth, vec2 uv)
{
	vec3 sampleStart, sampleEnd;

	depth = 0.0;
	intensity = 0.0;
	atmosphericBlendFactor = 0.0f;

	// If the current view direction is not intersected with cloud's layers
	if (!GetStartEndPointForRayMarching(startPos, dir, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz,
		min(VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart2, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart2),
		max(VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd2, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].EarthRadiusAddCloudsLayerEnd2), sampleStart, sampleEnd))
		return 0.0;

	// Determine the sample count and its step size along the cosTheta of view direction with Up vector 
	float horizon = abs(dir.y);

	uint sample_count = uint(mix(float(VolumetricCloudsCBuffer.g_VolumetricClouds.MAX_ITERATION_COUNT), float(VolumetricCloudsCBuffer.g_VolumetricClouds.MIN_ITERATION_COUNT), horizon));
	float sample_step = mix(float(VolumetricCloudsCBuffer.g_VolumetricClouds.m_StepSize.y), float(VolumetricCloudsCBuffer.g_VolumetricClouds.m_StepSize.x), horizon);

	// Update the distance between hit points against clouds and view position
	depth = distance(startPos, sampleEnd);

	float distCameraToStart = distance(sampleStart, startPos);
	atmosphericBlendFactor = distCameraToStart / MAX_SAMPLE_STEP_DISTANCE;

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
		intensity = 0.0;
		return 1.0;
}

	float transStepSize = VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.a;

#if STEREO_INSTANCED
	vec2 textureSize = VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.zw * vec2(0.5, 0.25);
#else	
	vec2 textureSize = VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.zw * 0.25;
#endif

	// Depth Culling
	// Get texel coordinates for Depth culling
	uvec2 texels = uvec2((vec2(VolumetricCloudsCBuffer.g_VolumetricClouds.DepthMapWidth, VolumetricCloudsCBuffer.g_VolumetricClouds.DepthMapHeight) * uv));

	//Get the lodded depth, if it is not using, use far distance instead to pass the culling
	float sceneDepth;

	if (float(VolumetricCloudsCBuffer.g_VolumetricClouds.EnabledLodDepthCulling) > 0.5f)
	{
		sceneDepth = texelFetch(depthTexture, ivec2(ivec2(texels)).xy + ivec2(0, 0), 0).r;

		if (sceneDepth < 1.0f)
		{
			return 1.0;
		}
	}

	// Prepare to do raymarching
	float alpha = 0.0f;
	intensity = 0.0f;
	bool detailedSample = false; // start with cheap raymarching
	int missedStepCount = 0;
	float ds = 0.0f;

	float bigStep = sample_step * 2.0f;

	vec3 smallStepMarching = sample_step * dir;
	vec3 bigStepMarching = bigStep * dir;

	// To prevent raymarching artifact, use a random value
	vec3 raymarchingDistance = smallStepMarching * raymarchOffset;

	vec3 magLightDirection = 2.0 * VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.xyz;

	bool pickedFirstHit = false;

	float cosTheta = dot(dir, VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.xyz);
	vec3 rayPos = sampleStart;

	vec4 windWithVelocity = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].StandardPosition;
	vec4 windWithVelocity_2nd = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].StandardPosition;

	vec3 biasedCloudPos = 4.5f * (VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WindDirection.xyz + vec3(0.0, 0.1, 0.0));
	vec3 biasedCloudPos_2nd = 4.5f * (VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WindDirection.xyz + vec3(0.0, 0.1, 0.0));

	vec3 cloudTopOffsetWithWindDir = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudTopOffset * VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WindDirection.xyz;
	vec3 cloudTopOffsetWithWindDir_2nd = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudTopOffset * VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WindDirection.xyz;

	float DetailShapeTilingDivCloudSize = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].DetailShapeTiling / VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudSize;
	float DetailShapeTilingDivCloudSize_2nd = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].DetailShapeTiling / VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudSize;

	for (uint j = 0; j < sample_count; j++)
	{
		rayPos += raymarchingDistance;

		vec3 currentProj = getProjectedShellPoint(VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart, rayPos, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz);
		float height_fraction = getRelativeHeightAccurate(VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart, rayPos, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness);

		vec3 currentProj_2nd = getProjectedShellPoint(VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart, rayPos, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz);
		float height_fraction_2nd = getRelativeHeightAccurate(VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart, rayPos, currentProj_2nd, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].LayerThickness);

		if (!detailedSample)
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness,
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

			float sampleResult_2nd = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction_2nd, currentProj_2nd, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].LayerThickness,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudSize, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].BaseShapeTiling, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudCoverage, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudType, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].AnvilBias,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlStrenth, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlTextureTiling,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].DetailStrenth,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporUpDirection, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporScale, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporIntensity,
				cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
				biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetZ, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureSize,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetZ,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationAngle,
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
				raymarchingDistance = bigStepMarching;
		}
		else
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness,
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

			float sampleResult_2nd = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction_2nd, currentProj_2nd, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].LayerThickness,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudSize, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].BaseShapeTiling, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudCoverage, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudType, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].AnvilBias,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlStrenth, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlTextureTiling,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].DetailStrenth,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporUpDirection, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporScale, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporIntensity,
				cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
				biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetZ, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureSize,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetZ,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationAngle,
				false);

			if (sampleResult == 0.0 && sampleResult_2nd == 0.0)
			{
				missedStepCount++;

				// If expensive raymarching failed more 10 times, go back to cheap raymarching
				if (missedStepCount > 10)
					detailedSample = false;
			}
			else
			{
				// Get the first hit position against clouds to use it for reprojection
				if (!pickedFirstHit)
				{
					depth = distance(rayPos, startPos);
					pickedFirstHit = true;
				}

				if (sampleResult > 0.0)
				{
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

					float oneMinusAlpha = 1.0 - alpha;
					sampledAlpha *= oneMinusAlpha;

					intensity += sampledAlpha * sampledEnergy;
					alpha += sampledAlpha;
				}

				if (sampleResult_2nd > 0.0)
				{
					// If it hit the clouds, get the light enery from current rayPos and accumulate it
					float sampledAlpha = sampleResult_2nd * VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudDensity;
					float sampledEnergy = SampleEnergy(
						rayPos, magLightDirection, height_fraction_2nd, currentProj_2nd,
						VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].LayerThickness,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudSize, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].BaseShapeTiling, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudCoverage, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudType, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].AnvilBias,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlStrenth, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlTextureTiling,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].DetailStrenth,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporUpDirection, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporScale, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporIntensity,
						cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
						biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetZ, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureSize,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetZ,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationAngle,
						sampleResult_2nd, transStepSize, cosTheta, LOW_FREQ_LOD);

					float oneMinusAlpha = 1.0 - alpha;
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
float GetDensity_Double_Layers_WithComparingDepth(vec3 startPos, vec3 worldPos, vec3 dir, float raymarchOffset,
	out float intensity, out float atmosphericBlendFactor, out float depth, vec2 uv)
{
	vec3 sampleStart, sampleEnd;

	depth = 0.0;
	intensity = 0.0;
	atmosphericBlendFactor = 0.0f;

	// If the current view direction is not intersected with cloud's layers
	if (!GetStartEndPointForRayMarching(startPos, dir, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz,
		min(VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart2, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart2),
		max(VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerEnd2, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].EarthRadiusAddCloudsLayerEnd2), sampleStart, sampleEnd))
		return 0.0;

	// Determine the sample count and its step size along the cosTheta of view direction with Up vector 
	float horizon = abs(dir.y);

	uint sample_count = uint(mix(float(VolumetricCloudsCBuffer.g_VolumetricClouds.MAX_ITERATION_COUNT), float(VolumetricCloudsCBuffer.g_VolumetricClouds.MIN_ITERATION_COUNT), horizon));
	float sample_step = mix(float(VolumetricCloudsCBuffer.g_VolumetricClouds.m_StepSize.y), float(VolumetricCloudsCBuffer.g_VolumetricClouds.m_StepSize.x), horizon);

	// Update the distance between hit points against clouds and view position
	depth = distance(startPos, sampleEnd);

	float distCameraToStart = distance(sampleStart, startPos);
	atmosphericBlendFactor = distCameraToStart / MAX_SAMPLE_STEP_DISTANCE;

	float sceneDepth = textureLod(sampler2D(depthTexture, g_LinearClampSampler), vec2(uv), float(0)).r;
	if (sceneDepth < 1.0f)
	{
		atmosphericBlendFactor = 1.0f;
	}

	float linearDepth = mix(50.0f, 100000000.0f, sceneDepth);


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

#if STEREO_INSTANCED
	vec2 textureSize = VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.zw * vec2(0.5, 0.25);
#else	
	vec2 textureSize = VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.zw * 0.25;
#endif

	// Depth Culling
	// Get texel coordinates for Depth culling
	uvec2 texels = uvec2(vec2(VolumetricCloudsCBuffer.g_VolumetricClouds.DepthMapWidth, VolumetricCloudsCBuffer.g_VolumetricClouds.DepthMapHeight) * uv);

	// Prepare to do raymarching
	float alpha = 0.0f;
	intensity = 0.0f;
	bool detailedSample = false; // start with cheap raymarching
	int missedStepCount = 0;
	float ds = 0.0f;

	float bigStep = sample_step * 2.0f;

	vec3 smallStepMarching = sample_step * dir;
	vec3 bigStepMarching = bigStep * dir;

	// To prevent raymarching artifact, use a random value
	vec3 raymarchingDistance = smallStepMarching * raymarchOffset;

	vec3 magLightDirection = 2.0 * VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.xyz;

	bool pickedFirstHit = false;

	float cosTheta = dot(dir, VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.xyz);
	vec3 rayPos = sampleStart;

	vec4 windWithVelocity = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].StandardPosition;
	vec4 windWithVelocity_2nd = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].StandardPosition;

	vec3 biasedCloudPos = 4.5f * (VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WindDirection.xyz + vec3(0.0, 0.1, 0.0));
	vec3 biasedCloudPos_2nd = 4.5f * (VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WindDirection.xyz + vec3(0.0, 0.1, 0.0));

	vec3 cloudTopOffsetWithWindDir = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudTopOffset * VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].WindDirection.xyz;
	vec3 cloudTopOffsetWithWindDir_2nd = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudTopOffset * VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WindDirection.xyz;

	float DetailShapeTilingDivCloudSize = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].DetailShapeTiling / VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].CloudSize;
	float DetailShapeTilingDivCloudSize_2nd = VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].DetailShapeTiling / VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudSize;

	for (uint j = 0; j < sample_count; j++)
	{
		rayPos += raymarchingDistance;

		if (linearDepth < distance(startPos, rayPos))
		{
			break;
		}

		vec3 currentProj = getProjectedShellPoint(VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart, rayPos, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz);
		float height_fraction = getRelativeHeightAccurate(VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].EarthRadiusAddCloudsLayerStart, rayPos, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness);

		vec3 currentProj_2nd = getProjectedShellPoint(VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart, rayPos, VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz);
		float height_fraction_2nd = getRelativeHeightAccurate(VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart, rayPos, currentProj_2nd, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].LayerThickness);

		if (!detailedSample)
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness,
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

			float sampleResult_2nd = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction_2nd, currentProj_2nd, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].LayerThickness,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudSize, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].BaseShapeTiling, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudCoverage, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudType, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].AnvilBias,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlStrenth, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlTextureTiling,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].DetailStrenth,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporUpDirection, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporScale, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporIntensity,
				cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
				biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetZ, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureSize,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetZ,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationAngle,
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
				raymarchingDistance = bigStepMarching;
		}
		else
		{
			// Get the density from current rayPos
			float sampleResult = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[0].LayerThickness,
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

			float sampleResult_2nd = SampleDensity(
				rayPos, LOW_FREQ_LOD, height_fraction_2nd, currentProj_2nd, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].LayerThickness,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudSize, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].BaseShapeTiling, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudCoverage, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudType, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].AnvilBias,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlStrenth, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlTextureTiling,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].DetailStrenth,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporUpDirection, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporScale, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporIntensity,
				cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
				biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetZ, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureSize,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetZ,
				VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationAngle,
				false);

			if (sampleResult == 0.0 && sampleResult_2nd == 0.0)
			{
				missedStepCount++;

				// If expensive raymarching failed more 10 times, go back to cheap raymarching
				if (missedStepCount > 10)
					detailedSample = false;
			}
			else
			{
				// Get the first hit position against clouds to use it for reprojection
				if (!pickedFirstHit)
				{
					depth = distance(rayPos, startPos);
					pickedFirstHit = true;
				}

				if (sampleResult > 0.0)
				{
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

					float oneMinusAlpha = 1.0 - alpha;
					sampledAlpha *= oneMinusAlpha;

					intensity += sampledAlpha * sampledEnergy;
					alpha += sampledAlpha;
				}

				if (sampleResult_2nd > 0.0)
				{
					// If it hit the clouds, get the light enery from current rayPos and accumulate it
					float sampledAlpha = sampleResult_2nd * VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudDensity;
					float sampledEnergy = SampleEnergy(
						rayPos, magLightDirection, height_fraction_2nd, currentProj_2nd,
						VolumetricCloudsCBuffer.g_VolumetricClouds.EarthCenter.xyz,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].EarthRadiusAddCloudsLayerStart,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].LayerThickness,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudSize, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].BaseShapeTiling, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudCoverage, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CloudType, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].AnvilBias,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlStrenth, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].CurlTextureTiling,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].DetailStrenth,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporUpDirection, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporScale, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RisingVaporIntensity,
						cloudTopOffsetWithWindDir_2nd, windWithVelocity_2nd,
						biasedCloudPos_2nd, DetailShapeTilingDivCloudSize_2nd,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureOffsetZ, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].WeatherTextureSize,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationPivotOffsetZ,
						VolumetricCloudsCBuffer.g_VolumetricClouds.m_DataPerLayer[1].RotationAngle,
						sampleResult_2nd, transStepSize, cosTheta, LOW_FREQ_LOD);

					float oneMinusAlpha = 1.0 - alpha;
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

vec4 SamplePrev(vec2 prevUV, out float outOfBound)
{
    ((prevUV).xy = vec2((((prevUV).x + float (1.0)) * float (0.5)), ((float (1.0) - (prevUV).y) * float (0.5))));
    (outOfBound = step(float (0.0), max(max((-(prevUV).x), (-(prevUV).y)), (max((prevUV).x, (prevUV).y) - float (1.0)))));
    return texture(sampler2D( g_PrevFrameTexture, g_LinearClampSampler), vec2((prevUV).xy));
}
vec4 SampleCurrent(vec2 uv, vec2 _Jitter)
{
    (uv = (uv - ((_Jitter - vec2 (1.5)) * (vec2 (1.0) / ((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw))));
    return textureLod(sampler2D( LowResCloudTexture, g_PointClampSampler), vec2(uv), float (0));
}
float ShouldbeUpdated(vec2 uv, vec2 jitter)
{
    vec2 texelRelativePos = mod((uv * ((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw), vec2 (4.0));
    (texelRelativePos -= jitter);
    vec2 valid = clamp((vec2 (2.0) * (vec2(0.5, 0.5) - abs((texelRelativePos - vec2(0.5, 0.5))))), 0.0, 1.0);
    return ((valid).x * (valid).y);
}