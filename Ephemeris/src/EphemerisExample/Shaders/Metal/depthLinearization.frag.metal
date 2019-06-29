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
    texture2d<float> SrcTexture;
    sampler g_LinearClamp;
    struct Uniforms_CameraInfoRootConstant
    {
        float nearPlane;
        float farPlane;
        float padding00;
        float padding01;
    };
    constant Uniforms_CameraInfoRootConstant & CameraInfoRootConstant;
    float DepthLinearization(float depth)
    {
        return (((float)(2.0) * CameraInfoRootConstant.nearPlane) / ((CameraInfoRootConstant.farPlane + CameraInfoRootConstant.nearPlane) - (depth * (CameraInfoRootConstant.farPlane - CameraInfoRootConstant.nearPlane))));
    };
    struct PSIn
    {
        float4 Position [[position]];
        float2 TexCoord;
    };
    float main(PSIn input)
    {
        return (DepthLinearization(SrcTexture.sample(g_LinearClamp, (input).TexCoord).r) * CameraInfoRootConstant.farPlane);
    };

    Fragment_Shader(
texture2d<float> SrcTexture,sampler g_LinearClamp,constant Uniforms_CameraInfoRootConstant & CameraInfoRootConstant) :
SrcTexture(SrcTexture),g_LinearClamp(g_LinearClamp),CameraInfoRootConstant(CameraInfoRootConstant) {}
};


fragment float4 stageMain(
    Fragment_Shader::PSIn input [[stage_in]],
    texture2d<float> SrcTexture [[texture(0)]],
    sampler g_LinearClamp [[sampler(0)]],
    constant Fragment_Shader::Uniforms_CameraInfoRootConstant & CameraInfoRootConstant [[buffer(1)]])
{
    Fragment_Shader::PSIn input0;
    input0.Position = float4(input.Position.xyz, 1.0 / input.Position.w);
    input0.TexCoord = input.TexCoord;
    Fragment_Shader main(
    SrcTexture,
    g_LinearClamp,
    CameraInfoRootConstant);
    return main.main(input0);
}
