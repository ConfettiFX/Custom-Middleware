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

#include "space_argument_buffers.h"

struct Fragment_Shader
{
    constant Uniforms_cbRootConstant & cbRootConstant;
    struct PsIn
    {
        float4 position [[position]];
        float3 texCoord;
        float2 screenCoord;
    };
    texture2d<float> depthTexture;
    texture2d<float> moonAtlas;
	texture2d<float> volumetricCloudsTexture;
    sampler g_LinearBorder;
    float4 main(PsIn In)
    {
        float2 screenUV = (In).screenCoord;
        float sceneDepth = depthTexture.sample(g_LinearBorder, screenUV, level(0)).r;
        if ((sceneDepth < (float)(1.0)))
        {
            discard_fragment();
        }

        float density = volumetricCloudsTexture.sample(g_LinearBorder, screenUV, level(0)).a;

        float ISun = 1.0;
        float4 res;

        if (In.texCoord.z >= 0.0)
        {
            float2 uv = In.texCoord.xy * 2.0 - float2(0.5, 0.5);
            float param = 2.0f * sqrt((uv.x-0.5)*(uv.x-0.5)+(uv.y-0.5)*(uv.y-0.5));
            float blendFactor = smoothstep(1, 0.8, param);
            res = float4((float3(1.0, 0.95294, 0.91765) * (float3)(ISun)), blendFactor);
        }
        else
        {
            float3 normal;
            ((normal).xy = ((((In).texCoord).xy - (float2)(0.5)) * (float2)(2)));
            normal.z = 1.0f - sqrt(clamp(normal.x * normal.x + normal.y * normal.y, 0.0f, 1.0f));
            res = moonAtlas.sample(g_LinearBorder, In.texCoord.xy * 2.0 - float2(0.5, 0.5));
        }

         float2 glow = clamp(abs(In.texCoord.xy * 1.5 - float2(0.75, 0.75)), 0.0f, 1.0f);
   
        float gl = clamp(1.0f - sqrt(dot(glow, glow)), 0.0f, 1.0f);
        gl = pow(gl, 2.1) * 1.5;
		    res = mix(float4(gl, gl, gl, res.a), res, res.a);
        res.a = clamp((gl + res.a) * (1.0 - density), 0.0f ,1.0f);
        return res;
    };

    Fragment_Shader(
constant Uniforms_cbRootConstant & SunUniform,
texture2d<float> depthTexture,
texture2d<float> moonAtlas,
texture2d<float> volumetricCloudsTexture,
sampler g_LinearBorder) :
cbRootConstant(SunUniform),
depthTexture(depthTexture),
moonAtlas(moonAtlas),
volumetricCloudsTexture(volumetricCloudsTexture),
g_LinearBorder(g_LinearBorder) {}
};

fragment float4 stageMain(
    Fragment_Shader::PsIn In [[stage_in]],
    constant ArgData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant ArgDataPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    Fragment_Shader::PsIn In0;
    In0.position = float4(In.position.xyz, 1.0 / In.position.w);
    In0.texCoord = In.texCoord;
    In0.screenCoord = In.screenCoord;
    Fragment_Shader main(
    argBufferPerFrame.SunUniform,
    argBufferStatic.depthTexture,
    argBufferStatic.moonAtlas,
    argBufferStatic.volumetricCloudsTexture,
    argBufferStatic.g_LinearBorder);
    return main.main(In0);
}
