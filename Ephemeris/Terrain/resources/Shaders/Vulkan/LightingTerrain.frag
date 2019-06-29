/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

layout(location = 0) in vec2 fragInput_ScreenPos;
layout(location = 0) out vec4 rast_FragData0; 

layout(set = 1, binding = 1) uniform LightingTerrainUniformBuffer_Block
{
    mat4 InvViewProjMat;
    mat4 ShadowViewProjMat;
    vec4 ShadowSpheres;
    vec4 LightDirection;
    vec4 SunColor;
    vec4 LightColor;
}LightingTerrainUniformBuffer;

layout(set = 1, binding = 2) uniform VolumetricCloudsShadowCB_Block
{
  vec4	SettingInfo00;			// x : EnableCastShadow, y : CloudCoverage, z : WeatherTextureTiling, w : Time
  vec4	StandardPosition;		// xyz : The current center location for applying wind, w : ShadowBrightness
  vec4  ShadowInfo;
}VolumetricCloudsShadowCB;

layout(set = 0, binding = 16) uniform texture2D BasicTexture;
layout(set = 0, binding = 17) uniform texture2D NormalTexture;
layout(set = 0, binding = 18) uniform texture2D weatherTexture;
layout(set = 0, binding = 19) uniform texture2D depthTexture;

layout(set = 0, binding = 13) uniform sampler g_LinearMirror;
layout(set = 0, binding = 14) uniform sampler g_LinearWrap;

//Code from https://area.autodesk.com/blogs/game-dev-blog/volumetric-clouds/.
bool ray_trace_sphere(vec3 center, vec3 rd, vec3 offset, float radius2, out float t1, out float t2) {
  vec3 p = center - offset;
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
#define _EARTH_CENTER vec3(0, -_EARTH_RADIUS, 0)					// Center position of the Earth

float EvaluateCloudShadow(in vec3 ws_pos, in vec3 lightDir)
{
#if USE_PROJECTED_SHADOW
  float it1, it2;
  ray_trace_sphere(ws_pos.xyz, lightDir, _EARTH_CENTER, _EARTH_RADIUS_ADD_CLOUDS_LAYER_START, it1, it2);
  vec3 CloudPos = ws_pos.xyz + lightDir * it2;

  vec3 weatherData = texture(sampler2D(weatherTexture, g_LinearWrap), (CloudPos.xz + VolumetricCloudsShadowCB.StandardPosition.xz) / (VolumetricCloudsShadowCB.SettingInfo00.z)).rgb;
  float result = clamp((clamp(weatherData.b - VolumetricCloudsShadowCB.SettingInfo00.y, 0.0, 1.0)) + VolumetricCloudsShadowCB.StandardPosition.w, 0.0, 1.0);
  float result2 = result * result;
  return result2 * result2;
#else
  vec3 weatherData = weatherTexture.SampleLevel(g_LinearWrap, (ws_pos.xz + VolumetricCloudsShadowCB.StandardPosition.xz) / (VolumetricCloudsShadowCB.SettingInfo00.z), 0).rgb;
  return saturate(weatherData.g + VolumetricCloudsShadowCB.StandardPosition.w);
#endif
}

// CONFFX_END JIN

struct PsIn
{
    vec4 position;
    vec2 screenPos;
};

vec4 HLSLmain(PsIn In)
{
    vec2 texCoord = (((In).screenPos * vec2 (0.5)) - vec2 (0.5));
    ((texCoord).x = (float (1) - (texCoord).x));
    vec3 albedo = vec3 ((texture(sampler2D( BasicTexture, g_LinearMirror), vec2(texCoord))).rgb);
    vec3 normal = vec3 ((texture(sampler2D( NormalTexture, g_LinearMirror), vec2(texCoord))).xyz);
    float lighting = clamp(dot(normal, (LightingTerrainUniformBuffer.LightDirection).xyz), 0.0, 1.0);

    vec2 texCoord2 = vec2((In.screenPos.x + 1.0) * 0.5, (1.0 - In.screenPos.y) * 0.5);

    vec4 ws_pos = LightingTerrainUniformBuffer.InvViewProjMat * vec4(In.screenPos.xy, texture(sampler2D(depthTexture, g_LinearWrap), texCoord2).r, 1.0);

    float shadow_atten_from_clouds = EvaluateCloudShadow(ws_pos.xyz, LightingTerrainUniformBuffer.LightDirection.xyz);
    return vec4((((((vec3 (lighting) * albedo) * (LightingTerrainUniformBuffer.SunColor).rgb) * (LightingTerrainUniformBuffer.LightColor).rgb) * vec3 ((LightingTerrainUniformBuffer.LightColor).a)) * shadow_atten_from_clouds + (albedo * vec3 (0.1))), 1.0);
}
void main()
{
    PsIn In;
    In.position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    In.screenPos = fragInput_ScreenPos;
    vec4 result = HLSLmain(In);
    rast_FragData0 = result;
}
