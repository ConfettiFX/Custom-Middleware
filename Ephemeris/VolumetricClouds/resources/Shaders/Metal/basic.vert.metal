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
    struct Uniforms_uniformGlobalInfoRootConstant
    {
        float2 _Time;
        float2 screenSize;
        float4 lightPosition;
        float4 lightColorAndIntensity;
        float4 cameraPosition;
        float4x4 VP;
        float4x4 InvVP;
        float4x4 InvWorldToCamera;
        float4x4 prevVP;
        float4x4 LP;
        float near;
        float far;
        float correctU;
        float correctV;
        float4 ProjectionExtents;
        uint lowResFrameIndex;
        uint JitterX;
        uint JitterY;
        float exposure;
        float decay;
        float density;
        float weight;
        uint NUM_SAMPLES;
        float4 skyBetaR;
        float4 skyBetaV;
        float turbidity;
        float rayleigh;
        float mieCoefficient;
        float mieDirectionalG;
    };
    constant Uniforms_uniformGlobalInfoRootConstant & uniformGlobalInfoRootConstant;
	
    struct VSOutput
    {
        float4 Position [[position]];
        float2 TexCoord;
        float2 VSray;
    };
	
    VSOutput main(uint VertexID)
    {
        VSOutput Out;
        float4 position;
        ((position).x = (float)((((VertexID == (uint)(2)))?(3.0):((-1.0)))));
        ((position).y = (float)((((VertexID == (uint)(0)))?((-3.0)):(1.0))));
        ((position).zw = (float2)(1.0));
        ((Out).Position = position);
        ((Out).TexCoord = (((position).xy * float2(0.5, (-0.5))) + (float2)(0.5)));
        ((Out).VSray = (((position).xy * (uniformGlobalInfoRootConstant.ProjectionExtents).xy) + (uniformGlobalInfoRootConstant.ProjectionExtents).zw));
        return Out;
    };

    Vertex_Shader(
constant Uniforms_uniformGlobalInfoRootConstant & uniformGlobalInfoRootConstant) :
uniformGlobalInfoRootConstant(uniformGlobalInfoRootConstant) {}
};


vertex Vertex_Shader::VSOutput stageMain(
    uint VertexID [[vertex_id]],
    constant Vertex_Shader::Uniforms_uniformGlobalInfoRootConstant & uniformGlobalInfoRootConstant [[buffer(2)]])
{
    Vertex_Shader main(uniformGlobalInfoRootConstant);
    return main.main(VertexID);
}
