/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer cbRootConstant : register(b0) 
{
	float4x4 projView;
}


struct VSInput
{
	float3 Position : POSITION;
	float2 Uv		: TEXCOORD0;
};

struct VSOutput 
{
	float4 Position	: SV_POSITION;
	float3 pos		: POSITION;
	float3 normal	: NORMAL;
	float2 uv		: TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput result;

	input.Position.xyz *= 10.0f;

	result.Position = mul(projView, float4(input.Position.xyz, 1.0f));

	result.pos = input.Position.xyz;
	result.normal = float3(0.0, 1.0, 0.0);
	result.uv = input.Uv;
	return result;
}
