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
        float4x4 projView;
    };
    constant Uniforms_cbRootConstant & cbRootConstant;
    struct VSOutput
    {
        float4 Position [[position]];
        float3 pos;
        float3 normal;
        float2 uv;
    };
    float4 main(VSOutput input)
    {
        float NoL = saturate(dot((input).normal, float3(0.0, 1.0, 0.0)));
        return float4(NoL, NoL, NoL, 1.0);
    };

    Fragment_Shader(
constant Uniforms_cbRootConstant & cbRootConstant) :
cbRootConstant(cbRootConstant) {}
};


fragment float4 stageMain(
    Fragment_Shader::VSOutput input [[stage_in]],
    constant Fragment_Shader::Uniforms_cbRootConstant & cbRootConstant [[buffer(1)]])
{
    Fragment_Shader::VSOutput input0;
    input0.Position = float4(input.Position.xyz, 1.0 / input.Position.w);
    input0.pos = input.pos;
    input0.normal = input.normal;
    input0.uv = input.uv;
    Fragment_Shader main(
    cbRootConstant);
    return main.main(input0);
}
