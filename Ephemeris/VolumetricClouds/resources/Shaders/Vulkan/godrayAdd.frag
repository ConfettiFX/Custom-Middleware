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
layout(location = 0) out vec4 rast_FragData0; 

struct PSIn
{
    vec4 Position;
    vec2 TexCoord;
};
struct PSOut
{
    vec4 VolumetricClouds;
    vec4 ResultColor;
};

vec4 HLSLmain(PSIn input0)
{
    vec2 db_uvs = (input0).TexCoord;
    vec3 sunWorldPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz * vec3 (63600000.0));
    vec4 sunPos;
    (sunPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerEye[0].m_WorldToProjMat)*(vec4(sunWorldPos, 1.0))));
    ((sunPos).xy /= vec2 ((sunPos).w));
    vec2 ScreenNDC = vec2(((((input0).TexCoord).x * float (2.0)) - float (1.0)), (((float (1.0) - ((input0).TexCoord).y) * float (2.0)) - float (1.0)));
    vec3 projectedPosition = vec3((ScreenNDC).xy, 0.0);
    vec4 worldPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerEye[0].m_ProjToWorldMat)*(vec4(projectedPosition, 1.0)));
    (worldPos /= vec4 ((worldPos).w));
    vec4 CameraPosition = (VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerEye[0].cameraPosition;
    vec3 viewDir = normalize(((worldPos).xyz - (CameraPosition).xyz));
    float cosTheta = clamp(dot(viewDir, ((VolumetricCloudsCBuffer.g_VolumetricClouds).lightDirection).xyz), 0.0, 1.0);
    if(((cosTheta <= 0.0) || ((sunPos).z < float (0.0))))
    {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
    float sceneDepth = float ((textureLod(sampler2D( depthTexture, g_LinearClampSampler), vec2((input0).TexCoord), float (0))).r);
    vec4 GodrayColor = vec4 (clamp(texture(sampler2D( g_GodrayTexture, g_LinearClampSampler), vec2((input0).TexCoord)), 0.0, 1.0));
    return pow(GodrayColor, vec4(1.0));
}

void main()
{
    PSIn input0;
    input0.Position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    input0.TexCoord = fragInput_TEXCOORD;
    vec4 result = HLSLmain(input0);
    rast_FragData0 = result;
}


