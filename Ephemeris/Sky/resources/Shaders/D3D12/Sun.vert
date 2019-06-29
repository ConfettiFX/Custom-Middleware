/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer SunUniform : register(b3)
{
	float4x4 ViewMat;
	float4x4 ViewProjMat;
	float4 LightDirection;
	float4 Dx;
	float4 Dy;
};

struct GsIn {
	float4 OffsetScale : Position;
};

GsIn main(uint VertexID : SV_VertexID)
{
	GsIn Out;
	Out.OffsetScale.xyz = LightDirection;
	return Out;
}