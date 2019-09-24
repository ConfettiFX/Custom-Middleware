/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core


layout(location = 0) in vec3 fragInput_POSITION;
layout(location = 1) in vec3 fragInput_NORMAL;
layout(location = 2) in vec2 fragInput_TEXCOORD0;
layout(location = 0) out vec4 rast_FragData0;

struct VSOutput
{
    vec4 Position;
    vec3 pos;
    vec3 normal;
    vec2 uv;
};
vec4 HLSLmain(VSOutput input0)
{
    float NoL = clamp(dot((input0).normal, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);
    return vec4(NoL, NoL, NoL, 1.0);
}
void main()
{
    VSOutput input0;
    input0.Position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    input0.pos = fragInput_POSITION;
    input0.normal = fragInput_NORMAL;
    input0.uv = fragInput_TEXCOORD0;
    vec4 result = HLSLmain(input0);
    rast_FragData0 = result;
}