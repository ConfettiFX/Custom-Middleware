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

#include "RenderSky.h"

struct Vertex_Shader
{
	constant RenderSky::spaceUniform & SpaceUniform;
    struct VSInput
    {
        float3 Position [[attribute(0)]];
        float3 Normal [[attribute(1)]];
        float3 StarInfo [[attribute(2)]];
    };
    struct VSOutput
    {
        float4 Position [[position]];
        float4 Normal;
        float4 Info;
        float2 ScreenCoord;
    };
    VSOutput main(VSInput Input)
    {
        VSOutput result;
        
        result.Position = SpaceUniform.ViewProjMat * float4(Input.Position.x, Input.Position.y, Input.Position.z, 1.0f);
        result.Normal = float4(Input.Normal.x,Input.Normal.y,Input.Normal.z,0.0);
        ((result).ScreenCoord = ((((result).Position).xy * float2(0.5, (-0.5))) + (float2)(0.5)));
        result.Info = float4(Input.StarInfo.x, Input.StarInfo.y, Input.StarInfo.z, 1.0);
        return result;
    };

    Vertex_Shader(
constant RenderSky::spaceUniform & SpaceUniform) :
SpaceUniform(SpaceUniform) {}
};

vertex Vertex_Shader::VSOutput stageMain(
    Vertex_Shader::VSInput Input [[stage_in]],
	 constant RenderSky::ArgDataPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    Vertex_Shader::VSInput Input0;
    Input0.Position = Input.Position;
    Input0.Normal = Input.Normal;
    Input0.StarInfo = Input.StarInfo;
    Vertex_Shader main(argBufferPerFrame.SpaceUniform);
    return main.main(Input0);
}
