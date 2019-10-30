/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer StarUniform : register(b2, UPDATE_FREQ_PER_FRAME)
{
	float4x4 RotMat;
	float4x4 ViewProjMat;
	float4 LightDirection;
	float4 Dx;
	float4 Dy;
};

struct GsIn {
  float3 Position   : Position;
  float4 Color      : Color;
  float4 StarInfo   : TexCoord;
};

struct PsIn {
	float4 position     : SV_Position;
	float3 texCoord	    : TexCoord0;
	float2 screenCoord  : TexCoord1;
	float3 color        : TexCoord2;
};

void PushVertex(inout TriangleStream<PsIn> Stream, float3 pos, float3 color, float3 dx, float3 dy, float2 vOffset, float2 baseTC)
{
	PsIn Out;

	// Output a billboarded quad	
  float4 position = mul(RotMat, float4(pos.x, pos.y, pos.z, 1.0));
  Out.position = mul(ViewProjMat, float4(position + dx * vOffset.x + dy * vOffset.y, 1.0));

  Out.texCoord.z = Out.position.z;

	Out.position /= Out.position.w;

	Out.screenCoord = float2((Out.position.x + 1.0) * 0.5, (1.0 - Out.position.y) * 0.5);

	Out.texCoord.xy = baseTC + vOffset;
   Out.color = color;
  
	Stream.Append(Out);
}

[maxvertexcount(4)]
void main(point GsIn In[1], uint VertexID : SV_PrimitiveID, inout TriangleStream<PsIn> Stream)
{
	PsIn Out;

	float2 baseTC = float2(0,0);

  float4 starInfo = In[0].StarInfo;
  float particleSize  = starInfo.y;
  float3 Width        = Dx.xyz * particleSize;
  float3 Height       = Dy.xyz * particleSize;

  float3 Color = In[0].Color.rgb * In[0].Color.a;
  float time = LightDirection.a;
  float timeSeed = starInfo.z;
  float timeScale = starInfo.a;

  float blink = (sin(timeSeed + time * timeScale ) + 1.0f) * 0.5f;
  Color *= blink;

	PushVertex(Stream, In[0].Position.xyz, Color, Width, Height, float2(-1,-1), baseTC);
	PushVertex(Stream, In[0].Position.xyz, Color, Width, Height, float2( 1,-1), baseTC);
	PushVertex(Stream, In[0].Position.xyz, Color, Width, Height, float2(-1, 1), baseTC);
	PushVertex(Stream, In[0].Position.xyz, Color, Width, Height, float2( 1, 1), baseTC);

	Stream.RestartStrip();
}