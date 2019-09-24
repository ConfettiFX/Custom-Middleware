/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core


layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec2 TEXCOORD0;
layout(location = 0) out vec3 vertOutput_POSITION;
layout(location = 1) out vec3 vertOutput_NORMAL;
layout(location = 2) out vec2 vertOutput_TEXCOORD0;

layout(UPDATE_FREQ_PER_FRAME, binding = 0) uniform RenderTerrainUniformBuffer_Block
{
    mat4 projView;
    vec4 TerrainInfo;
    vec4 CameraInfo;
}RenderTerrainUniformBuffer;

struct VSInput
{
    vec3 Position;
    vec2 Uv;
};
struct VSOutput
{
    vec4 Position;
    vec3 pos;
    vec3 normal;
    vec2 uv;
};
VSOutput HLSLmain(VSInput input0)
{
    VSOutput result;
    (((input0).Position).xyz *= vec3 (10.0));
    ((result).Position = ((RenderTerrainUniformBuffer.projView) * (vec4(((input0).Position).xyz, 1.0))));
    ((result).pos = ((input0).Position).xyz);
    ((result).normal = vec3(0.0, 1.0, 0.0));
    ((result).uv = (input0).Uv);
    return result;
}
void main()
{
    VSInput input0;
    input0.Position = POSITION;
    input0.Uv = TEXCOORD0;
    VSOutput result = HLSLmain(input0);
    gl_Position = result.Position;
    vertOutput_POSITION = result.pos;
    vertOutput_NORMAL = result.normal;
    vertOutput_TEXCOORD0 = result.uv;
}
