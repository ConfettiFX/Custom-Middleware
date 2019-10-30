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
    vec4 m_Position;
    vec2 m_Tex0;
    vec2 VSray;
};

vec4 HLSLmain(in PSIn input0)
{
    vec2 db_uvs = (input0).m_Tex0;
    vec2 ScreenUV = (input0).m_Tex0;
    ((ScreenUV).x += (VolumetricCloudsCBuffer.g_VolumetricClouds).m_CorrectU);
    ((ScreenUV).y += (VolumetricCloudsCBuffer.g_VolumetricClouds).m_CorrectV);
    vec3 ScreenNDC;
    ((ScreenNDC).x = (((ScreenUV).x * float (2.0)) - float (1.0)));
    ((ScreenNDC).y = (((float (1.0) - (ScreenUV).y) * float (2.0)) - float (1.0)));
    vec3 projectedPosition = vec3((ScreenNDC).xy, 0.0);
    vec4 worldPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerEye[0].m_ProjToWorldMat)*(vec4(projectedPosition, 1.0)));
    vec4 CameraPosition = (VolumetricCloudsCBuffer.g_VolumetricClouds).m_DataPerEye[0].cameraPosition;
    (worldPos /= vec4 ((worldPos).w));
    vec3 viewDir = normalize(((worldPos).xyz - (CameraPosition).xyz));
    float intensity;
    float atmosphereBlendFactor;
    float depth;

    float randomSeed = mix(0.0, (VolumetricCloudsCBuffer.g_VolumetricClouds).Random00, (VolumetricCloudsCBuffer.g_VolumetricClouds).m_UseRandomSeed);    
    float dentisy = GetDensity(CameraPosition.xyz, worldPos.xyz, viewDir, randomSeed, intensity, atmosphereBlendFactor, depth, db_uvs);
    return vec4(intensity, atmosphereBlendFactor, depth, dentisy);
}

void main()
{
    PSIn input0;
    input0.m_Position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    input0.m_Tex0 = fragInput_TEXCOORD;
    input0.VSray = fragInput_TEXCOORD1;
    vec4 result = HLSLmain(input0);
    rast_FragData0 = result;
}