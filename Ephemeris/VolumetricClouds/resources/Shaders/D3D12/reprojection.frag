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
	float4 m_Position : SV_POSITION;
    float2 m_Tex0 : TEXCOORD;
	float2 VSray: TEXCOORD1;
};

float4 main(in PSIn input) : SV_TARGET
{
	float2 db_uvs = input.m_Tex0;

	float outOfBound;

	float2 _Jitter = float2(float(g_VolumetricClouds.m_JitterX), float(g_VolumetricClouds.m_JitterY));	
	float2 onePixeloffset = (1.0 / g_VolumetricClouds.TimeAndScreenSize.zw);
	float2 uv = input.m_Tex0 - (_Jitter - 1.5) * onePixeloffset;


	float4 currSample = LowResCloudTexture.Sample(g_PointClampSampler, uv);
	float depth = UnPackFloat16(currSample.z);
	float4 prevUV;
	float4 worldPos;

	
	float2 NDC = float2(input.m_Tex0.x * 2.0 - 1.0, (1.0 - input.m_Tex0.y) * 2.0 - 1.0);
	worldPos = mul(g_VolumetricClouds.m_ProjToWorldMat_1st, float4(NDC, 0.0, 1.0));
	worldPos /= worldPos.w;
	
	/*
	float3 vspos = float3(input.VSray, 1.0) * depth;
	worldPos = mul(g_VolumetricClouds.m_ViewToWorldMat_1st, float4(vspos, 1.0));
	*/

	float3 viewDir = normalize(worldPos.xyz - g_VolumetricClouds.cameraPosition_1st.xyz);

	float3 firstHitRayPos = (viewDir * depth) + g_VolumetricClouds.cameraPosition_1st.xyz;
	prevUV = mul(g_VolumetricClouds.m_PrevWorldToProjMat_1st, float4(firstHitRayPos, 1.0));
	
	prevUV /= prevUV.w;

	prevUV.xy = float2((prevUV.x + 1.0) * 0.5, (1.0 - prevUV.y) * 0.5);

	//Check that projected texture coordinates are out bounded or not
	outOfBound = step(0.0, max(max(-prevUV.x, -prevUV.y), max(prevUV.x, prevUV.y) - 1.0));


	

	float4 prevSample = g_PrevFrameTexture.Sample(g_LinearClampSampler, prevUV.xy);

	float blend = max(ShouldbeUpdated(input.m_Tex0, _Jitter), outOfBound);
	return lerp(prevSample, currSample, blend);
	//return lerp(float4(0.0, 1.0, 0.0, 0.0), float4(1.0, 0.0, 0.0, 0.0), blend);
}