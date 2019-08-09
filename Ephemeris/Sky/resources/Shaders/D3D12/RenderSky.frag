/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "RenderSky.h"

struct PsIn {
	float4 position: SV_Position;
	float3 texCoord: TexCoord;
	float2 ScreenCoord: TexCoord1;
};

float DepthLinearization(float depth)
{
	return (2.0 * QNNear.z) / (QNNear.w + QNNear.z - depth * (QNNear.w - QNNear.z));
}


float4 main(PsIn In): SV_Target
{
	int3 LoadCoord = int3((int2)In.position, 0);
	float4 OriginalImage = SceneColorTexture.Load(LoadCoord);
	float3 ReduceOriginalImage = OriginalImage.rgb * 0.3f;

	float3 _groundColor = float3(0,0,0);
	float3 _spaceColor = float3(0,0,0);
	float3 inscatterColor = float3(0,0,0);
	
	if(LightDirection.y < -0.3f)
	{
		_groundColor = ReduceOriginalImage.xyz * ISun / M_PI;
		return float4(HDR_NORM(_groundColor) + HDR(_spaceColor + inscatterColor) + OriginalImage.rgb, 0.0);
	}
	
	
	float depth = Depth.Load(LoadCoord).x;	

	float fixedDepth = (-QNNear.y * QNNear.x) / (depth - QNNear.x);
	//float fixedDepth = depth / QNNear.w;

	float3 ray = In.texCoord;
    float3 x = CameraPosition.xyz;
    float3 v = normalize(ray);

    float r = length(x);
    float mu = dot(x, v) / r;
	float VoRay = dot(v, ray);
    float t = (depth < 1.0f) ? VoRay * fixedDepth : 0.0f;


    float3 attenuation;
	inscatterColor = inscatter(x, t, v, LightDirection.xyz, r, mu, attenuation); //S[L]-T(x,xs)S[l]|xs

    
	
    
	
    float3 _sunColor = sunColor(x, t, v, LightDirection.xyz, r, mu); //L0
	

	if ( t <= 0.0 ) 
	{
		float3 _transmittance = r <= Rt ? transmittance(r, mu) : (1.0).xxx; // T(x,xo)
		_spaceColor = ReduceOriginalImage.xyz * _transmittance;
	}
	else		
		_groundColor = ReduceOriginalImage.xyz * ISun / M_PI;
	
	inscatterColor *= InScatterParams.x + InScatterParams.y;

	return float4(HDR_NORM(_groundColor) + HDR(_spaceColor + inscatterColor) + OriginalImage.rgb, 0.0);
}

//#endif