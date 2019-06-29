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

static const float distanceMul = 0.001;

static const int MaxParticles = 100;
static const int QMaxParticles = (MaxParticles+3)/4;


#ifdef	CLAMP_IMPOSTOR_PROJ
float4		ClampWindow;
#else	//	CLAMP_IMPOSTOR_PROJ
static const float4 ClampWindow = float4(-1.0f, 1.0f, -1.0f, 1.0f);
#endif	//	CLAMP_IMPOSTOR_PROJ


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

PsIn main(uint VertexID: SV_VertexID){
	PsIn Out;

	// Produce a fullscreen triangle
	float4 position;
	position.x = (VertexID & 1) ? ClampWindow.y : ClampWindow.x;
	position.y = 0.0;
	position.z = (VertexID & 2) ? ClampWindow.w : ClampWindow.z;
	position.w = 1.0;

	#ifdef	DEBUG_GRID
	Out.DebugTC	= position.xz * float2(0.5, 0.5) + 0.5;
	Out.DebugTC.y = 1.0f-Out.DebugTC.y;
	#endif	//	DEBUG_GRID
	
	Out.TC.x = (VertexID & 1) ? 1.0f : -1.0f;
	Out.TC.y = (VertexID & 2) ? 1.0f : -1.0f;

	Out.TC = Out.TC * float2(0.5, 0.5) + 0.5;
	//Out.TC = position.xy * float2(0.5, 0.5) + 0.5;
	Out.position = mul(mvp, position);
	Out.WPos = mul(model, position);
	Out.WDir = (Out.WPos*offsetScale.w + offsetScale.xyz)-CameraPosition.xyz;
	//Out.WDir = Out.WPos-CameraPosition.xyz;
	Out.WPos = CameraPosition.xyz;
	
	Out.ScreenCoord.x = (Out.position.x + Out.position.w) * 0.5 ;
	Out.ScreenCoord.y = (-Out.position.y + Out.position.w) * 0.5 ;
	Out.ScreenCoord.z = Out.position.w;
	
//	Out.WPos += float3(167, 532, -788);
//	Out.WPos *= distanceMul;
	//	Apply Earth radius offset
//	Out.WPos += float3(0,Rg+0.001,0);
//	Out.WDir *= distanceMul;
	
	//	Allow clouds to be rendered even if they are too far
	if (Out.position.z > Out.position.w)
		Out.position.z = Out.position.w;

	return Out;
}