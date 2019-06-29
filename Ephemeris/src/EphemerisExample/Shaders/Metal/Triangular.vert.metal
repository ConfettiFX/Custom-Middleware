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

struct PsIn {
	float4 Position [[position]];
	float2 TexCoord;
};

vertex PsIn stageMain(uint VertexID [[vertex_id]]) {
	PsIn Out;
	
	// Produce a fullscreen triangle
	float4 position;
	position.x = (VertexID == 2) ? 3.0 : -1.0;
	position.y = (VertexID == 0) ? -3.0 : 1.0;
	position.zw = 1.0;
	
	Out.Position = position;
	Out.TexCoord = position.xy * float2(0.5, -0.5) + 0.5;
	
	return Out;
}
