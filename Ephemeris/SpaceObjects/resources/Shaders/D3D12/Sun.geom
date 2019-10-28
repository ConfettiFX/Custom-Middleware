/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer SunUniform : register(b3, UPDATE_FREQ_PER_FRAME)
{
	float4x4 ViewMat;
	float4x4 ViewProjMat;
	float4 LightDirection;
	float4 Dx;
	float4 Dy;
};

struct GsIn {
	float4 OffsetScale : Position;
};

struct PsIn {
	float4 position : SV_Position;
	float3 texCoord	: TexCoord0;
	float2 screenCoord : TexCoord1;
};

void PushVertex(inout TriangleStream<PsIn> Stream, float3 pos, 
				float3 dx, float3 dy, float2 vOffset, float2 baseTC)
{
	PsIn Out;

	// Output a billboarded quad
	Out.position = mul(ViewProjMat, float4(pos + dx*vOffset.x + dy*vOffset.y, 1.0));

  Out.texCoord.z = Out.position.z;

	Out.position.z = Out.position.w;
	Out.position /= Out.position.w;

	Out.screenCoord = float2((Out.position.x + 1.0) * 0.5, (1.0 - Out.position.y) * 0.5);

	vOffset+=float2(1,-1);
	vOffset*=float2(0.5,-0.5);
	Out.texCoord.xy = baseTC + vOffset;
  
	Stream.Append(Out);
}

[maxvertexcount(4)]
void main(point GsIn In[1], uint VertexID : SV_PrimitiveID, inout TriangleStream<PsIn> Stream)
{
	PsIn Out;

	float2 baseTC = float2(0,0);

	PushVertex(Stream, In[0].OffsetScale.xyz,  Dx.xyz, Dy.xyz, float2(-1,-1), baseTC);
	PushVertex(Stream, In[0].OffsetScale.xyz,  Dx.xyz, Dy.xyz, float2(1,-1), baseTC);
	PushVertex(Stream, In[0].OffsetScale.xyz,  Dx.xyz, Dy.xyz, float2(-1,1), baseTC);
	PushVertex(Stream, In[0].OffsetScale.xyz,  Dx.xyz, Dy.xyz, float2(1,1), baseTC);

	Stream.RestartStrip();
}