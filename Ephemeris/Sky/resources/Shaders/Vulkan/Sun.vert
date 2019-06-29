/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

layout(location = 0) in vec4 POSITION;
layout(location = 1) in vec4 NORMAL;
layout(location = 0) out vec4 vertOutput_Position;

layout(set = 0, binding = 9) uniform SunUniform_Block
{
    mat4 ViewMat;
    mat4 ViewProjMat;
    vec4 LightDirection;
    vec4 Dx;
    vec4 Dy;
}SunUniform;

struct GsIn
{
    vec4 OffsetScale;
};
GsIn HLSLmain(uint VertexID)
{
    GsIn Out;
    (((Out).OffsetScale).xyz = (SunUniform.LightDirection).xyz);
    return Out;
}
void main()
{
    uint VertexID;
    VertexID = gl_VertexIndex;
    GsIn result = HLSLmain(VertexID);
    vertOutput_Position = result.OffsetScale;
}