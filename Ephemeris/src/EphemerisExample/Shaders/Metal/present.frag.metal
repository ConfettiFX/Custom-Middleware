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
    texture2d<float> skydomeTexture;
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
texture2d<float> skydomeTexture,texture2d<float> sceneTexture,sampler BilinearClampSampler) :
skydomeTexture(skydomeTexture),sceneTexture(sceneTexture),BilinearClampSampler(BilinearClampSampler) {}
};


fragment float4 stageMain(
    Fragment_Shader::PSIn input [[stage_in]],
    texture2d<float> skydomeTexture [[texture(0)]],
    texture2d<float> sceneTexture [[texture(1)]],
    sampler BilinearClampSampler [[sampler(0)]])
{
    Fragment_Shader::PSIn input0;
    input0.Position = float4(input.Position.xyz, 1.0 / input.Position.w);
    input0.TexCoord = input.TexCoord;
    Fragment_Shader main(
    skydomeTexture,
    sceneTexture,
    BilinearClampSampler);
    return main.main(input0);
}
