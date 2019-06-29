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
    struct spaceUniform
    {
        float4x4 ViewProjMat;
        float4 LightDirection;
        float4 ScreenSize;
    };
    constant spaceUniform & SpaceUniform;
    struct VSInput
    {
        float4 Position [[attribute(0)]];
        float4 Normal [[attribute(1)]];
    };
    struct VSOutput
    {
        float4 Position [[position]];
        float4 Normal;
        float4 Color;
        float2 ScreenCoord;
    };
    VSOutput main(VSInput Input)
    {
        VSOutput result;
        (((Input).Position).xyz *= (float3)(1.000000e+08));
        ((result).Position = ((SpaceUniform.ViewProjMat)*((Input).Position)));
        ((result).Normal = (Input).Normal);
        ((result).ScreenCoord = ((((result).Position).xy * float2(0.5, (-0.5))) + (float2)(0.5)));
        return result;
    };

    Vertex_Shader(
constant spaceUniform & SpaceUniform) :
SpaceUniform(SpaceUniform) {}
};


vertex Vertex_Shader::VSOutput stageMain(
    Vertex_Shader::VSInput Input [[stage_in]],
    constant Vertex_Shader::spaceUniform & SpaceUniform [[buffer(3)]])
{
    Vertex_Shader::VSInput Input0;
    Input0.Position = Input.Position;
    Input0.Normal = Input.Normal;
    Vertex_Shader main(
    SpaceUniform);
    return main.main(Input0);
}
