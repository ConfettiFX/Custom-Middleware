/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#version 450 core

layout(location = 0) out vec2 vertOutput_ScreenPos;

struct PsIn
{
    vec4 position;
    vec2 screenPos;
};
PsIn HLSLmain(uint vertexID)
{
    PsIn Out;
    (((Out).position).x = float ((((vertexID == uint (2)))?(float (3.0)):(float ((-1.0))))));
    (((Out).position).y = float ((((vertexID == uint (0)))?(float ((-3.0))):(float (1.0)))));
    (((Out).position).zw = vec2(1, 1));
    ((Out).screenPos = ((Out).position).xy);
    return Out;
}
void main()
{
    uint vertexID;
    vertexID = gl_VertexIndex;
    PsIn result = HLSLmain(vertexID);
    gl_Position = result.position;
    vertOutput_ScreenPos = result.screenPos;
}
