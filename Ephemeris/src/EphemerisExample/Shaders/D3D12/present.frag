/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

Texture2D skydomeTexture : register(t0);
Texture2D sceneTexture : register(t1);
//Texture2D shadowTexture : register(t2);
SamplerState BilinearClampSampler : register(s0);


struct PSIn {
	float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

float4 main(PSIn input) : SV_TARGET
{	
	float4 sceneColor = sceneTexture.Sample(BilinearClampSampler, input.TexCoord);

	//float4 skydomeColor = skydomeTexture.Sample(BilinearClampSampler, input.TexCoord);
	//float shadow = shadowTexture.Sample(BilinearClampSampler, input.TexCoord).r;	
	//return lerp(skydomeColor, float4(shadow, shadow, shadow, shadow), sceneColor.a);
	//return lerp(skydomeColor, sceneColor, 0.01f);

	return sceneColor;
}