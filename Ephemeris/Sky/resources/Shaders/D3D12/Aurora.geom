/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer AuroraUniformBuffer : register(b4)
{
	uint        maxVertex;
	float       heightOffset;
	float       height;
	float       deltaTime;

	float4x4    ViewProjMat;
};

struct GsIn {
  float3 Position       : Position;
  float3 Color          : Color;
  float3 NextPosition   : TexCoord;
};

struct PsIn {
	float4 position     : SV_Position;
	float2 texCoord	    : TexCoord0;
  float3 color        : TexCoord1;
};

void PushVertex(inout TriangleStream<PsIn> Stream, float3 pos, float3 color, float2 UV)
{
	PsIn Out;

	// Output a billboarded quad  
  Out.position = mul(ViewProjMat, float4(pos, 1.0));
  Out.texCoord.xy = UV;
  Out.color = color;
  
	Stream.Append(Out);
}

[maxvertexcount(4)]
void main(point GsIn In[1], uint VertexID : SV_PrimitiveID, inout TriangleStream<PsIn> Stream)
{
	PsIn Out;

  PushVertex(Stream, In[0].Position.xyz + float3(0.0, heightOffset, 0.0)              , In[0].Color, float2(0.0, 1.0));
	PushVertex(Stream, In[0].Position.xyz + float3(0.0, heightOffset + height, 0.0)     , In[0].Color, float2(0.0, 0.0));
	PushVertex(Stream, In[0].NextPosition.xyz + float3(0.0, heightOffset, 0.0)          , In[0].Color, float2(1.0, 1.0));
	PushVertex(Stream, In[0].NextPosition.xyz + float3(0.0, heightOffset + height, 0.0) , In[0].Color, float2(1.0, 0.0));

	Stream.RestartStrip();
}