/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer StarUniform : register(b2, UPDATE_FREQ_PER_FRAME)
{
	float4x4 RotMat;
	float4x4 ViewProjMat;
	float4 LightDirection;
	float4 Dx;
	float4 Dy;
};

struct PsIn
{
	float4 position         : SV_Position;
	float3 texCoord	        : TexCoord0;
	float2 screenCoord      : TexCoord1;
	float3 color            : TexCoord2;
};

Texture2D depthTexture : register(t5);
Texture2D volumetricCloudsTexture : register(t7);
SamplerState g_LinearClamp : register(s0);

float4 main(PsIn In) : SV_Target
{	
	float2 screenUV = In.screenCoord;

	float sceneDepth = depthTexture.SampleLevel(g_LinearClamp, screenUV, 0).r;
	if(sceneDepth < 1.0)
		discard;

  float density = volumetricCloudsTexture.SampleLevel(g_LinearClamp, screenUV, 0).a;

  float x = 1.0 - abs(In.texCoord.x);
  float x2 = x*x;
  float x4 = x2*x2;
  float x8 = x4*x2;
  float y = 1.0 - abs(In.texCoord.y);
  float y2 = y*y;
  float y4 = y2*y2;
  float y8 = y4*y2;

  float Mask = max( x8 * y8, 0.0);
  float alpha = saturate(saturate(-LightDirection.y + 0.2f) * 2.0f);

  return float4(In.color, Mask * alpha * (1.0 - density));

}