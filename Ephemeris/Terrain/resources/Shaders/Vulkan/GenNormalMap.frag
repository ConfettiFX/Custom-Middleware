/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

layout(location = 0) in vec2 fragInput_ScreenPos;
layout(location = 0) out vec2 rast_FragData0; 

layout(push_constant) uniform cbRootConstant_Block
{
    float heightScale;
}cbRootConstant;

layout(set = 0, binding = 0) uniform texture2D Heightmap;
layout(set = 0, binding = 1) uniform sampler g_LinearMirror;
struct PsIn
{
    vec4 position;
    vec2 screenPos;
};
float GET_ELEV(vec2 f2ElevMapUV, float fMIPLevel, ivec2 Offset)
{
    return textureLod(sampler2D( Heightmap, g_LinearMirror), vec2(f2ElevMapUV) + Offset, fMIPLevel).r;
}
vec3 ComputeNormal(vec2 f2ElevMapUV, float fSampleSpacingInterval, float fMIPLevel)
{
    float Height00 = GET_ELEV(f2ElevMapUV, fMIPLevel, ivec2((-1), (-1)));
    float Height10 = GET_ELEV(f2ElevMapUV, fMIPLevel, ivec2(0, (-1)));
    float Height20 = GET_ELEV(f2ElevMapUV, fMIPLevel, ivec2((+1), (-1)));
    float Height01 = GET_ELEV(f2ElevMapUV, fMIPLevel, ivec2((-1), 0));
    float Height21 = GET_ELEV(f2ElevMapUV, fMIPLevel, ivec2((+1), 0));
    float Height02 = GET_ELEV(f2ElevMapUV, fMIPLevel, ivec2((-1), (+1)));
    float Height12 = GET_ELEV(f2ElevMapUV, fMIPLevel, ivec2(0, (+1)));
    float Height22 = GET_ELEV(f2ElevMapUV, fMIPLevel, ivec2((+1), (+1)));
    vec3 Grad;
    ((Grad).x = (((Height00 + Height01) + Height02) - ((Height20 + Height21) + Height22)));
    ((Grad).y = (((Height00 + Height10) + Height20) - ((Height02 + Height12) + Height22)));
    ((Grad).z = ((fSampleSpacingInterval * 6.0) * 1 / cbRootConstant.heightScale));
    ((Grad).xy *= (vec2 ((float (65536) * 0.015000000)) * vec2(1.0, (-1.0))));
    vec3 Normal = normalize(Grad);
    return Normal;
}
vec2 HLSLmain(PsIn In)
{
    vec2 f2UV = (vec2(0.5, 0.5) + (vec2(0.5, (-0.5)) * ((In).screenPos).xy));
    vec3 Normal = ComputeNormal(f2UV, (float (0.015000000) * exp2(float (9))), float (9));
    return (Normal).xy;
}
void main()
{
    PsIn In;
    In.position = vec4(gl_FragCoord.xyz, 1.0 / gl_FragCoord.w);
    In.screenPos = fragInput_ScreenPos;
    vec2 result = HLSLmain(In);
    rast_FragData0 = result;
}