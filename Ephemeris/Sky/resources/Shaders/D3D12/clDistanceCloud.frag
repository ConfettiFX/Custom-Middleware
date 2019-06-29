/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "RenderSky.h"

struct PsIn {
	float4	position	: POSITION;
	float2 	TC			: TexCoord0;
	float3	ScreenCoord	: TexCoord1;
	float3	WPos		: TexCoord2;
	float3	WDir		: TexCoord3;
};

cbuffer DistanceUniform : register(b4)
{
	float4x4	mvp;
	float4x4	model;
	float4		offsetScale;
	float4	  localSun;

	float StepSize;
	float Attenuation;
	float AlphaSaturation;
  float padding00;
};

Texture2D <float4> base;
Texture2D <float> SunShafts;

SamplerState linearClamp;

float4 main(PsIn In) : SV_Target {

	//return float4(1,0,0,1);
	//return base.Sample(linearClamp, In.TC);

	const int c_numSamples = 8;
	
	float2 sampleDir = localSun.xz * StepSize;
	//float2 sampleDir = localSun.xy * StepSize;
	float2 uv = In.TC.xy;
	
	float opacity = base.Sample (linearClamp, In.TC).x;
	float density = 0;
	
	//	Igor:	early-out is used to render the cloud faster
	//	Still, early out could be moved here.
	//	Need to profile carefully the real-world application
	//	to determin if early out here and gradient sampling is 
	//	better than the current solution. 
	for( int i = 0; i < c_numSamples; i++ )
	{
		float t = base.Sample (linearClamp, In.TC+ i * sampleDir).x;
		density += t;
	}
	
	float c = exp2( -Attenuation * density );
	float a = pow( opacity, AlphaSaturation );
	
	if (a<0.5/256.0) return float4(0.0, 0.0, 0.0, 0.0);
	
	float shaftsMask = SunShafts.SampleLevel(linearClamp, In.ScreenCoord.xy/In.ScreenCoord.z, 0);

	float3 ray = In.WDir;
    float3 x = In.WPos;
    float3 v = normalize(ray);

    float r = length(x);
    float mu = dot(x, v) / r;
    float t = dot(v,ray);
    
#ifdef	ADVANCED_SHAFTS
	//float smartFactor = smoothstep(100,200,t);
	//t *= lerp(1, InScatterParams.x + InScatterParams.y*shaftsMask, smartFactor);
	t *= InScatterParams.x + InScatterParams.y*shaftsMask;
#endif	//	ADVANCED_SHAFTS
    
    float vdots = saturate(3*dot(v,LightDirection.xyz)-2);

	//	Add back lighting    
	//c = c*pow(vdots,5)+c;

	//	Calculate front lighting
	//c = lerp(c, 1, pow(saturate(-dot(v,s)),2) );
	
	//	Igor: the cloud is transparent to the sun
    c += pow(vdots,5);

    float3 attenuation;
	float3 inscatterColor = inscatter(x, t, v, LightDirection.xyz, r, mu, attenuation); //S[L]-T(x,xs)S[l]|xs
    float3 _groundColor = groundColor(x, t, v, LightDirection.xyz, r, mu, attenuation, float4((c).xxx, 1), true); //R[L0]+R[L*]
    float3 _sunColor = sunColor(x, t, v, LightDirection.xyz, r, mu); //L0
    
#ifdef	ADVANCED_SHAFTS
	//inscatterColor *= lerp(InScatterParams.x + InScatterParams.y*shaftsMask, 1, smartFactor);
#else	//	ADVANCED_SHAFTS
	inscatterColor *= InScatterParams.x + InScatterParams.y*shaftsMask;
#endif	//	ADVANCED_SHAFTS
	
	return float4( HDR(_sunColor + _groundColor + inscatterColor), a);
}