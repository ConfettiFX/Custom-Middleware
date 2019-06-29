/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "volumetricCloud.h"

struct PSIn {
	float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
	float2 VSray: TEXCOORD1;
};

Texture2D<float4> g_PostProcessedTexture : register(t11);

float4 main(PSIn input) : SV_TARGET
{
	float4 volumetricCloudsResult = g_PostProcessedTexture.Sample(g_LinearClampSampler, input.TexCoord);
	return float4(volumetricCloudsResult.rgb, 1.0f);
}