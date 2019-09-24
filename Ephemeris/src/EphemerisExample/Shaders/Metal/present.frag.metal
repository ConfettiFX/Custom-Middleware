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
    texture2d<float> sceneTexture;
    sampler BilinearClampSampler;
    struct PSIn
    {
        float4 Position [[position]];
        float2 TexCoord;
    };
    float4 main(PSIn input)
    {
        float4 sceneColor = sceneTexture.sample(BilinearClampSampler, (input).TexCoord);
        return sceneColor;
    };

    Fragment_Shader(
texture2d<float> sceneTexture,sampler BilinearClampSampler) :
sceneTexture(sceneTexture),BilinearClampSampler(BilinearClampSampler) {}
};

struct ArgsData
{
	texture2d<float> SrcTexture;
	sampler g_LinearClamp;
};

fragment float4 stageMain(
    Fragment_Shader::PSIn input [[stage_in]],
    constant ArgsData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]]
)
{
    Fragment_Shader::PSIn input0;
    input0.Position = float4(input.Position.xyz, 1.0 / input.Position.w);
    input0.TexCoord = input.TexCoord;
    Fragment_Shader main(
    argBufferStatic.SrcTexture,
    argBufferStatic.g_LinearClamp);
    return main.main(input0);
}
