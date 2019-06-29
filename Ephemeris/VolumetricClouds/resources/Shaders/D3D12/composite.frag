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
Texture2D<float4> g_PrevVolumetricCloudTexture : register(t12);


float4 main(PSIn input) : SV_TARGET
{
	float4 volumetricCloudsResult = g_PostProcessedTexture.Sample(g_LinearClampSampler, input.TexCoord);
	float sceneDepth = depthTexture.SampleLevel(g_LinearClampSampler, input.TexCoord, 0).r;

	float atmosphericBlendFactor = UnPackFloat16(g_PrevVolumetricCloudTexture.SampleLevel(g_LinearClampSampler, input.TexCoord, 0).g);

  if(g_VolumetricClouds.EnabledDepthCulling > 0.5)
	  return float4(volumetricCloudsResult.rgb, sceneDepth >= 1.0 /*g_VolumetricClouds.CameraFarClip*/ ? 1.0 - getAtmosphereBlendForComposite(atmosphericBlendFactor) : 0.0f);
  else
    return float4(volumetricCloudsResult.rgb, 1.0 - getAtmosphereBlendForComposite(atmosphericBlendFactor));
}