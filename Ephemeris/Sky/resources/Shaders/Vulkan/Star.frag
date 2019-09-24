/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

layout(location = 0) in vec3 fragInput_TexCoord;
layout(location = 1) in vec2 fragInput_ScreenCoord;
layout(location = 2) in vec3 fragInput_Color;

layout(location = 0) out vec4 rast_FragData0; 

layout(UPDATE_FREQ_PER_FRAME, binding = 11) uniform StarUniform_Block
{
    mat4 RotMat;
    mat4 ViewProjMat;
    vec4 LightDirection;
    vec4 Dx;
    vec4 Dy;
}StarUniform;

struct PsIn
{
    vec4 position;
    vec3 texCoord;
    vec2 screenCoord;
    vec3 color;
};
layout(UPDATE_FREQ_NONE, binding = 8) uniform texture2D depthTexture;
layout(UPDATE_FREQ_NONE, binding = 5) uniform sampler g_LinearClamp;
vec4 HLSLmain(PsIn In)
{
    vec2 screenUV = (In).screenCoord;
    float sceneDepth = float ((textureLod(sampler2D( depthTexture, g_LinearClamp), vec2(screenUV), float (0))).r);
    if((sceneDepth < float (1.0)))
    {
        discard;
    }

    float x = 1.0 - abs(In.texCoord.x);
    float x2 = x*x;
    float x4 = x2*x2;
    float x8 = x4*x2;
    float y = 1.0 - abs(In.texCoord.y);
    float y2 = y*y;
    float y4 = y2*y2;
    float y8 = y4*y2;

    float Mask = max( x8 * y8, 0.0);
    float alpha = clamp(clamp(-StarUniform.LightDirection.y + 0.2f, 0.0f, 1.0f) * 2.0f, 0.0f, 1.0f);

    return vec4(In.color, Mask * alpha);
}
void main()
{
    PsIn In;
    In.position     = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    In.texCoord     = fragInput_TexCoord;
    In.screenCoord  = fragInput_ScreenCoord;
    In.color        = fragInput_Color;
    vec4 result     = HLSLmain(In);
    rast_FragData0  = result;
}
