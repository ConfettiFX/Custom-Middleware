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

struct PSOut
{
	float4 VolumetricClouds : SV_Target0;
	float4 ResultColor : SV_Target1;
};

Texture2D<float4> g_SrcTexture2D : register(t7);
Texture2D<float4> g_SkyBackgroudTexture: register(t8);

RWStructuredBuffer<float4> TransmittanceColor : register(u1);


float4 main(PSIn input) : SV_TARGET
{
	float4 volumetricCloudsResult = g_SrcTexture2D.Sample(g_LinearClampSampler, input.TexCoord);

	float intensity = volumetricCloudsResult.r;
	float density = volumetricCloudsResult.a;
  float atmosphereBlendFactor = min(volumetricCloudsResult.g, 1.0f);

	float3 BackgroudColor = g_SkyBackgroudTexture.Sample(g_LinearClampSampler, input.TexCoord).rgb;

	float3 TransmittanceRGB = TransmittanceColor[0].rgb;

	float4 PostProcessedResult;
	PostProcessedResult.a = density;
	PostProcessedResult.rgb = (intensity / max(density, 0.000001)) * lerp(TransmittanceRGB , lerp(g_VolumetricClouds.lightColorAndIntensity.rgb, TransmittanceRGB, pow(saturate(1.0 - g_VolumetricClouds.lightDirection.y), 0.5)), g_VolumetricClouds.Test00) * g_VolumetricClouds.lightColorAndIntensity.a * g_VolumetricClouds.CloudBrightness;
  PostProcessedResult.rgb = lerp(BackgroudColor, PostProcessedResult.rgb, PostProcessedResult.a * g_VolumetricClouds.BackgroundBlendFactor);
  PostProcessedResult.rgb = lerp(PostProcessedResult.rgb, BackgroudColor, pow(atmosphereBlendFactor, 1.4));

	return float4(PostProcessedResult.rgb, 1.0);
}