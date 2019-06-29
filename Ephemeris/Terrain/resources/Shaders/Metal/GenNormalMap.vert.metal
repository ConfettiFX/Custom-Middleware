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
    struct PsIn
    {
        float4 position [[position]];
        float2 screenPos [[sample_perspective]];
    };
    PsIn main(uint vertexID)
    {
        PsIn Out;
        (((Out).position).x = (float)((((vertexID == (uint)(2)))?(3.0):((-1.0)))));
        (((Out).position).y = (float)((((vertexID == (uint)(0)))?((-3.0)):(1.0))));
        (((Out).position).zw = float2(1, 1));
        ((Out).screenPos = ((Out).position).xy);
        return Out;
    };

    Vertex_Shader(
) {}
};


vertex Vertex_Shader::PsIn stageMain(
uint vertexID [[vertex_id]])
{
    uint vertexID0;
    vertexID0 = vertexID;
    Vertex_Shader main;
    return main.main(vertexID0);
}
