/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer SunUniform : register(b3, UPDATE_FREQ_PER_FRAME)
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
SamplerState g_LinearBorder : register(s3);

float4 main(PsIn In) : SV_Target
{	
	float2 screenUV = In.screenCoord;

	float sceneDepth = depthTexture.SampleLevel(g_LinearBorder, screenUV, 0).r;
	if(sceneDepth < 1.0)
		discard;

	float ISun = 1.f;
  float4 res;

	[branch]
	//If SUN
	if(In.texCoord.z >= 0.0)
	{
    float2 uv = In.texCoord.xy * 2.0 - float2(0.5, 0.5);
		float param = 2*sqrt((uv.x-0.5)*(uv.x-0.5)+(uv.y-0.5)*(uv.y-0.5));
		float blendFactor = smoothstep(1, 0.8, param);

    res = float4(float3(1.0, 0.95294, 0.91765)*ISun, blendFactor);
	}
	//If Moon 
	else
	{
		float3 normal;
		normal.xy = (In.texCoord.xy-0.5)*2;
		normal.z = 1-sqrt(saturate(normal.x*normal.x + normal.y*normal.y));	
		res = moonAtlas.Sample(g_LinearBorder, In.texCoord.xy * 2.0 - float2(0.5, 0.5));   
	}

    float2 glow = saturate(abs(In.texCoord.xy * 1.5 - float2(0.75, 0.75)));
   
    float gl = saturate(1.0f - sqrt(dot(glow, glow)));
    gl = pow(gl, 2.1) * 1.5;
		res = lerp(float4(gl, gl, gl, res.a), res, res.a);
    res.a = saturate(gl + res.a);
    return res;
}