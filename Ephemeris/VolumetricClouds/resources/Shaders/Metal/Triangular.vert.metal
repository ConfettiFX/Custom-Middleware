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
    struct VSInput
    {
        uint VertexID [[vertex_id]];
    };
    struct VSOutput
    {
        float4 Position [[position]];
        float2 TexCoord;
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
        return Out;
    };

    Vertex_Shader(
) {}
};


vertex Vertex_Shader::VSOutput stageMain(uint VertexID [[vertex_id]])
{
    Vertex_Shader main;
    return main.main(VertexID);
}
