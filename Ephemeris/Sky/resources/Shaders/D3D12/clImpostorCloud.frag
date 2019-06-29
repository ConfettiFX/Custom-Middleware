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
	float4	position	: SV_Position;
	float2 	TC			: TexCoord0;
	float3	ScreenCoord	: TexCoord1;
	float3	WPos		: TexCoord2;
	float3	WDir		: TexCoord3;
};

static const int MaxParticles = 100;
static const int QMaxParticles = (MaxParticles+3)/4;

cbuffer ImposterUniform : register(b3)
{
	float4x4	mvp;
	float4x4	model;
	float4		offsetScale;

	float3	localSun;
	float StepSize;

	float Attenuation;
	float AlphaSaturation;

	float2 UnpackDepthParams;

	float CloudOpacity;
	float SaturationFactor;
	float ContrastFactor;
  float padding00;
};

Texture2D <float4> base;
Texture2D <float> SunShafts;

#ifdef	SKYDOME_SOFT_CLOUDS
Texture2D<float>	Depth;
float3 camDir;
#endif	//	SKYDOME_SOFT_CLOUDS

SamplerState linearClamp;

float4 main(PsIn In) : SV_Target {

	//return float4(1,0,0,1);
	//return base.Sample(linearClamp, In.TC);
	//return float4(1-base.Sample(linearClamp, In.TC).xxx, 1);
	//return base.Sample(linearClamp, In.TC).xxxw;
	if (0){
		float4 res = base.Sample(linearClamp, In.TC);
		if (res.a<0.5/256.0) return float4(0.0, 0.0, 0.0, 0.0);
		return float4(1-res.xxx, 1);
	}
	if (0){
		float4 res = base.Sample(linearClamp, In.TC);
		if (res.a<0.5/256.0) return float4(0.0, 0.0, 0.0, 0.0);
		return float4((res.x/res.y).xxx, 1);
	}

	const int c_numSamples = 8;
	
	float2 sampleDir = localSun.xz * StepSize;
	//float2 sampleDir = localSun.xy * StepSize;
	float2 uv = In.TC.xy;
	
	float4	ImpostorData = base.Sample (linearClamp, In.TC);
#ifdef	INVERSE_ALPHA
	ImpostorData.a = 1.0 - ImpostorData.a;
#endif	//	INVERSE_ALPHA
	float opacity = ImpostorData.a*CloudOpacity;
	float density = 0;
	
	//	Igor:	early-out is used to render the cloud faster
	//	Still, early out could be moved here.
	//	Need to profile carefully the real-world application
	//	to determin if early out here and gradient sampling is 
	//	better than the current solution. 
	for( int i = 0; i < c_numSamples; i++ )
	{
		float t = base.Sample (linearClamp, In.TC+ i * sampleDir).a;
#ifdef	INVERSE_ALPHA
		t = 1.0 - t;
#endif	//	INVERSE_ALPHA
		density += t;
	}
	
	float c = exp2( -Attenuation * density );
	float a = pow( opacity, AlphaSaturation );
	
	//	TODO: Igor: re-enable this
	//if (a<0.5/256.0) return float4(0.0, 0.0, 0.0, 0.0);
	
	float shaftsMask = SunShafts.SampleLevel(linearClamp, In.ScreenCoord.xy/In.ScreenCoord.z, 0);

	//	TODO: remove inversion when clear to 1
	//float packedDepth = ImpostorData.x;
	//float packedDepth = 1-ImpostorData.x;
	float packedDepth = ImpostorData.x/ImpostorData.y;
	//float unpackedDepth = packedDepth * UnpackDepthParams.x + UnpackDepthParams.y;
	float unpackedDepth = packedDepth * UnpackDepthParams.x + UnpackDepthParams.y;
	//unpackedDepth /= In.ScreenCoord.z;
	//unpackedDepth /= In.WDir.z;

	//return float4((packedDepth).xxx, 1);
	//return float4((unpackedDepth-1).xxx, 1);

	float3 ray = In.WDir*unpackedDepth;
    float3 x = In.WPos;
    float3 v = normalize(In.WDir);

    float r = length(x);
    float mu = dot(x, v) / r;
    float t = dot(v,ray);

#ifdef	SKYDOME_SOFT_CLOUDS
	int3 LoadCoord = int3((int2) In.position, 0);
	float depth = Depth.Load(LoadCoord).x;
	float fixedDepth = (-QNNear.y * QNNear.x) / (depth - QNNear.x);
	float viewSpaceUnpackedDapth = dot(ray, camDir);
	float depthClip = smoothstep(fixedDepth, fixedDepth-0.3, viewSpaceUnpackedDapth);

	opacity *= depthClip;
	a = pow( opacity, AlphaSaturation );

//	return float4(1,1,1,depthClip);
#endif	//	SKYDOME_SOFT_CLOUDS
    
#ifdef	ADVANCED_SHAFTS
	//float smartFactor = smoothstep(100,200,t);
	//t *= lerp(1, InScatterParams.x + InScatterParams.y*shaftsMask, smartFactor);
	t *= InScatterParams.x + InScatterParams.y*shaftsMask;
#endif	//	ADVANCED_SHAFTS
    
    float vdots = saturate(3*dot(v, LightDirection.xyz)-2);

	//	Add back lighting    
	//c = c*pow(vdots,5)+c;
	c = (1-a)*pow(vdots,5)+c;

	//	Calculate front lighting
	//c = lerp(c, 1, pow(saturate(-dot(v,s)),2) );
	
	//	Igor: the cloud is transparent to the sun
    //c += pow(vdots,5);

    float3 attenuation;
	float3 inscatterColor = inscatter(x, t, v, LightDirection.xyz, r, mu, attenuation); //S[L]-T(x,xs)S[l]|xs
    float3 _groundColor = groundColor(x, t, v, LightDirection.xyz, r, mu, attenuation, float4((c).xxx, 1), true); //R[L0]+R[L*]
    float3 _sunColor = sunColor(x, t, v, LightDirection.xyz, r, mu); //L0
    
#ifdef	ADVANCED_SHAFTS
	//inscatterColor *= lerp(InScatterParams.x + InScatterParams.y*shaftsMask, 1, smartFactor);
#else	//	ADVANCED_SHAFTS
	inscatterColor *= InScatterParams.x + InScatterParams.y*shaftsMask;
#endif	//	ADVANCED_SHAFTS
	
//	float3 res = HDR(_sunColor + dot(_groundColor, 0.3).xxx + inscatterColor);
	float3 res = HDR(_sunColor + _groundColor + inscatterColor);

	//	Use a separate variable. It seems that otherwise shader compiler optimises some code out which is a compiler bug?
    float3 resA = ColorSaturate(res, SaturationFactor);
	res = ColorContrast(resA, ContrastFactor);

	//	TODO: Igor: remove this. Debug grid.
#ifdef	DEBUG_GRID
	{
		if (a<0.5/256.0) res = float3(0.0, 0.0, 0.0);

		{
			const float numLines = 1.0f;
			float2 gridData = frac(In.TC*numLines);

			//float LineFallOff = 2.0f;
			float LineFallOff = 5.0f;
			float2 GridDensity = saturate(gridData*LineFallOff)*saturate((1.0-gridData)*LineFallOff);

			float GridDensityScalar = min(GridDensity.x,GridDensity.y);
			if (GridDensityScalar>0.1) GridDensityScalar = 1.0f;

			GridDensityScalar = saturate(1 - pow(GridDensityScalar,0.5));

			
			a = max(a, GridDensityScalar);
			res = lerp(res, float3(0.0f, 1.0f, 0.0f), GridDensityScalar);
		}

		{
			const float numLines = 10.0f;
			float2 gridData = frac(In.DebugTC*numLines);

			//float LineFallOff = 2.0f;
			float LineFallOff = 5.0f;
			float2 GridDensity = saturate(gridData*LineFallOff)*saturate((1.0-gridData)*LineFallOff);

			float GridDensityScalar = min(GridDensity.x,GridDensity.y);
			if (GridDensityScalar>0.1) GridDensityScalar = 1.0f;

			GridDensityScalar = saturate(1 - pow(GridDensityScalar,0.5));

			
			a = max(a, GridDensityScalar);
			res = lerp(res, float3(1.0f, 0.0f, 0.0f), GridDensityScalar);
		}

		{
			const float numLines = 1.0f;
			float2 gridData = frac(In.DebugTC*numLines);

			//float LineFallOff = 2.0f;
			float LineFallOff = 10.0f;
			float2 GridDensity = saturate(gridData*LineFallOff)*saturate((1.0-gridData)*LineFallOff);

			float GridDensityScalar = min(GridDensity.x,GridDensity.y);
			if (GridDensityScalar>0.1) GridDensityScalar = 1.0f;

			GridDensityScalar = saturate(1 - pow(GridDensityScalar,0.5));

			
			a = max(a, GridDensityScalar);
			res = lerp(res, float3(1.0f, 0.0f, 0.0f), GridDensityScalar);
		}

	}
#endif	//	DEBUG_GRID

#ifdef	SKYDOME_SOFT_CLOUDS
	//a *= depthClip;
#endif	//	SKYDOME_SOFT_CLOUDS	

	return float4( saturate(res), a);
}