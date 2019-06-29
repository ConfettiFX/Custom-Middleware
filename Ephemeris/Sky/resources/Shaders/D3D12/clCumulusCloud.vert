/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "RenderSky.h"

struct GsIn {
	float4 OffsetScale : Position;
};

struct PsIn {
	float4 position : SV_Position;
	float4 texCoord	: TexCoord0;
//	TODO: test for spherical billboards
	float3 spherical: TexCoord1;
};


static const int MaxParticles = 100;
static const int QMaxParticles = (MaxParticles+3)/4;


cbuffer CumulusUniform : register(b3)
{
	float4x4 model;
	float4	OffsetScale[MaxParticles];	
	float4	ParticleProps[QMaxParticles];

	float4x4 vp;
	float4x4 v;
	float3 dx, dy;
	float zNear;
  float2 packDepthParams;
  float masterParticleRotation;
};

GsIn main(uint VertexID : SV_VertexID)
{
	GsIn Out;
	Out.OffsetScale = OffsetScale[VertexID];
	Out.OffsetScale.xyz = mul( model, float4(Out.OffsetScale.xyz, 1.0f));
	return Out;
}