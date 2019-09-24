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
layout(location = 0) out vec4 rast_FragData0; 

layout(UPDATE_FREQ_NONE, binding = 2) uniform texture2D skydomeTexture;
layout(UPDATE_FREQ_NONE, binding = 3) uniform texture2D SrcTexture;
layout(UPDATE_FREQ_NONE, binding = 4) uniform sampler g_LinearClamp;
struct PSIn
{
    vec4 Position;
    vec2 TexCoord;
};
vec4 HLSLmain(PSIn input0)
{
    vec4 sceneColor = texture(sampler2D( SrcTexture, g_LinearClamp), vec2((input0).TexCoord));
    return sceneColor;
}
void main()
{
    PSIn input0;
    input0.Position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    input0.TexCoord = fragInput_TEXCOORD;
    vec4 result = HLSLmain(input0);
    rast_FragData0 = result;
}
