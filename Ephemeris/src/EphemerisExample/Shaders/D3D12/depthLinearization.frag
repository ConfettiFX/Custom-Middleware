/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

Texture2D<float> SrcTexture : register(t0);
SamplerState g_LinearClamp : register(s0);

cbuffer CameraInfoRootConstant : register(b0)
{
	float nearPlane;
	float farPlane;
	float padding00;
	float padding01;
};

float DepthLinearization(float depth)
{
	return (2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane));
}

float PackFloat16(float value)
{
  return value * 0.001f;
}

float UnPackFloat16(float value)
{
  return value * 1000.0f;
}

struct PSIn {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

float main(PSIn input) : SV_TARGET
{
	return PackFloat16(DepthLinearization(SrcTexture.Sample(g_LinearClamp, input.TexCoord).r) * farPlane);
}

