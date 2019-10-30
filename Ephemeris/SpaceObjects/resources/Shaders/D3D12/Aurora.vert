/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

struct AuroraParticle
{
	float4 PrevPosition;			// PrePosition and movable flag
	float4 Position;				// Position and mass
	float4 Acceleration;
};

RWStructuredBuffer<AuroraParticle> AuroraParticleBuffer		: register(u1);

cbuffer AuroraUniformBuffer : register(b4, UPDATE_FREQ_PER_FRAME)
{
	uint        maxVertex;
	float       heightOffset;
	float       height;
	float       deltaTime;

	float4x4    ViewProjMat;
};

struct VSOutput
{
	float3 Position       : Position;
	float3 Color          : Color;
	float3 NextPosition   : TexCoord;
};

VSOutput main(uint VertexID : SV_VertexID)
{	
	VSOutput result;

  result.Color = float3(0.0, 0.0, 0.0);

  result.Position = AuroraParticleBuffer[VertexID].Position.xyz;
  result.NextPosition =  result.Position;

  if( VertexID < (maxVertex - 1) )
  {
    result.NextPosition =  AuroraParticleBuffer[VertexID + 1].Position.xyz;
  }
  
  return result;
}