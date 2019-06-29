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

struct Fragment_Shader
{
    struct Uniforms_LightingTerrainUniformBuffer
    {
        float4x4 InvViewProjMat;
        float4x4 ShadowViewProjMat;
        float4 ShadowSpheres;
        float4 LightDirection;
        float4 SunColor;
        float4 LightColor;
    };
    constant Uniforms_LightingTerrainUniformBuffer & LightingTerrainUniformBuffer;
    struct VolumetricCloudsShadowCB
    {
        float4 SettingInfo00;
        float4 StandardPosition;
        float4 ShadowInfo;
    };
    struct Uniforms_VolumetricCloudsShadowCB
    {
        VolumetricCloudsShadowCB g_VolumetricCloudsShadow;
    };
    constant Uniforms_VolumetricCloudsShadowCB & VolumetricCloudsShadowCB;
    texture2d<float> BasicTexture;
    texture2d<float> NormalTexture;
    texture2d<float> weatherTexture;
    texture2d<float> depthTexture;
    sampler g_LinearMirror;
    sampler g_LinearWrap;
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
#define USE_PROJECTED_SHADOW 1
#define _EARTH_RADIUS_ADD_CLOUDS_LAYER_START 5.015000e+06
#define _EARTH_RADIUS 5.000000e+06
#define _EARTH_CENTER float3(0, (-_EARTH_RADIUS), 0)
    float EvaluateCloudShadow(float3 ws_pos, float3 lightDir)
    {
#if USE_PROJECTED_SHADOW
        float it1, it2;
        ray_trace_sphere((ws_pos).xyz, lightDir, _EARTH_CENTER, _EARTH_RADIUS_ADD_CLOUDS_LAYER_START, it1, it2);
        float3 CloudPos = ((ws_pos).xyz + (lightDir * (float3)(it2)));
        float3 weatherData = weatherTexture.sample(g_LinearWrap, (((CloudPos).xz + ((VolumetricCloudsShadowCB.g_VolumetricCloudsShadow).StandardPosition).xz) / (float2)(((VolumetricCloudsShadowCB.g_VolumetricCloudsShadow).SettingInfo00).z)), level(0)).rgb;
        float result = saturate((saturate(((weatherData).b - ((VolumetricCloudsShadowCB.g_VolumetricCloudsShadow).SettingInfo00).y)) + ((VolumetricCloudsShadowCB.g_VolumetricCloudsShadow).StandardPosition).w));
        float result2 = (result * result);
        return (result2 * result2);
#else
        float3 weatherData = weatherTexture.sample(g_LinearWrap, (((ws_pos).xz + ((VolumetricCloudsShadowCB.g_VolumetricCloudsShadow).StandardPosition).xz) / (float2)(((VolumetricCloudsShadowCB.g_VolumetricCloudsShadow).SettingInfo00).z)), level(0)).rgb;
        return saturate(((weatherData).g + ((VolumetricCloudsShadowCB.g_VolumetricCloudsShadow).StandardPosition).w));
#endif

    };
    struct PsIn
    {
        float4 position [[position]];
        float2 screenPos;
    };
    float4 main(PsIn In)
    {
        float2 texCoord = (((In).screenPos * (float2)(0.5)) - (float2)(0.5));
        ((texCoord).x = ((float)(1.0) - (texCoord).x));
        float3 albedo = BasicTexture.sample(g_LinearMirror, texCoord).rgb;
        float3 normal = NormalTexture.sample(g_LinearMirror, texCoord).xyz;
        float lighting = saturate(dot(normal, (LightingTerrainUniformBuffer.LightDirection).xyz));
        float2 texCoord2 = float2(((((In).screenPos).x + (float)(1.0)) * (float)(0.5)), (((float)(1.0) - ((In).screenPos).y) * (float)(0.5)));
        float4 ws_pos = ((LightingTerrainUniformBuffer.InvViewProjMat)*(float4(((In).screenPos).xy, depthTexture.sample(g_LinearWrap, texCoord2).r, 1.0)));
        float shadow_atten_from_clouds = EvaluateCloudShadow((ws_pos).xyz, (LightingTerrainUniformBuffer.LightDirection).xyz);
        return float4((((((((float3)(lighting) * albedo) * (LightingTerrainUniformBuffer.SunColor).rgb) * (LightingTerrainUniformBuffer.LightColor).rgb) * (float3)((LightingTerrainUniformBuffer.LightColor).a)) * (float3)(shadow_atten_from_clouds)) + (albedo * (float3)(0.1))), 1.0);
    };

    Fragment_Shader(
constant Uniforms_LightingTerrainUniformBuffer & LightingTerrainUniformBuffer,constant Uniforms_VolumetricCloudsShadowCB & VolumetricCloudsShadowCB,texture2d<float> BasicTexture,texture2d<float> NormalTexture,texture2d<float> weatherTexture,texture2d<float> depthTexture,sampler g_LinearMirror,sampler g_LinearWrap) :
LightingTerrainUniformBuffer(LightingTerrainUniformBuffer),VolumetricCloudsShadowCB(VolumetricCloudsShadowCB),BasicTexture(BasicTexture),NormalTexture(NormalTexture),weatherTexture(weatherTexture),depthTexture(depthTexture),g_LinearMirror(g_LinearMirror),g_LinearWrap(g_LinearWrap) {}
};


fragment float4 stageMain(
    Fragment_Shader::PsIn In [[stage_in]],
    constant Fragment_Shader::Uniforms_LightingTerrainUniformBuffer & LightingTerrainUniformBuffer [[buffer(3)]],
    constant Fragment_Shader::Uniforms_VolumetricCloudsShadowCB & VolumetricCloudsShadowCB [[buffer(4)]],
    texture2d<float> BasicTexture [[texture(14)]],
    texture2d<float> NormalTexture [[texture(15)]],
    texture2d<float> weatherTexture [[texture(16)]],
    texture2d<float> depthTexture [[texture(17)]],
    sampler g_LinearMirror [[sampler(0)]],
    sampler g_LinearWrap [[sampler(1)]])
{
    Fragment_Shader::PsIn In0;
    In0.position = float4(In.position.xyz, 1.0 / In.position.w);
    In0.screenPos = In.screenPos;
    Fragment_Shader main(
    LightingTerrainUniformBuffer,
    VolumetricCloudsShadowCB,
    BasicTexture,
    NormalTexture,
    weatherTexture,
    depthTexture,
    g_LinearMirror,
    g_LinearWrap);
    return main.main(In0);
}
