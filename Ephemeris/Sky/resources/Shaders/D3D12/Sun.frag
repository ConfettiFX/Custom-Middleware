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

struct PsIn {
	float4 position : SV_Position;
	float3 texCoord	: TexCoord0;
	float2 screenCoord : TexCoord1;
};

Texture2D depthTexture : register(t5);
Texture2D moonAtlas    : register(t6);
SamplerState g_LinearClampSampler : register(s1);

float4 main(PsIn In) : SV_Target
{	
	float2 screenUV = In.screenCoord;

	float sceneDepth = depthTexture.SampleLevel(g_LinearClampSampler, screenUV, 0).r;
	if(sceneDepth < 1.0)
		discard;

	float ISun = 1.f;

	[branch]
	//If SUN
	if(In.texCoord.z >= 0.0)
	{
		float param = 2*sqrt((In.texCoord.x-0.5)*(In.texCoord.x-0.5)+(In.texCoord.y-0.5)*(In.texCoord.y-0.5));
		float blendFactor = smoothstep(1, 0.8, param);
		return float4(float3(1.0, 0.95294, 0.91765)*ISun, blendFactor);
	}
	//If Moon 
	else
	{
		float3 normal;
		normal.xy = (In.texCoord.xy-0.5)*2;
		normal.z = 1-sqrt(saturate(normal.x*normal.x + normal.y*normal.y));	
		float4 res = moonAtlas.Sample(g_LinearClampSampler, In.texCoord.xy);
		//res.xyz *= smoothstep(-0.05, 0.05, dot(localSunDir, normal));
		return res;
	}

}