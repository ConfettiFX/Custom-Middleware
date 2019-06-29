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
};

struct PSOut
{
	float4 VolumetricClouds : SV_Target0;
	float4 ResultColor : SV_Target1;
};

Texture2D g_GodrayTexture: register(t10);

float4 main(PSIn input) : SV_TARGET
{
	float2 db_uvs = input.TexCoord;

	float3 sunWorldPos = g_VolumetricClouds.lightDirection.xyz * 1000000.0;
	float4 sunPos;
	
	sunPos = mul(g_VolumetricClouds.m_WorldToProjMat_1st, float4(sunWorldPos, 1.0));
	
	sunPos.xy /= sunPos.w;

	// Screen space to NDC
	float2 ScreenNDC = float2(input.TexCoord.x * 2.0 - 1.0, (1.0 - input.TexCoord.y) * 2.0 - 1.0);
	float3 projectedPosition = float3(ScreenNDC.xy, 0.0);
	
	float4 worldPos = mul(g_VolumetricClouds.m_ProjToWorldMat_1st, float4(projectedPosition, 1.0));
	
	worldPos /= worldPos.w;
	
	float4 CameraPosition = g_VolumetricClouds.cameraPosition_1st;
	float3 viewDir = normalize(worldPos.xyz - CameraPosition.xyz);

	float cosTheta = saturate(dot(viewDir, g_VolumetricClouds.lightDirection.xyz));

	if (cosTheta <= 0.0f || sunPos.z < 0.0)
		return float4(0.0, 0.0, 0.0, 0.0);

	float sceneDepth = depthTexture.SampleLevel(g_LinearClampSampler, input.TexCoord, 0).r;

	float4 GodrayColor = saturate(g_GodrayTexture.Sample(g_LinearClampSampler, input.TexCoord));

	return pow(GodrayColor, 1.0);//sceneDepth >= 1.0 ? pow(GodrayColor, 1.0) : float4(0.0, 0.0, 0.0, 0.0);
}


