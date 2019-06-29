/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

struct PsIn
{
    float4 position : SV_Position;
    sample float2 screenPos : ScreenPos;
};

PsIn main(uint vertexID : SV_VertexID)
{
    // Produce a fullscreen triangle    
    PsIn Out;
    Out.position.x = (vertexID == 2) ? 3.0 : -1.0;
    Out.position.y = (vertexID == 0) ? -3.0 : 1.0;
    Out.position.zw = float2(1,1);
    Out.screenPos = Out.position.xy;
    return Out;
}