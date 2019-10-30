/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer RenderTerrainUniformBuffer : register(b1, UPDATE_FREQ_PER_FRAME)
{
	float4x4 projView;
	float4	TerrainInfo; //x: y: usePregeneratedNormalmap
	float4  CameraInfo;
}


Texture2D NormalMap           : register(t0);
Texture2D MaskMap             : register(t1);

Texture2D tileTextures[5]     : register(t2);
Texture2D tileTexturesNrm[5]  : register(t7);

Texture2D shadowMap           : register(t12);

SamplerState g_LinearMirror : register(s0);
SamplerState g_LinearWrap   : register(s1);
SamplerState g_LinearBorder : register(s2);

struct PsIn {
  float4 position : SV_Position;
  sample float3 positionWS : PositionWS;
  float2 texcoord : TexCoord;
  float3 normal : Normal;
  float3 tangent : Tangent;
  float3 bitangent : Bitangent;
};

struct PsOut {
  float4 albedo : SV_Target0;
  float4 normal : SV_Target1;
};

float DepthLinearization(float depth, float near, float far)
{
	return (2.0 * near) / (far + near - depth * (far - near));
}

float3 ReconstructNormal(in float4 sampleNormal, float intensity)
{
	float3 tangentNormal;
	tangentNormal.xy = (sampleNormal.rg * 2 - 1) * intensity;
	tangentNormal.z = sqrt(1 - saturate(dot(tangentNormal.xy, tangentNormal.xy)));
	return tangentNormal;
}

float MipLevel( float2 uv )
{
  float TextureSize = 2048.0f;

  float2 dx = ddx( uv * TextureSize );
  float2 dy = ddy( uv * TextureSize );
  float d = max( dot( dx, dx ), dot( dy, dy ) );

  uint MipCount = uint(log2(TextureSize)) + 1;
  
  // Clamp the value to the max mip level counts
  float rangeClamp = pow(2, MipCount - 1);
  d = clamp(sqrt(d), 1.0, rangeClamp);
      
  float mipLevel = 0.75f * log2(d);
  return mipLevel;
}

PsOut main(PsIn In)
{
  PsOut Out;
    
  float linearDepth = DepthLinearization(In.position.z, CameraInfo.x, CameraInfo.y);

  float dist = distance(CameraInfo.xyz, In.positionWS);
  dist = dist / CameraInfo.w; 

  float4 maskVal = MaskMap.SampleLevel(g_LinearMirror, In.texcoord, 0.0);
  //maskVal /= max( dot(maskVal, float4(1,1,1,1)) , 1 );
  float baseWeight = saturate(1 - dot(maskVal, float4(1,1,1,1)));
  float baseTileScale = 70;
  float4 tileScale = float4(100,120,80,80);

  float3 result = float3(0,0,0);
  float3 surfaceNrm = (float3)0;
  [unroll]
  for(uint i = 0; i < 4; ++i)
  {
    //const float fThresholdWeight = 3.f/256.f;
    float2 uv = In.texcoord * tileScale[i];
    float lod = MipLevel(uv);

    result += tileTextures[i].SampleLevel(g_LinearWrap, uv, lod).xyz * maskVal[i];
    surfaceNrm += ReconstructNormal(tileTexturesNrm[i].SampleLevel(g_LinearWrap, uv, lod), 1.0f) * maskVal[i];
  }

  float2 uv = In.texcoord * baseTileScale;
  float lod = MipLevel(uv);
  float3 baseColor = tileTextures[4].SampleLevel(g_LinearWrap, uv, lod).xyz * baseWeight;
  result += baseColor;
  surfaceNrm += ReconstructNormal(tileTexturesNrm[4].SampleLevel(g_LinearWrap, uv, lod), 1.0f) * baseWeight;

  float3 EarthNormal = normalize(In.normal);
  float3 EarthTangent = normalize(In.tangent);
  float3 EarthBitangent = normalize(In.bitangent);
  
  float3 f3TerrainNormal;

  if (TerrainInfo.y > 0.5f)
  {   
	  f3TerrainNormal.xzy = NormalMap.SampleLevel(g_LinearMirror, In.texcoord.xy, 0.0).xyz * 2 - 1;
	  // Since UVs are mirrored, we have to adjust normal coords accordingly:
	  float2 f2XZSign = sign(0.5 - frac(In.texcoord.xy / 2));
	  f3TerrainNormal.xz *= f2XZSign * 0.1;
	  f3TerrainNormal.y /= TerrainInfo.z;
  }
  else
  {
	  f3TerrainNormal.xz = NormalMap.SampleLevel(g_LinearMirror, In.texcoord.xy, 0.0).xy;
	  // Since UVs are mirrored, we have to adjust normal coords accordingly:
	  float2 f2XZSign = sign(0.5 - frac(In.texcoord.xy / 2));
	  f3TerrainNormal.xz *= f2XZSign;

	  f3TerrainNormal.y = sqrt(saturate(1 - dot(f3TerrainNormal.xz, f3TerrainNormal.xz))) / TerrainInfo.z;
  }

  f3TerrainNormal = normalize( mul(f3TerrainNormal, float3x3(EarthTangent, EarthNormal, -EarthBitangent)) );
  float3 f3TerrainTangent, f3TerrainBitangent;
  f3TerrainTangent = normalize( cross(f3TerrainNormal, float3(0,0,1)) );
  f3TerrainBitangent = normalize( cross(f3TerrainTangent, f3TerrainNormal) );

  f3TerrainNormal = normalize( mul(surfaceNrm.xzy, float3x3(f3TerrainTangent, f3TerrainNormal, -f3TerrainBitangent)) );
  
  Out.albedo = float4(result, 1.0);
  Out.normal = float4(f3TerrainNormal, 1);
  
  return Out;
}