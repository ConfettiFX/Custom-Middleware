/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer LightingTerrainUniformBuffer : register(b2)
{
	float4x4	InvViewProjMat;
	float4x4	ShadowViewProjMat;
	float4	ShadowSpheres;
	float4	LightDirection;
	float4	SunColor;
  float4	LightColor;
}

struct VolumetricCloudsShadowCB
{
  float4	SettingInfo00;			// x : EnableCastShadow, y : CloudCoverage, z : WeatherTextureTiling, w : Time
  float4	StandardPosition;		// xyz : The current center location for applying wind, w : ShadowBrightness
  float4  ShadowInfo;
};

cbuffer VolumetricCloudsShadowCB : register(b3)
{
  // SettingInfo; // x : EnableCastShadow, y : CloudCoverage, z : WeatherTextureTiling, w : Time
  VolumetricCloudsShadowCB g_VolumetricCloudsShadow;
};

Texture2D BasicTexture      : register(t14);
Texture2D NormalTexture     : register(t15);
Texture2D weatherTexture    : register(t16);
Texture2D depthTexture      : register(t17);

SamplerState g_LinearMirror : register(s0);
SamplerState g_LinearWrap   : register(s1);


//Code from https://area.autodesk.com/blogs/game-dev-blog/volumetric-clouds/.
bool ray_trace_sphere(float3 center, float3 rd, float3 offset, float radius2, out float t1, out float t2) {
  float3 p = center - offset;
  float b = dot(p, rd);
  float c = dot(p, p) - radius2;

  float f = b * b - c;

  if (f >= 0.0)
  {
    float sqrtF = sqrt(f);
    t1 = -b - sqrtF;
    t2 = -b + sqrtF;

    if (t2 <= 0.0 && t1 <= 0.0)
      return false;

    return true;
  }
  else
  {
    t1 = 0.0;
    t2 = 0.0;
    return false;
  }
}

#define USE_PROJECTED_SHADOW 1
#define _EARTH_RADIUS_ADD_CLOUDS_LAYER_START	5015000.0			// EARTH_RADIUS + _CLOUDS_LAYER_START
#define _EARTH_RADIUS				5000000.0						// Currently, follow the g_EarthRadius in Sky.cpp
#define _EARTH_CENTER float3(0, -_EARTH_RADIUS, 0)					// Center position of the Earth

float EvaluateCloudShadow(in float3 ws_pos, in float3 lightDir)
{
#if USE_PROJECTED_SHADOW
  float it1, it2;
  ray_trace_sphere(ws_pos.xyz, lightDir, _EARTH_CENTER, _EARTH_RADIUS_ADD_CLOUDS_LAYER_START, it1, it2);
  float3 CloudPos = ws_pos.xyz + lightDir * it2;

  float3 weatherData = weatherTexture.SampleLevel(g_LinearWrap, (CloudPos.xz + g_VolumetricCloudsShadow.StandardPosition.xz) / (g_VolumetricCloudsShadow.SettingInfo00.z), 0).rgb;
  float result = saturate((saturate(weatherData.b - g_VolumetricCloudsShadow.SettingInfo00.y)) + g_VolumetricCloudsShadow.StandardPosition.w);
  float result2 = result * result;
  return result2 * result2;
#else
  float3 weatherData = weatherTexture.SampleLevel(g_LinearWrap, (ws_pos.xz + g_VolumetricCloudsShadow.StandardPosition.xz) / (g_VolumetricCloudsShadow.SettingInfo00.z), 0).rgb;
  return saturate(weatherData.g + g_VolumetricCloudsShadow.StandardPosition.w);
#endif
}

// CONFFX_END JIN




struct PsIn
{
    float4 position : SV_Position;
    float2 screenPos : TEXCOORD0;
};

float4 main(PsIn In) : SV_Target {

	float2 texCoord = In.screenPos * 0.5 - 0.5;
	texCoord.x = 1.0 - texCoord.x;

	float3 albedo = BasicTexture.Sample(g_LinearMirror, texCoord).rgb;
	float3 normal = NormalTexture.Sample(g_LinearMirror, texCoord).xyz;
  
	float lighting = saturate(dot(normal, LightDirection.xyz));

  float2 texCoord2 = float2((In.screenPos.x + 1.0) * 0.5, (1.0 - In.screenPos.y) * 0.5);

  float4 ws_pos = mul(InvViewProjMat, float4(In.screenPos.xy, depthTexture.Sample(g_LinearWrap, texCoord2).r , 1.0));
 
  float shadow_atten_from_clouds = EvaluateCloudShadow(ws_pos.xyz, LightDirection.xyz);
  //return float4(shadow_atten_from_clouds, shadow_atten_from_clouds, shadow_atten_from_clouds, shadow_atten_from_clouds);
	return float4(lighting * albedo * SunColor.rgb * LightColor.rgb * LightColor.a * shadow_atten_from_clouds + albedo * 0.1, 1.0);
}
