/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

layout(location = 0) in vec2 fragInput_TEXCOORD;
layout(location = 0) out float rast_FragData0; 

layout(set = 1, binding = 0) uniform texture2D SrcTexture;
layout(set = 0, binding = 3) uniform sampler g_LinearClamp;
layout(push_constant) uniform CameraInfoRootConstant_Block
{
    float nearPlane;
    float farPlane;
    float padding00;
    float padding01;
}CameraInfoRootConstant;

float DepthLinearization(float depth)
{
    return ((float (2.0) * CameraInfoRootConstant.nearPlane) / ((CameraInfoRootConstant.farPlane + CameraInfoRootConstant.nearPlane) - (depth * (CameraInfoRootConstant.farPlane - CameraInfoRootConstant.nearPlane))));
}
struct PSIn
{
    vec4 Position;
    vec2 TexCoord;
};
float HLSLmain(PSIn input0)
{
    return (DepthLinearization((texture(sampler2D( SrcTexture, g_LinearClamp), vec2((input0).TexCoord))).r) * CameraInfoRootConstant.farPlane);
}
void main()
{
    PSIn input0;
    input0.Position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    input0.TexCoord = fragInput_TEXCOORD;
    float result = HLSLmain(input0);
    rast_FragData0 = result;
}
