/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

layout(location = 0) in vec2 fragInput_TexCoord0;
layout(location = 1) in vec3 fragInput_TexCoord1;
layout(location = 0) out vec4 rast_FragData0; 

struct PsIn
{
    vec4 position;
    vec2 texCoord;
    vec3 color;
};
vec4 HLSLmain(PsIn In)
{
    return vec4(((In).texCoord).x, ((In).texCoord).y, 0.0, 1.0);
}
void main()
{
    PsIn In;
    In.position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    In.texCoord = fragInput_TexCoord0;
    In.color = fragInput_TexCoord1;
    vec4 result = HLSLmain(In);
    rast_FragData0 = result;
}