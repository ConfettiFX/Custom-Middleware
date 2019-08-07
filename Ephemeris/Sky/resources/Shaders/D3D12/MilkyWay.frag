/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

cbuffer SpaceUniform : register(b1)
{
	float4x4 ViewProjMat;
	float4 LightDirection;
	float4 ScreenSize;
  float4 NebulaHighColor;
  float4 NebulaMidColor;
  float4 NebulaLowColor;
};

Texture2D depthTexture : register(t5);

struct VSOutput {
	float4 Position : SV_POSITION;
	float4 Normal : NORMAL;
  float4 Info : COLOR;
	float2 ScreenCoord : TexCoord;
};

float4 main(VSOutput In): SV_Target
{	
	int3 LoadCoord = int3((int2) In.Position, 0);

	float depth = depthTexture.Load(LoadCoord).x;

	if (depth < 1.0f)
		discard;

  //float4 resultSpaceColor = In.Info.x > 0.5f ?  lerp(NebulaMidColor, NebulaHighColor, (In.Info.x - 0.5f) * 2.0f) : lerp(NebulaLowColor, NebulaMidColor, In.Info.x * 2.0f);
  float3 resultSpaceColor = (NebulaLowColor.rgb * NebulaLowColor.a * In.Info.x) + (NebulaMidColor.rgb * NebulaLowColor.a * In.Info.y) + (NebulaHighColor.rgb * NebulaHighColor.a * In.Info.z);

  return float4( float3(0.5, 0.2, 0.1), saturate(saturate(-LightDirection.y + 0.2f) * 2.0f));
}