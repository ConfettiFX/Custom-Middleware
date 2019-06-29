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



static const float distanceMul = 0.001;

PsIn main(uint VertexID: SV_VertexID){
	PsIn Out;

	// Produce a fullscreen triangle
	float4 position;
	position.x = (VertexID & 1) ? 1.0 : -1.0;
	position.y = 0.0;
	position.z = (VertexID & 2) ? 1.0 : -1.0;
	//position.y = (VertexID & 2) ? 1.0 : -1.0;
	//position.z = 0.0;
	position.w = 1.0;
	
	Out.TC = position.xz * float2(0.5, 0.5) + 0.5;
	//Out.TC = position.xy * float2(0.5, 0.5) + 0.5;
	Out.position = mul(mvp, position);
	Out.WPos = mul(model, position).xyz;
	Out.WDir = (Out.WPos*offsetScale.w + offsetScale.xyz) - CameraPosition.xyz;
	//Out.WDir = Out.WPos - CameraPosition.xyz;
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