/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_samplerless_texture_functions : enable

#include "volumetricCloud.h"

layout(location = 0) in vec2 fragInput_TEXCOORD;
layout(location = 1) in vec2 fragInput_TEXCOORD1;
layout(location = 0) out vec4 rast_FragData0;

struct PSIn
{
  vec4 Position;
  vec2 TexCoord;
  vec2 VSray;
};
struct PSOut
{
  vec4 VolumetricClouds;
  vec4 ResultColor;
};

vec4 HLSLmain(PSIn input0)
{
  vec4 volumetricCloudsResult = texture(sampler2D(g_SrcTexture2D, g_LinearClampSampler), input0.TexCoord);

  float intensity = volumetricCloudsResult.r;
  float density = volumetricCloudsResult.a;

  vec3 BackgroudColor = texture(sampler2D(g_SkyBackgroudTexture, g_LinearClampSampler), input0.TexCoord).rgb;

  vec3 TransmittanceRGB = TransmittanceColor_Data[0].rgb;

  vec4 PostProcessedResult;
  PostProcessedResult.a = density;
  PostProcessedResult.rgb = intensity * mix(TransmittanceRGB , mix(VolumetricCloudsCBuffer.g_VolumetricClouds.lightColorAndIntensity.rgb, TransmittanceRGB, pow(clamp(1.0 - VolumetricCloudsCBuffer.g_VolumetricClouds.lightDirection.y, 0.0, 1.0), 0.5)), VolumetricCloudsCBuffer.g_VolumetricClouds.Test00) * VolumetricCloudsCBuffer.g_VolumetricClouds.lightColorAndIntensity.a * VolumetricCloudsCBuffer.g_VolumetricClouds.CloudBrightness;
  PostProcessedResult.rgb = mix(clamp(BackgroudColor, 0.0, 1.0) + PostProcessedResult.rgb, PostProcessedResult.rgb, min(PostProcessedResult.a, VolumetricCloudsCBuffer.g_VolumetricClouds.BackgroundBlendFactor));
  PostProcessedResult.rgb = mix(BackgroudColor, PostProcessedResult.rgb, VolumetricCloudsCBuffer.g_VolumetricClouds.BackgroundBlendFactor);
  return vec4(PostProcessedResult.rgb, 1.0);
}

void main()
{
  PSIn input0;
  input0.Position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
  input0.TexCoord = fragInput_TEXCOORD;
  input0.VSray = fragInput_TEXCOORD1;
  vec4 result = HLSLmain(input0);
  rast_FragData0 = result;
}