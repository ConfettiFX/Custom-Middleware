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

float4 main(PSIn input) : SV_TARGET
{
	float2 db_uvs = input.TexCoord;


	float3 sunWorldPos = g_VolumetricClouds.lightDirection.xyz * 63600000.0f;
	float4 sunPos = mul(g_VolumetricClouds.m_DataPerEye[0].m_WorldToProjMat, float4(sunWorldPos, 1.0));

	sunPos.xy /= sunPos.w;

	// Screen space to NDC
	float2 ScreenNDC = float2(input.TexCoord.x * 2.0 - 1.0, (1.0 - input.TexCoord.y) * 2.0 - 1.0);
	float3 projectedPosition = float3(ScreenNDC.xy, 0.0);

	float4 worldPos = mul(g_VolumetricClouds.m_DataPerEye[0].m_ProjToWorldMat, float4(projectedPosition, 1.0));
	worldPos /= worldPos.w;
	
	float4 CameraPosition = g_VolumetricClouds.m_DataPerEye[0].cameraPosition;
	

	float3 viewDir = normalize(worldPos.xyz - CameraPosition.xyz);
	float cosTheta = saturate(dot(viewDir, g_VolumetricClouds.lightDirection.xyz));

	if (cosTheta <= 0.0f || sunPos.z < 0.0)
		return float4(0.0, 0.0, 0.0, 0.0);

	float cosTheta2 = cosTheta * cosTheta;
	float cosTheta4 = cosTheta2 * cosTheta2;
	float cosTheta16 = cosTheta4 * cosTheta4;
	float cosTheta32 = cosTheta16 * cosTheta2;
	float cosTheta64 = cosTheta16 * cosTheta4;

	float2 deltaTexCoord = float2(ScreenNDC - sunPos.xy);

	if (length(deltaTexCoord) > 1.0)
		deltaTexCoord = normalize(deltaTexCoord);

	deltaTexCoord.y = -deltaTexCoord.y;


	float2 texCoord = db_uvs;
	float2 delta = float2(1.0f, 1.0f);


	deltaTexCoord *= delta / (800.0f * g_VolumetricClouds.GodrayDensity);

	float illuminationDecay = 1.0;
	float finalIntensity = 0.0;

	// If you want to use GodNumSamples, remove [unroll] and change the maximum iteration number(80) to g_VolumetricClouds.g_VolumetricClouds
	[unroll]
	for (uint i = 0; i < 80; i++)
	{
		texCoord -= deltaTexCoord;
			   
		float localDensity = 1.0 - g_PrevFrameTexture.Sample(g_LinearBorderSampler, texCoord).a;

		localDensity *= illuminationDecay * g_VolumetricClouds.GodrayWeight;

		finalIntensity += localDensity;
		illuminationDecay *= g_VolumetricClouds.GodrayDecay;
	}

	finalIntensity *= g_VolumetricClouds.GodrayExposure;

	finalIntensity += min(g_VolumetricClouds.m_DataPerLayer[0].CloudCoverage * 5.0, 0.0);
	finalIntensity = saturate(finalIntensity);	

	return float4(g_VolumetricClouds.lightColorAndIntensity.rgb * g_VolumetricClouds.lightColorAndIntensity.a * finalIntensity * cosTheta32, 0.0);
}


