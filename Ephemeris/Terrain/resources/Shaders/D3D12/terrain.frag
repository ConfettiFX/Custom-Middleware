/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

struct VSOutput 
{
	float4 Position : SV_POSITION;
	float3 pos		: POSITION;
	float3 normal	: NORMAL;
	float2 uv		: TEXCOORD0;
};


float4 main(VSOutput input) : SV_TARGET
{	
	float NoL = saturate(dot(input.normal, float3(0.0, 1.0, 0.0)));

	return float4(NoL, NoL, NoL, 1.0f);
}
