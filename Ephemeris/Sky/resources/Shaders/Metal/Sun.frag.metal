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
    struct Uniforms_cbRootConstant
    {
        float4x4 ViewMat;
        float4x4 ViewProjMat;
        float4 LightDirection;
        float4 Dx;
        float4 Dy;
    };
    constant Uniforms_cbRootConstant & cbRootConstant;
    struct PsIn
    {
        float4 position [[position]];
        float3 texCoord;
        float2 screenCoord;
    };
    texture2d<float> depthTexture;
    texture2d<float> moonAtlas;
    sampler g_LinearClampSampler;
    float4 main(PsIn In)
    {
        float2 screenUV = (In).screenCoord;
        float sceneDepth = depthTexture.sample(g_LinearClampSampler, screenUV, level(0)).r;
        if ((sceneDepth < (float)(1.0)))
        {
            discard_fragment();
        }
        float ISun = 1.0;
        if (In.texCoord.z >= 0.0)
        {
            float param = ((float)(2) * sqrt((((((In).texCoord).x - (float)(0.5)) * (((In).texCoord).x - (float)(0.5))) + ((((In).texCoord).y - (float)(0.5)) * (((In).texCoord).y - (float)(0.5))))));
            float blendFactor = smoothstep(1, 0.8, param);
            return float4((float3(1.0, 0.95294, 0.91765) * (float3)(ISun)), blendFactor);
        }
        else
        {
            float3 normal;
            ((normal).xy = ((((In).texCoord).xy - (float2)(0.5)) * (float2)(2)));
            ((normal).z = ((float)(1) - sqrt(saturate((((normal).x * (normal).x) + ((normal).y * (normal).y))))));
            float4 res = moonAtlas.sample(g_LinearClampSampler, ((In).texCoord).xy);
            return res;
        }
    };

    Fragment_Shader(
constant Uniforms_cbRootConstant & SunUniform,
texture2d<float> depthTexture,
texture2d<float> moonAtlas,
sampler g_LinearClampSampler) :
cbRootConstant(SunUniform),
depthTexture(depthTexture),
moonAtlas(moonAtlas),
g_LinearClampSampler(g_LinearClampSampler) {}
};


fragment float4 stageMain(
    Fragment_Shader::PsIn In [[stage_in]],
    constant Fragment_Shader::Uniforms_cbRootConstant & SunUniform [[buffer(4)]],
    texture2d<float> depthTexture [[texture(4)]],
    texture2d<float> moonAtlas [[texture(5)]],
    sampler g_LinearClampSampler [[sampler(1)]])
{
    Fragment_Shader::PsIn In0;
    In0.position = float4(In.position.xyz, 1.0 / In.position.w);
    In0.texCoord = In.texCoord;
    In0.screenCoord = In.screenCoord;
    Fragment_Shader main(
    SunUniform,
    depthTexture,
    moonAtlas,
    g_LinearClampSampler);
    return main.main(In0);
}
