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
layout(location = 1) in vec2 fragInput_TEXCOORD1;
layout(location = 0) out vec4 rast_FragData0; 

layout(set = 0, binding = 0) uniform texture2D highResCloudTexture;
layout(set = 0, binding = 1) uniform sampler BilinearClampSampler;
layout(push_constant) uniform uniformGlobalInfoRootConstant_Block
{
    vec2 _Time;
    vec2 screenSize;
    vec4 lightDirection;
    vec4 lightColorAndIntensity;
    vec4 cameraPosition;
    mat4 VP;
    mat4 InvVP;
    mat4 InvWorldToCamera;
    mat4 prevVP;
    mat4 LP;
    float near;
    float far;
    float correctU;
    float correctV;
    vec4 ProjectionExtents;
    uint lowResFrameIndex;
    uint JitterX;
    uint JitterY;
    float exposure;
    float decay;
    float density;
    float weight;
    uint NUM_SAMPLES;
    vec4 skyBetaR;
    vec4 skyBetaV;
    float turbidity;
    float rayleigh;
    float mieCoefficient;
    float mieDirectionalG;
}uniformGlobalInfoRootConstant;

struct PSIn
{
    vec4 Position;
    vec2 TexCoord;
    vec2 VSray;
};
vec4 HLSLmain(PSIn input0)
{
    vec4 result = texture(sampler2D( highResCloudTexture, BilinearClampSampler), vec2((input0).TexCoord));
    return result;
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
