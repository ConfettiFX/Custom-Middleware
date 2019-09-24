/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include <metal_stdlib>
using namespace metal;

struct Vertex_Shader
{
    struct VsIn
    {
        float3 position [[attribute(0)]];
        float2 texcoord [[attribute(1)]];
    };
    struct PsIn
    {
        float4 position [[position]];
        float3 positionWS [[sample_perspective]];
        float2 texcoord;
        float3 normal;
        float3 tangent;
        float3 bitangent;
    };
    struct Uniforms_RenderTerrainUniformBuffer
    {
        float4x4 projView;
        float4 TerrainInfo;
        float4 CameraInfo;
    };
    constant Uniforms_RenderTerrainUniformBuffer & RenderTerrainUniformBuffer;
    PsIn main(VsIn In)
    {
        PsIn Out;
        ((Out).position = ((RenderTerrainUniformBuffer.projView)*(float4((In).position, 1.0))));
        ((Out).positionWS = (In).position);
        ((Out).texcoord = (In).texcoord);
        float3 f3Normal = normalize(((In).position - float3(0, (-(RenderTerrainUniformBuffer.TerrainInfo).x), 0)));
        ((Out).normal = f3Normal);
        ((Out).tangent = normalize(cross(f3Normal, float3(0, 0, 1))));
        ((Out).bitangent = normalize(cross((Out).tangent, f3Normal)));
        return Out;
    };

    Vertex_Shader(
constant Uniforms_RenderTerrainUniformBuffer & RenderTerrainUniformBuffer) :
RenderTerrainUniformBuffer(RenderTerrainUniformBuffer) {}
};

struct ArgsPerFrame
{
    constant Vertex_Shader::Uniforms_RenderTerrainUniformBuffer & RenderTerrainUniformBuffer;
};

vertex Vertex_Shader::PsIn stageMain(
    Vertex_Shader::VsIn In [[stage_in]],
    constant ArgsPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    Vertex_Shader::VsIn In0;
    In0.position = In.position;
    In0.texcoord = In.texcoord;
    Vertex_Shader main(
    argBufferPerFrame.RenderTerrainUniformBuffer);
    return main.main(In0);
}
