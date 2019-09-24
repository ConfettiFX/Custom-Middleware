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

vec4 HLSLmain(PSIn input0)
{
    vec4 volumetricCloudsResult = texture(sampler2D( g_PostProcessedTexture, g_LinearClampSampler), vec2((input0).TexCoord));
    float sceneDepth = float ((textureLod(sampler2D( depthTexture, g_LinearClampSampler), vec2((input0).TexCoord), float (0))).r);
    float atmosphericBlendFactor = UnPackFloat16(float ((textureLod(sampler2D( g_PrevVolumetricCloudTexture, g_LinearClampSampler), vec2((input0).TexCoord), float (0))).g));

    if (VolumetricCloudsCBuffer.g_VolumetricClouds.EnabledDepthCulling > 0.5)
      return vec4(volumetricCloudsResult.rgb, sceneDepth >= 1.0 /*VolumetricCloudsCBuffer.g_VolumetricClouds.CameraFarClip*/ ? 1.0 - getAtmosphereBlendForComposite(atmosphericBlendFactor) : 0.0f);
    else
      return vec4(volumetricCloudsResult.rgb, 1.0 - getAtmosphereBlendForComposite(atmosphericBlendFactor));
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