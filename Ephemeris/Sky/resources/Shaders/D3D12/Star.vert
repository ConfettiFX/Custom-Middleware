/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

struct VSInput
{
  float4 Position : POSITION;
  float2 Tex      : TEXCOORD;
  float4 ParticlePositions : TEXCOORD1;
  float4 ParticleColors : TEXCOORD2;
  float4 ParticleInfo : TEXCOORD3;
};

struct GsIn {
	float3 Position    : Position;
	float4 Color       : Color;
	float4 StarInfo    : Texcoord;
};

GsIn main(VSInput Input)
{
	GsIn Out;
	Out.Position = Input.ParticlePositions.xyz;
	Out.Color = Input.ParticleColors;
	Out.StarInfo = Input.ParticleInfo;
	return Out;
}