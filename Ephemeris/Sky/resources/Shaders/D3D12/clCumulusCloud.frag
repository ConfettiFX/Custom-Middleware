/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

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
	//int4	TexIDs[QMaxParticles];
	float4	ParticleProps[QMaxParticles];

	float4x4 vp;
	float4x4 v;
	float3 dx, dy;
	float zNear;
};

Texture2D <float4> base;

SamplerState linearClamp;

float4 main(PsIn In) : SV_Target {
	float4 res = base.Sample(linearClamp, In.texCoord.xy);

	float depth = In.texCoord.w;

	//	TODO: store depth offset in a texture!
	depth -= In.spherical.z*sqrt(saturate(1-dot(In.spherical.xy, In.spherical.xy)));

	//	TODO: remove inversion when clear to 1
	//	Simple depth overwrite
	{
		res.rgb = 1-depth;
		res.a *= In.texCoord.z;
	}
	if (1)
	//	Weigthed depth blend
	{
		res.rgb = 1-depth;
		res.a *= In.texCoord.z;
		res.r = max(depth,0);
		res.g = 1;
	}

	//res *= 0.3;

	clip(res.a-1.0/255.0);

	//res.a = 1;

	return res;
}