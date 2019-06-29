/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer uniformGlobalInfoRootConstant : register(b0)
{
	float2 _Time;
	float2 screenSize;
	float4 lightPosition;
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

struct VSInput
{
   uint VertexID : SV_VertexID;
};

struct VSOutput {
	float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
	float2 VSray: TEXCOORD1;
};

VSOutput main(VSInput input)
{
	VSOutput Out;

   // Produce a fullscreen triangle
	float4 position;
	position.x = (input.VertexID == 2) ? 3.0 : -1.0;
	position.y = (input.VertexID == 0) ? -3.0 : 1.0;
	position.zw = 1.0;

	Out.Position = position;
	Out.TexCoord = position.xy * float2(0.5, -0.5) + 0.5;
	Out.VSray = position.xy * ProjectionExtents.xy + ProjectionExtents.zw;
	return Out;
}
