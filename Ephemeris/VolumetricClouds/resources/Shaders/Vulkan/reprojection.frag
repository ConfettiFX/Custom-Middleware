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
    float outOfBound;
    vec2 _Jitter = vec2(float((VolumetricCloudsCBuffer.g_VolumetricClouds).m_JitterX), float((VolumetricCloudsCBuffer.g_VolumetricClouds).m_JitterY));
    vec2 onePixeloffset = (vec2 (1.0) / ((VolumetricCloudsCBuffer.g_VolumetricClouds).TimeAndScreenSize).zw);
    vec2 uv = ((input0).m_Tex0 - ((_Jitter - vec2 (1.5)) * onePixeloffset));
    vec4 currSample = texture(sampler2D( LowResCloudTexture, g_PointClampSampler), vec2(uv));
    float depth = UnPackFloat16((currSample).z);
    vec4 prevUV;
    vec4 worldPos;
    vec2 NDC = vec2(((((input0).m_Tex0).x * float (2.0)) - float (1.0)), (((float (1.0) - ((input0).m_Tex0).y) * float (2.0)) - float (1.0)));
    (worldPos = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_ProjToWorldMat_1st)*(vec4(NDC, 0.0, 1.0))));
    (worldPos /= vec4 ((worldPos).w));
    vec3 viewDir = normalize(((worldPos).xyz - ((VolumetricCloudsCBuffer.g_VolumetricClouds).cameraPosition_1st).xyz));
    vec3 firstHitRayPos = ((viewDir * vec3 (depth)) + ((VolumetricCloudsCBuffer.g_VolumetricClouds).cameraPosition_1st).xyz);
    (prevUV = (((VolumetricCloudsCBuffer.g_VolumetricClouds).m_PrevWorldToProjMat_1st)*(vec4(firstHitRayPos, 1.0))));
    (prevUV /= vec4 ((prevUV).w));
    ((prevUV).xy = vec2((((prevUV).x + float (1.0)) * float (0.5)), ((float (1.0) - (prevUV).y) * float (0.5))));
    (outOfBound = step(float (0.0), max(max((-(prevUV).x), (-(prevUV).y)), (max((prevUV).x, (prevUV).y) - float (1.0)))));
    vec4 prevSample = texture(sampler2D( g_PrevFrameTexture, g_LinearClampSampler), vec2((prevUV).xy));
    float blend = max(ShouldbeUpdated((input0).m_Tex0, _Jitter), outOfBound);
    return vec4 (mix(vec4 (prevSample), vec4 (currSample), vec4 (blend)));
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