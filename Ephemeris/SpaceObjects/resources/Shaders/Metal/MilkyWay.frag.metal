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

struct Uniforms_SpaceUniform
{
	float4x4 ViewProjMat;
	float4 LightDirection;
	float4 ScreenSize;
	float4 NebulaHighColor;
	float4 NebulaMidColor;
	float4 NebulaLowColor;
};

struct VSOutput
{
	float4 Position     [[position]];
	float4 Normal;
    float4 Info;
	float2 ScreenCoord;
};

struct ArgsData
{
	texture2d<float> depthTexture;
};

struct ArgsPerFrame
{
	constant Uniforms_SpaceUniform & SpaceUniform;
};

fragment float4 stageMain(
    VSOutput In [[stage_in]],
	constant ArgsData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
	constant ArgsPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
	float depth = argBufferStatic.depthTexture.read((uint2)In.Position.xy).x;
	if(depth < 1.0)
		discard_fragment();

	float3 resultSpaceColor = (argBufferPerFrame.SpaceUniform.NebulaLowColor.rgb * argBufferPerFrame.SpaceUniform.NebulaLowColor.a * In.Info.x) + (argBufferPerFrame.SpaceUniform.NebulaMidColor.rgb * argBufferPerFrame.SpaceUniform.NebulaLowColor.a * In.Info.y) + (argBufferPerFrame.SpaceUniform.NebulaHighColor.rgb * argBufferPerFrame.SpaceUniform.NebulaHighColor.a * In.Info.z);

	return float4( float3(0.5, 0.2, 0.1), clamp(clamp(-argBufferPerFrame.SpaceUniform.LightDirection.y + 0.2f, 0.0f, 1.0f) * 2.0f, 0.0f, 1.0f));
}
