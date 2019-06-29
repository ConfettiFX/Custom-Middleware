/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer SpaceUniform : register(b1)
{
	float4x4 ViewProjMat;
	float4 LightDirection;
  float4 ScreenSize;
};

struct VSInput
{
    float4 Position : POSITION;
    float4 Normal : NORMAL;
};

struct VSOutput {
	float4 Position : SV_POSITION;
	float4 Normal : NORMAL;
    float4 Color : COLOR;
	float2 ScreenCoord : TexCoord;
};

VSOutput main(VSInput Input){
	
	VSOutput result;

	Input.Position.xyz *= 100000000.0f;

	result.Position = mul(ViewProjMat, Input.Position);
	
	result.Normal = Input.Normal;

	result.ScreenCoord = result.Position.xy * float2(0.5, -0.5) + 0.5;

    return result;
}