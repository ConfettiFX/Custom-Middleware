/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

Texture2D highResCloudTexture : register(t0);
SamplerState BilinearClampSampler : register(s0);

cbuffer uniformGlobalInfoRootConstant : register(b0)
{
	float2 _Time;
	float2 screenSize;
	float4 lightDirection;
	float4 lightColorAndIntensity;
	float4 cameraPosition;

	float4x4 VP;
	float4x4 InvVP;
	float4x4 InvWorldToCamera;
	float4x4 prevVP;
	float4x4 LP;

	float near;
	float far;

	float correctU;
	float correctV;

	float4 ProjectionExtents;
	uint lowResFrameIndex;

	uint JitterX;
	uint JitterY;

	float exposure;
	float decay;
	float density;
	float weight;

	uint NUM_SAMPLES;

	float4 skyBetaR;
	float4 skyBetaV;

	float turbidity;
	float rayleigh;
	float mieCoefficient;
	float mieDirectionalG;
}

struct PSIn {
	float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
	float2 VSray: TEXCOORD1;
};

float4 main(PSIn input) : SV_TARGET
{
	float4 result = highResCloudTexture.Sample(BilinearClampSampler, input.TexCoord);
	return result;
}