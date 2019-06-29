/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

struct GsIn {
	float4 OffsetScale : Position;
};

static const int MaxParticles = 100;
static const int QMaxParticles = (MaxParticles+3)/4;


cbuffer CumulusUniform : register(b3)
{
	float4x4 model;
	float4	OffsetScale[MaxParticles];	
	float4	ParticleProps[QMaxParticles];

	float4x4 vp;
	float4x4 v;
	float3 dx, dy;
	float zNear;
  float2 packDepthParams;
  float masterParticleRotation;
};

struct PsIn {
	float4 position : SV_Position;
	float4 texCoord	: TexCoord0;
//	TODO: test for spherical billboards
	float3 spherical: TexCoord1;
};

[maxvertexcount(4)]
void main(point GsIn In[1], uint VertexID : SV_PrimitiveID, inout TriangleStream<PsIn> Stream)
{
	PsIn Out;

	// This controls how the clouds fade-out as the camera clips them.  Smaller values
	// fade out more aggressively: good for large frustum setups.
	const float zScale = 0.001;
	//const float zScale = 0.00001;

	float s,c;
	sincos(masterParticleRotation+ParticleProps[VertexID].y, s, c );

	s *= In[0].OffsetScale.w;
	c *= In[0].OffsetScale.w;

	float3 dxScaled = (dx*c - dy*s);
	float3 dyScaled = (dx*s + dy*c);
	//float3 dxScaled = dx;
	//float3 dyScaled = dy;

	Out.texCoord.z = mul(v, float4(In[0].OffsetScale.xyz, 1.0)).z;
	Out.texCoord.w = Out.texCoord.z * packDepthParams.x + packDepthParams.y;
	Out.texCoord.z = saturate( (Out.texCoord.z - zNear)*zScale );
	Out.texCoord.z *= saturate(ParticleProps[VertexID].z);
	//Out.texCoord.z *= In[0].OffsetScale.w;

	//	TODO: replace it with real particle R passed from C++ code
	//	TODO: encode particle R into spherical.xy instead
	Out.spherical.z = length(dxScaled);
	Out.spherical.z = Out.spherical.z * packDepthParams.x;
        
	//int texID = TexIDs[VertexID>>2][VertexID&3];
	//int texID = ParticleProps[svPrimitiveID.x].x;
	//float2 baseTC = float2((texID&3)*0.25, (texID>>2)*0.25);
	float texID = ParticleProps[VertexID].x;
	float2 baseTC = float2( frac(texID*0.25), (floor(texID*0.25))*0.25 );
	//float texIDY = floor(texID*0.25f);
	//float texIDY = 0;
	//float2 baseTC = float2( texIDY*0.25f, frac(texID*0.25));

	// Output a billboarded quad
	Out.position = mul(vp, float4(In[0].OffsetScale.xyz - dxScaled - dyScaled, 1.0));
	Out.texCoord.xy = baseTC;//float2(0, 0);
	Out.spherical.xy = float2(-1, -1);
	Stream.Append(Out);

	Out.position = mul(vp, float4(In[0].OffsetScale.xyz + dxScaled - dyScaled, 1.0));
	Out.texCoord.xy = baseTC+float2(0.25, 0);
	Out.spherical.xy = float2(1, -1);
	Stream.Append(Out);

	Out.position = mul(vp, float4(In[0].OffsetScale.xyz - dxScaled + dyScaled, 1.0));
	Out.texCoord.xy = baseTC+float2(0, 0.25);
	Out.spherical.xy = float2(-1, 1);
	Stream.Append(Out);

	Out.position = mul(vp, float4(In[0].OffsetScale.xyz + dxScaled + dyScaled, 1.0));
	Out.texCoord.xy = baseTC+float2(0.25, 0.25);
	Out.spherical.xy = float2(1, 1);
	Stream.Append(Out);
	                
	Stream.RestartStrip();
}
