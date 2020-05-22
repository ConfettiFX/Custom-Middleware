/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include <metal_stdlib>
#include <metal_compute>
using namespace metal;

#include "space_argument_buffers.h"

struct VSInput
{
  float3 Position     [[attribute(0)]];
  float3 Normal       [[attribute(1)]];
  float3 StarInfo     [[attribute(2)]];
};

struct VSOutput
{
	float4 Position     [[position]];
	float4 Normal;
	float4 Info;
	float2 ScreenCoord;
};

vertex VSOutput stageMain(uint VertexID [[vertex_id]], VSInput Input [[stage_in]], uint InstanceID [[instance_id]], constant ArgDataPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]])
{
  VSOutput result;
	result.Position = argBufferPerFrame.SpaceUniform.ViewProjMat * float4(Input.Position, 1.0);
	result.Normal = float4(Input.Normal, 0.0);
	result.ScreenCoord = result.Position.xy * float2(0.5, -0.5) + 0.5;  
  result.Info = float4(Input.StarInfo.x, Input.StarInfo.y, Input.StarInfo.z, 1.0);
  return result;
}
