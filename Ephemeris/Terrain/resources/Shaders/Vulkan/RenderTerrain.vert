/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec3 vertOutput_PositionWS;
layout(location = 1) out vec2 vertOutput_TexCoord;
layout(location = 2) out vec3 vertOutput_Normal;
layout(location = 3) out vec3 vertOutput_Tangent;
layout(location = 4) out vec3 vertOutput_Bitangent;

struct VsIn
{
    vec3 position;
    vec2 texcoord;
};
struct PsIn
{
    vec4 position;
    vec3 positionWS;
    vec2 texcoord;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
};
layout(set = 1, binding = 0) uniform RenderTerrainUniformBuffer_Block
{
    mat4 projView;
    vec4 TerrainInfo;
    vec4 CameraInfo;
}RenderTerrainUniformBuffer;

PsIn HLSLmain(VsIn In)
{
    PsIn Out;
    ((Out).position = ((RenderTerrainUniformBuffer.projView)*(vec4((In).position, 1.0))));
    ((Out).positionWS = (In).position);
    ((Out).texcoord = (In).texcoord);
    vec3 f3Normal = normalize(((In).position - vec3(0, (-(RenderTerrainUniformBuffer.TerrainInfo).x), 0)));
    ((Out).normal = f3Normal);
    ((Out).tangent = normalize(cross(f3Normal, vec3(0, 0, 1))));
    ((Out).bitangent = normalize(cross((Out).tangent, f3Normal)));
    return Out;
}
void main()
{
    VsIn In;
    In.position = Position;
    In.texcoord = TexCoord;
    PsIn result = HLSLmain(In);
    gl_Position = result.position;
    vertOutput_PositionWS = result.positionWS;
    vertOutput_TexCoord = result.texcoord;
    vertOutput_Normal = result.normal;
    vertOutput_Tangent = result.tangent;
    vertOutput_Bitangent = result.bitangent;
}
