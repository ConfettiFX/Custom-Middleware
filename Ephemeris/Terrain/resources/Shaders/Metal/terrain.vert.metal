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

struct Vertex_Shader
{
    struct Uniforms_cbRootConstant
    {
        float4x4 projView;
    };
    constant Uniforms_cbRootConstant & cbRootConstant;
    struct VSInput
    {
        float3 Position [[attribute(0)]];
        float2 Uv [[attribute(1)]];
    };
    struct VSOutput
    {
        float4 Position [[position]];
        float3 pos;
        float3 normal;
        float2 uv;
    };
    VSOutput main(VSInput input)
    {
        VSOutput result;
        (((input).Position).xyz *= (float3)(10.0));
        ((result).Position = ((cbRootConstant.projView)*(float4(((input).Position).xyz, 1.0))));
        ((result).pos = ((input).Position).xyz);
        ((result).normal = float3(0.0, 1.0, 0.0));
        ((result).uv = (input).Uv);
        return result;
    };

    Vertex_Shader(
constant Uniforms_cbRootConstant & cbRootConstant) :
cbRootConstant(cbRootConstant) {}
};


vertex Vertex_Shader::VSOutput stageMain(
    Vertex_Shader::VSInput input [[stage_in]],
    constant Vertex_Shader::Uniforms_cbRootConstant & cbRootConstant [[buffer(1)]])
{
    Vertex_Shader::VSInput input0;
    input0.Position = input.Position;
    input0.Uv = input.Uv;
    Vertex_Shader main(
    cbRootConstant);
    return main.main(input0);
}
