/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer SpaceUniform : register(b1, UPDATE_FREQ_PER_FRAME)
{
	float4x4 ViewProjMat;
	float4 LightDirection;
  float4 ScreenSize;
  float4 NebulaHighColor;
  float4 NebulaMidColor;
  float4 NebulaLowColor;
};

struct VSInput
{
    float3 Position     : POSITION;
    float3 Normal       : NORMAL;
    float3 StarInfo     : TEXCOORD;
};

struct VSOutput
{
	float4 Position       : SV_POSITION;
	float4 Normal         : NORMAL;
  float4 Info          : COLOR;
	float2 ScreenCoord    : TexCoord;
};

VSOutput main(VSInput Input){
	
	VSOutput result;
	result.Position = mul(ViewProjMat, float4(Input.Position, 1.0));	
	result.Normal = float4(Input.Normal, 0.0);
	result.ScreenCoord = result.Position.xy * float2(0.5, -0.5) + 0.5;  
  result.Info = float4(Input.StarInfo.x, Input.StarInfo.y, Input.StarInfo.z, 1.0);
  return result;
}