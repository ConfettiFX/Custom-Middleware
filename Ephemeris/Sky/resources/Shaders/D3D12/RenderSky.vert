/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "RenderSky.h"

RWStructuredBuffer<float4> TransmittanceColor : register(u0);

struct PsIn {
	float4 position: SV_Position;
	float3 texCoord: TexCoord;
	float2 ScreenCoord: TexCoord1;
};

PsIn main(uint VertexID: SV_VertexID){
	PsIn Out;
	
	// Produce a fullscreen triangle
	float4 position;
	position.x = (VertexID == 0)? 3.0 : -1.0;
	position.y = (VertexID == 2)? 3.0 : -1.0;
	position.zw = 1.0;
	Out.position = position;
	
	//	Unproject position
	position.z = 0.0;
	position.w = QNNear.z;
	position.xy *= position.w;

	Out.texCoord = mul(invProj, position).xyz;
	Out.texCoord /= Out.texCoord.z;

	Out.texCoord = mul((float3x3)invView, Out.texCoord);

	Out.ScreenCoord = Out.position.xy * float2(0.5, -0.5) + 0.5;

	float3 ray = LightDirection.xyz;
	float3 x = CameraPosition.xyz;
	float3 v = normalize(ray);

	float r = length(x);
	float mu = dot(x, v) / r;

  float4 dayLight = float4(transmittance(r, mu), 1.0f);
  float4 nightLight = float4(LightIntensity) * 0.15f;

	TransmittanceColor[VertexID] = LightDirection.y >= 0.2f ? dayLight : lerp(nightLight, dayLight, saturate(LightDirection.y / 0.2f));

	return Out;
}