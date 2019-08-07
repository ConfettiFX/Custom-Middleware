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


fragment float4 stageMain(
    VSOutput In [[stage_in]],
    constant Uniforms_SpaceUniform & SpaceUniform [[buffer(3)]],
    texture2d<float> depthTexture [[texture(4)]])
{
	float depth = depthTexture.read((uint2)In.Position.xy).x;
	if(depth < 1.0)
		discard_fragment();

	float3 resultSpaceColor = (SpaceUniform.NebulaLowColor.rgb * SpaceUniform.NebulaLowColor.a * In.Info.x) + (SpaceUniform.NebulaMidColor.rgb * SpaceUniform.NebulaLowColor.a * In.Info.y) + (SpaceUniform.NebulaHighColor.rgb * SpaceUniform.NebulaHighColor.a * In.Info.z);

	return float4( float3(0.5, 0.2, 0.1), clamp(clamp(-SpaceUniform.LightDirection.y + 0.2f, 0.0f, 1.0f) * 2.0f, 0.0f, 1.0f));
}
