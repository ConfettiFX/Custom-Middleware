/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#define _EARTH_RADIUS 5.000000e+06
#define _EARTH_CENTER vec3(0, (-_EARTH_RADIUS), 0)
#define _CLOUDS_LAYER_START 15000.0
//#define _CLOUDS_LAYER_THICKNESS 20000.0
//#define _CLOUDS_LAYER_END 35000.0
#define _EARTH_RADIUS_ADD_CLOUDS_LAYER_START 5.015000e+06
#define _EARTH_RADIUS_ADD_CLOUDS_LAYER_START2 2.515023e+13
//#define _EARTH_RADIUS_ADD_CLOUDS_LAYER_END 5.035000e+06
//#define _EARTH_RADIUS_ADD_CLOUDS_LAYER_END2 2.535122e+13
#define TRANSMITTANCE_SAMPLE_STEP_COUNT 5
#define MAX_SAMPLE_DISTANCE 200000.0
#define PI 3.14159274
#define PI2 6.2831854
#define ONE_OVER_FOURPI 0.07957747
#define THREE_OVER_16PI 0.059683104
#define USE_Ray_Distance 1
#define ALLOW_CONTROL_CLOUD 1
#define STEREO_INSTANCED 0

#define FLOAT16_MAX 65500.0f 

struct VolumetricCloudsCB
{
    mat4 m_WorldToProjMat_1st;
    mat4 m_ProjToWorldMat_1st;
    mat4 m_ViewToWorldMat_1st;
    mat4 m_PrevWorldToProjMat_1st;
    mat4 m_WorldToProjMat_2nd;
    mat4 m_ProjToWorldMat_2nd;
    mat4 m_ViewToWorldMat_2nd;
    mat4 m_PrevWorldToProjMat_2nd;
    mat4 m_LightToProjMat_1st;
    mat4 m_LightToProjMat_2nd;
    uint m_JitterX;
    uint m_JitterY;
    uint MIN_ITERATION_COUNT;
    uint MAX_ITERATION_COUNT;
    vec4 m_StepSize;
    vec4 TimeAndScreenSize;
    vec4 lightDirection;
    vec4 lightColorAndIntensity;
    vec4 cameraPosition_1st;
    vec4 cameraPosition_2nd;
    vec4 WindDirection;
    vec4 StandardPosition;

    float m_CorrectU;
    float m_CorrectV;
    float LayerThickness;

    float CloudDensity;
    float CloudCoverage;
    float CloudType;
    float CloudTopOffset;


    //Modeling
    float	CloudSize;						// Overall size of the clouds. Using bigger value generates larger chunks of clouds.
    float	BaseShapeTiling;				// Control the base shape of the clouds. Using bigger value makes smaller chunks of base clouds.
    float	DetailShapeTiling;				// Control the detail shape of the clouds. Using bigger value makes smaller chunks of detail clouds.
    float	DetailStrenth;					// Intensify the detail of the clouds. It is possible to lose whole shape of the clouds if the user uses too high value of it.

    float	CurlTextureTiling;				// Control the curl size of the clouds. Using bigger value makes smaller curl shapes.
    float	CurlStrenth;					// Intensify the curl effect.

    float UpstreamScale;
    float UpstreamSpeed;

    float	AnvilBias;						// Using lower value makes anvil shape.
    float	WeatherTextureSize;				// Control the size of Weather map, bigger value makes the world to be covered by larger clouds pattern.
    float	WeatherTextureOffsetX;
    float	WeatherTextureOffsetZ;


    //Lighting
    float	BackgroundBlendFactor;			// Blend clouds with the background, more background will be shown if this value is close to 0.0
    float	Contrast;						// Contrast of the clouds' color 
    float	Eccentricity;					// The bright highlights around the sun that the user needs at sunset
    float	CloudBrightness;				// The brightness for clouds

    float	Precipitation;
    float	SilverliningIntensity;			// Intensity of silver-lining
    float	SilverliningSpread;				// Using bigger value spreads more silver-lining, but the intesity of it
    float	Random00;						// Random seed for the first ray-marching offset

    float	CameraFarClip;
    float	Padding01;

    uint  EnabledDepthCulling;
    uint	EnabledLodDepthCulling;
    uint  DepthMapWidth;
    uint  DepthMapHeight;

    // VolumetricClouds' Light shaft
    uint	GodNumSamples;					// Number of godray samples

    float	GodrayMaxBrightness;
    float	GodrayExposure;					// Intensity of godray

    float	GodrayDecay;					// Using smaller value, the godray brightness applied to each iteration is reduced. The level of reduction is also reduced per iteration.
    float	GodrayDensity;					// The distance between each interation.
    float	GodrayWeight;					// Using smaller value, the godray brightness applied to each iteration is reduced. The level of reduction is not changed.
    float	m_UseRandomSeed;

    float	Test00;
    float	Test01;
    float	Test02;
    float	Test03;
};
layout(UPDATE_FREQ_NONE, binding = 0) uniform texture3D highFreqNoiseTexture;
layout(UPDATE_FREQ_NONE, binding = 1) uniform texture3D lowFreqNoiseTexture;
layout(UPDATE_FREQ_NONE, binding = 2) uniform texture2D curlNoiseTexture;
layout(UPDATE_FREQ_NONE, binding = 3) uniform texture2D weatherTexture;
layout(UPDATE_FREQ_NONE, binding = 4) uniform texture2D depthTexture;
layout(UPDATE_FREQ_NONE, binding = 5) uniform texture2D LowResCloudTexture;
layout(UPDATE_FREQ_NONE, binding = 6) uniform texture2D g_PrevFrameTexture;
layout(UPDATE_FREQ_NONE, binding = 7) uniform texture2D g_LinearDepthTexture;

layout(UPDATE_FREQ_NONE, binding = 8) uniform texture2D g_PostProcessedTexture;
layout(UPDATE_FREQ_NONE, binding = 9) uniform texture2D g_PrevVolumetricCloudTexture;
layout(UPDATE_FREQ_NONE, binding = 10) uniform texture2D g_GodrayTexture;

layout(UPDATE_FREQ_NONE, binding = 11) uniform texture2D g_SrcTexture2D;
layout(UPDATE_FREQ_NONE, binding = 12) uniform texture2D g_SkyBackgroudTexture;
layout(UPDATE_FREQ_NONE, binding = 13) uniform texture2D g_BlurTexture;
layout(UPDATE_FREQ_NONE, binding = 14) buffer TransmittanceColor
{
	vec4 TransmittanceColor_Data[];
};

layout(UPDATE_FREQ_NONE, binding = 15) uniform texture2D InputTex;

layout(UPDATE_FREQ_NONE, binding = 30, rgba32f) uniform image2D OutputTex;
layout(UPDATE_FREQ_NONE, binding = 31, rgba32f) uniform image2D volumetricCloudsDstTexture;
layout(UPDATE_FREQ_NONE, binding = 32, rgba32f) uniform image2D SavePrevTexture;

layout(UPDATE_FREQ_NONE, binding = 100) uniform sampler g_LinearClampSampler;
layout(UPDATE_FREQ_NONE, binding = 101) uniform sampler g_LinearWrapSampler;
layout(UPDATE_FREQ_NONE, binding = 102) uniform sampler g_PointClampSampler;
layout(UPDATE_FREQ_NONE, binding = 103) uniform sampler g_LinearBorderSampler;

layout(UPDATE_FREQ_PER_FRAME, binding = 110) uniform VolumetricCloudsCBuffer_Block
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
bool GetStartEndPointForRayMarching(vec3 ws_origin, vec3 ws_ray, float EARTH_RADIUS_ADD_CLOUDS_LAYER_END2, out vec3 start, out vec3 end)
{
    float ot1, ot2, it1, it2;
    (start = (end = vec3(0.0, 0.0, 0.0)));
    if((!ray_trace_sphere(ws_origin, ws_ray, _EARTH_CENTER, float (EARTH_RADIUS_ADD_CLOUDS_LAYER_END2), ot1, ot2)))
    {
        return false;
    }
    bool inIntersected = ray_trace_sphere(ws_origin, ws_ray, _EARTH_CENTER, float (_EARTH_RADIUS_ADD_CLOUDS_LAYER_START2), it1, it2);
    if(inIntersected)
    {
        float branchFactor = clamp(floor((it1 + float (1.0))), 0.0, 1.0);
        (start = (ws_origin + (vec3 (mix(max(it2, float (0)), max(ot1, float (0)), branchFactor)) * ws_ray)));
        (end = (ws_origin + (vec3 (mix(ot2, it1, branchFactor)) * ws_ray)));
    }
    else
    {
        (end = (ws_origin + (vec3 (ot2) * ws_ray)));
        (start = (ws_origin + (vec3 (max(ot1, float (0))) * ws_ray)));
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
vec3 getProjectedShellPoint(in vec3 pt, in vec3 center)
{
    return ((vec3 (_EARTH_RADIUS_ADD_CLOUDS_LAYER_START) * normalize((pt - center))) + center);
}
float getRelativeHeight(in vec3 pt, in vec3 projectedPt, float layer_thickness)
{
    return clamp((length((pt - projectedPt)) / float (layer_thickness)), 0.0, 1.0);
}
float getRelativeHeightAccurate(in vec3 pt, in vec3 projectedPt, float layer_thickness)
{
    float t = distance(pt, _EARTH_CENTER);
    (t -= float (_EARTH_RADIUS_ADD_CLOUDS_LAYER_START));
    return clamp((max(t, 0.0) / float (layer_thickness)), 0.0, 1.0);
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
    return clamp((max((distance - 150000.0), 0.0) / 50000.0), 0.0, 1.0);
}
float GetLODBias(float distance)
{
    float factor = 50000.0;
    return clamp((max((distance - factor), 0.0) / (float (MAX_SAMPLE_DISTANCE) - factor)), 0.0, 1.0);
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
float SampleDensity(vec3 worldPos, float lod, float height_fraction, vec3 currentProj, in vec3 cloudTopOffsetWithWindDir, in vec4 windWithVelocity, in vec3 biasedCloudPos, in float DetailShapeTilingDivCloudSize, bool cheap)
{
    vec3 unwindWorldPos = worldPos;
    (worldPos += (vec3 (height_fraction) * cloudTopOffsetWithWindDir));
    (worldPos += biasedCloudPos);
    vec3 weatherData = vec3 ((textureLod(sampler2D( weatherTexture, g_LinearWrapSampler), vec2((((unwindWorldPos).xz + windWithVelocity.xy + vec2(VolumetricCloudsCBuffer.g_VolumetricClouds.WeatherTextureOffsetX, VolumetricCloudsCBuffer.g_VolumetricClouds.WeatherTextureOffsetZ)) / vec2 ((VolumetricCloudsCBuffer.g_VolumetricClouds).WeatherTextureSize))), 0.0)).rgb);
    vec3 worldPosDivCloudSize = (worldPos / vec3 ((VolumetricCloudsCBuffer.g_VolumetricClouds).CloudSize));
    vec4 low_freq_noises = textureLod(sampler3D( lowFreqNoiseTexture, g_LinearWrapSampler), vec3((worldPosDivCloudSize * vec3 ((VolumetricCloudsCBuffer.g_VolumetricClouds).BaseShapeTiling))), lod);
    float low_freq_fBm = ((((low_freq_noises).g * float (0.625)) + ((low_freq_noises).b * float (0.25))) + ((low_freq_noises).a * float (0.125)));
    float base_cloud = RemapClamped((low_freq_noises).r, (low_freq_fBm - float (1.0)), float (1.0), float (0.0), float (1.0));
    (base_cloud = clamp((base_cloud + (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudCoverage), 0.0, 1.0));
    float cloudType = clamp(((weatherData).b + (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudType), 0.0, 1.0);
    float density_height_gradient = GetDensityHeightGradientForPoint(height_fraction, cloudType);
    (base_cloud *= density_height_gradient);
    float cloud_coverage = clamp((weatherData).g, 0.0, 1.0);
    (cloud_coverage = pow(cloud_coverage,float(RemapClamped(height_fraction, float (0.2), float (0.8), float (1.0), mix(float (1.0), float (0.5), (VolumetricCloudsCBuffer.g_VolumetricClouds).AnvilBias)))));
    float base_cloud_coverage = RemapClamped(base_cloud, cloud_coverage, float (1.0), float (0.0), float (1.0));
    (base_cloud_coverage *= cloud_coverage);
    float final_cloud = base_cloud_coverage;
    if((!cheap))
    {
        vec2 curl_noise = vec2 ((textureLod(sampler2D( curlNoiseTexture, g_LinearWrapSampler), vec2(vec2(((worldPosDivCloudSize).xz * vec2 ((VolumetricCloudsCBuffer.g_VolumetricClouds).CurlTextureTiling)))), int (0.0))).rg);
        ((worldPos).xz += ((curl_noise * vec2 ((float (1.0) - height_fraction))) * vec2 ((VolumetricCloudsCBuffer.g_VolumetricClouds).CurlStrenth)));
        worldPos.y -= ((VolumetricCloudsCBuffer.g_VolumetricClouds.TimeAndScreenSize.x * VolumetricCloudsCBuffer.g_VolumetricClouds.UpstreamSpeed) / (VolumetricCloudsCBuffer.g_VolumetricClouds.CloudSize * 0.0657 * VolumetricCloudsCBuffer.g_VolumetricClouds.UpstreamScale));

        vec3 high_frequency_noises = vec3 ((textureLod(sampler3D( highFreqNoiseTexture, g_LinearWrapSampler), vec3(vec3(((worldPos + vec3(windWithVelocity.z, 0.0, windWithVelocity.w))   * vec3 (DetailShapeTilingDivCloudSize)))), lod)).rgb);
        float high_freq_fBm = ((((high_frequency_noises).r * float (0.625)) + ((high_frequency_noises).g * float (0.25))) + ((high_frequency_noises).b * float (0.125)));
        float height_fraction_new = getRelativeHeight(worldPos, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.LayerThickness);
        float height_freq_noise_modifier = mix(high_freq_fBm, (float (1.0) - high_freq_fBm), clamp((height_fraction_new * float (10.0)), 0.0, 1.0));
        (final_cloud = RemapClamped(base_cloud_coverage, (height_freq_noise_modifier * (VolumetricCloudsCBuffer.g_VolumetricClouds).DetailStrenth), float (1.0), float (0.0), float (1.0)));
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
float SampleEnergy(vec3 rayPos, vec3 magLightDirection, float height_fraction, vec3 currentProj, in vec3 cloudTopOffsetWithWindDir, in vec4 windWithVelocity, in vec3 biasedCloudPos, in float DetailShapeTilingDivCloudSize, float ds_loded, float stepSize, float cosTheta, float mipBias)
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
        vec3 currentProj_new = getProjectedShellPoint(samplePoint, _EARTH_CENTER);
        float height_fraction_new = getRelativeHeight(samplePoint, currentProj_new, VolumetricCloudsCBuffer.g_VolumetricClouds.LayerThickness);
        (totalSample += SampleDensity(samplePoint, mipmapOffset, height_fraction_new, currentProj_new, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, false));
        (mipmapOffset += float (0.5));
        (step += 1.0);
    }
    float hg = max(HenryGreenstein((VolumetricCloudsCBuffer.g_VolumetricClouds).Eccentricity, cosTheta), (clamp(HenryGreenstein((0.99 - (VolumetricCloudsCBuffer.g_VolumetricClouds).SilverliningSpread), cosTheta), 0.0, 1.0))) * (VolumetricCloudsCBuffer.g_VolumetricClouds).SilverliningIntensity;
	
	float dl = totalSample * VolumetricCloudsCBuffer.g_VolumetricClouds.Precipitation;
	hg = hg / max(dl, 0.05);	
	
	float lodded_density = clamp(SampleDensity(rayPos, mipBias, height_fraction, currentProj, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, true), 0.0, 1.0);
    float energy = GetLightEnergy(height_fraction, dl, ds_loded, hg, cosTheta, stepSize, (VolumetricCloudsCBuffer.g_VolumetricClouds).Contrast);
    return energy;
}
float GetDensity(vec3 startPos, vec3 worldPos, vec3 dir, float maxSampleDistance, float raymarchOffset,
  float EARTH_RADIUS_ADD_CLOUDS_LAYER_END, float EARTH_RADIUS_ADD_CLOUDS_LAYER_END2,
 out float intensity, out float atmosphericBlendFactor, out float depth, vec2 uv)
{
    vec3 sampleStart, sampleEnd;
    (depth = float (0.0));
    (intensity = float (0.0));
    (atmosphericBlendFactor = 0.0);
    if((!GetStartEndPointForRayMarching(startPos, dir, EARTH_RADIUS_ADD_CLOUDS_LAYER_END2, sampleStart, sampleEnd)))
    {
        return float (0.0);
    }
    float horizon = abs((dir).y);
    uint sample_count = uint (mix(float ((VolumetricCloudsCBuffer.g_VolumetricClouds).MAX_ITERATION_COUNT), float ((VolumetricCloudsCBuffer.g_VolumetricClouds).MIN_ITERATION_COUNT), horizon));
    float sample_step = min((length((sampleEnd - sampleStart)) / float ((float (sample_count) * 0.43))), mix(((VolumetricCloudsCBuffer.g_VolumetricClouds).m_StepSize).y, ((VolumetricCloudsCBuffer.g_VolumetricClouds).m_StepSize).x, pow(horizon,float(0.33))));
    (depth = distance(sampleEnd, startPos));
    float distCameraToStart = distance(sampleStart, startPos);
    //(atmosphericBlendFactor = PackFloat16(distCameraToStart));
    if (UnPackFloat16(textureLod(sampler2D(g_LinearDepthTexture, g_LinearClampSampler), uv, float(0)).r) < FLOAT16_MAX)
    {
      atmosphericBlendFactor = 1.0f;
    }

    if(((sampleStart).y + VolumetricCloudsCBuffer.g_VolumetricClouds.Test01 < 0.0))
    {
        (intensity = float (0.0));
        return float (0.0);
    }
    /*
    if((distCameraToStart >= float (MAX_SAMPLE_DISTANCE)))
    {
        return float (0.0);
    }
    */
#if STEREO_INSTANCED
    vec2 textureSize = (((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw * vec2(0.5, 0.25));
#else
    vec2 textureSize = (((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw * vec2 (0.25));
#endif

    vec2 Corrected_UV = (uv - (vec2(1.0, 1.0) / textureSize));
    uvec2 texels = uvec2 ((vec2((VolumetricCloudsCBuffer.g_VolumetricClouds).DepthMapWidth, (VolumetricCloudsCBuffer.g_VolumetricClouds).DepthMapHeight) * uv));
    float sceneDepth;

    if (VolumetricCloudsCBuffer.g_VolumetricClouds.EnabledLodDepthCulling > 0.5f)
    {
      sceneDepth = texelFetch(depthTexture, ivec2(ivec2(texels)).xy + ivec2(0, 0), 0).r;

      if (UnPackFloat16(sceneDepth) < FLOAT16_MAX)
        return 1.0;
    }

    float alpha = 0.0;
    (intensity = 0.0);
    bool detailedSample = false;
    int missedStepCount = 0;
    float ds = 0.0;
    float bigStep = (sample_step * float (2.0));
    vec3 smallStepMarching = (vec3 (sample_step) * dir);
    vec3 bigStepMarching = (vec3 (bigStep) * dir);
    vec3 raymarchingDistance = (smallStepMarching * vec3 (raymarchOffset));
    vec3 magLightDirection = (vec3 (2.0) * ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);
    bool pickedFirstHit = false;
    float cosTheta = dot(dir, ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);
    vec3 rayPos = sampleStart;
    vec4 windWithVelocity = ((VolumetricCloudsCBuffer.g_VolumetricClouds).StandardPosition);
    vec3 biasedCloudPos = (vec3 (4.5) * (((VolumetricCloudsCBuffer.g_VolumetricClouds).WindDirection).xyz + vec3(0.0, 0.1, 0.0)));
    vec3 cloudTopOffsetWithWindDir = (vec3 ((VolumetricCloudsCBuffer.g_VolumetricClouds).CloudTopOffset) * ((VolumetricCloudsCBuffer.g_VolumetricClouds).WindDirection).xyz);
    float DetailShapeTilingDivCloudSize = ((VolumetricCloudsCBuffer.g_VolumetricClouds).DetailShapeTiling / (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudSize);
    float LODbias = 0.0;
    float cheapLOD = (LODbias + 2.0);
    for (uint j = uint (0); (j < sample_count); (j++))
    {
        (rayPos += raymarchingDistance);
        vec3 currentProj = getProjectedShellPoint(rayPos, _EARTH_CENTER);
        float DetailShapeTilingDivCloudSize = ((VolumetricCloudsCBuffer.g_VolumetricClouds).DetailShapeTiling / (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudSize);
        float height_fraction = getRelativeHeightAccurate(rayPos, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.LayerThickness);
        if((!detailedSample))
        {
            float sampleResult = SampleDensity(rayPos, cheapLOD, height_fraction, currentProj, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, true);
            if((sampleResult > 0.0))
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
            if((sampleResult == float (0.0)))
            {
                (missedStepCount++);
                if((missedStepCount > 10))
                {
                    (detailedSample = false);
                }
            }
            else
            {
                if((!pickedFirstHit))
                {
                    (depth = distance(rayPos, startPos));
                    (pickedFirstHit = true);
                }
                float sampledAlpha = (sampleResult * (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudDensity);
                float sampledEnergy = SampleEnergy(rayPos, magLightDirection, height_fraction, currentProj, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, sampleResult, ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).a, cosTheta, 0.0);
                float oneMinusAlpha = (float (1.0) - alpha);
                (sampledAlpha *= oneMinusAlpha);
                (intensity += (sampledAlpha * sampledEnergy));
                (alpha += sampledAlpha);
                if((alpha >= 1.0))
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
}

float GetDensityWithComparingDepth(vec3 startPos, vec3 worldPos, vec3 dir, float maxSampleDistance, float raymarchOffset, 
  float EARTH_RADIUS_ADD_CLOUDS_LAYER_END, float EARTH_RADIUS_ADD_CLOUDS_LAYER_END2, 
  out float intensity, out float atmosphericBlendFactor, out float depth, vec2 uv)
{
  vec3 sampleStart, sampleEnd;
  (depth = float(0.0));
  (intensity = float(0.0));
  (atmosphericBlendFactor = 0.0);
  if ((!GetStartEndPointForRayMarching(startPos, dir, EARTH_RADIUS_ADD_CLOUDS_LAYER_END2, sampleStart, sampleEnd)))
  {
    return float(0.0);
  }
  float horizon = abs((dir).y);
  uint sample_count = uint(mix(float((VolumetricCloudsCBuffer.g_VolumetricClouds).MAX_ITERATION_COUNT), float((VolumetricCloudsCBuffer.g_VolumetricClouds).MIN_ITERATION_COUNT), horizon));
  float sample_step = min((length((sampleEnd - sampleStart)) / float((float(sample_count) * 0.43))), mix(((VolumetricCloudsCBuffer.g_VolumetricClouds).m_StepSize).y, ((VolumetricCloudsCBuffer.g_VolumetricClouds).m_StepSize).x, pow(horizon, float(0.33))));
  (depth = distance(sampleEnd, startPos));
  float distCameraToStart = distance(sampleStart, startPos);
  //(atmosphericBlendFactor = PackFloat16(distCameraToStart));
  float sceneDepth = UnPackFloat16(textureLod(sampler2D(depthTexture, g_LinearClampSampler), vec2(uv), float(0)).r);
  if (sceneDepth < FLOAT16_MAX)
  {
    atmosphericBlendFactor = 1.0f;
  }

  if (((sampleStart).y + VolumetricCloudsCBuffer.g_VolumetricClouds.Test01 < 0.0))
  {
    (intensity = float(0.0));
    return float(0.0);
  }
/*
  if ((distCameraToStart >= float(MAX_SAMPLE_DISTANCE)))
  {
    return float(0.0);
  }
*/
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
  float bigStep = (sample_step * float(2.0));
  vec3 smallStepMarching = (vec3(sample_step) * dir);
  vec3 bigStepMarching = (vec3(bigStep) * dir);
  vec3 raymarchingDistance = (smallStepMarching * vec3(raymarchOffset));
  vec3 magLightDirection = (vec3(2.0) * ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);
  bool pickedFirstHit = false;
  float cosTheta = dot(dir, ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz);
  vec3 rayPos = sampleStart;
  vec4 windWithVelocity = ((VolumetricCloudsCBuffer.g_VolumetricClouds).StandardPosition);
  vec3 biasedCloudPos = (vec3(4.5) * (((VolumetricCloudsCBuffer.g_VolumetricClouds).WindDirection).xyz + vec3(0.0, 0.1, 0.0)));
  vec3 cloudTopOffsetWithWindDir = (vec3((VolumetricCloudsCBuffer.g_VolumetricClouds).CloudTopOffset) * ((VolumetricCloudsCBuffer.g_VolumetricClouds).WindDirection).xyz);
  float DetailShapeTilingDivCloudSize = ((VolumetricCloudsCBuffer.g_VolumetricClouds).DetailShapeTiling / (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudSize);
  float LODbias = 0.0;
  float cheapLOD = (LODbias + 2.0);
  for (uint j = uint(0); (j < sample_count); (j++))
  {
    (rayPos += raymarchingDistance);

    if (sceneDepth < distance(startPos, rayPos))
    {
      break;
    }

    vec3 currentProj = getProjectedShellPoint(rayPos, _EARTH_CENTER);
    float height_fraction = getRelativeHeightAccurate(rayPos, currentProj, VolumetricCloudsCBuffer.g_VolumetricClouds.LayerThickness);
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
        float sampledAlpha = (sampleResult * (VolumetricCloudsCBuffer.g_VolumetricClouds).CloudDensity);
        float sampledEnergy = SampleEnergy(rayPos, magLightDirection, height_fraction, currentProj, cloudTopOffsetWithWindDir, windWithVelocity, biasedCloudPos, DetailShapeTilingDivCloudSize, sampleResult, ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).a, cosTheta, 0.0);
        float oneMinusAlpha = (float(1.0) - alpha);
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