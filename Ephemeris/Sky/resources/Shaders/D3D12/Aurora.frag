/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

struct PsIn {
	float4 position     : SV_Position;
	float2 texCoord	    : TexCoord0;
  float3 color        : TexCoord1;
};

float4 main(PsIn In): SV_Target
{	
	return float4(In.texCoord.x, In.texCoord.y, 0.0, 1.0);
}