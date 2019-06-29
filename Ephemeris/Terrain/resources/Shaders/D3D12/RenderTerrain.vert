/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

struct VsIn {
  float3 position : Position;
  float2 texcoord : TexCoord;
};

struct PsIn {
  float4 position : SV_Position;
  sample float3 positionWS : PositionWS;
  float2 texcoord : TexCoord;
  float3 normal : Normal;
  float3 tangent : Tangent;
  float3 bitangent : Bitangent;
};

cbuffer RenderTerrainUniformBuffer : register(b1)
{
	float4x4 projView;
	float4	TerrainInfo;
	float4  CameraInfo;
}

PsIn main(VsIn In)
{
  PsIn Out;

  Out.position = mul(projView, float4(In.position, 1.0));
  Out.positionWS = In.position;
  Out.texcoord = In.texcoord;

  float3 f3Normal = normalize(In.position - float3(0, -TerrainInfo.x, 0));
  Out.normal = f3Normal;
  Out.tangent = normalize( cross(f3Normal, float3(0,0,1)) );
  Out.bitangent = normalize( cross(Out.tangent, f3Normal) );
  return Out;
}