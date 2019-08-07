/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core


layout(location = 0) out vec3 vertOutput_Position;
layout(location = 1) out vec3 vertOutput_Color;
layout(location = 2) out vec3 vertOutput_TexCoord;

struct AuroraParticle
{
    vec4 PrevPosition;
    vec4 Position;
    vec4 Acceleration;
};
layout(row_major, set=0, binding = 14) buffer AuroraParticleBuffer
{
    AuroraParticle AuroraParticleBuffer_Data[];
};

layout(row_major, set = 0, binding = 13) uniform AuroraUniformBuffer
{
    uint maxVertex;
    float heightOffset;
    float height;
    float deltaTime;
    mat4 ViewProjMat;
};

struct VSOutput
{
    vec3 Position;
    vec3 Color;
    vec3 NextPosition;
};
VSOutput HLSLmain(uint VertexID)
{
    VSOutput result;
    ((result).Color = vec3(0.0, 0.0, 0.0));
    ((result).Position = ((AuroraParticleBuffer_Data[VertexID]).Position).xyz);
    ((result).NextPosition = (result).Position);
    if((VertexID < (maxVertex - uint (1))))
    {
        ((result).NextPosition = ((AuroraParticleBuffer_Data[(VertexID + uint (1))]).Position).xyz);
    }
    return result;
}
void main()
{
    uint VertexID;
    VertexID = gl_VertexIndex;
    VSOutput result = HLSLmain(VertexID);
    vertOutput_Position = result.Position;
    vertOutput_Color = result.Color;
    vertOutput_TexCoord = result.NextPosition;
}
