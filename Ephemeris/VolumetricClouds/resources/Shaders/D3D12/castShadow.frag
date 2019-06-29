/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

Texture2D depthTexture : register(t2);
Texture2D noiseTexture : register(t3);

SamplerState PointClampSampler : register(s1);
SamplerState BilinearSampler : register(s2);

cbuffer uniformGlobalInfoRootConstant : register(b0)
{
	float2 _Time;
	float2 screenSize;
	float4 lightDirection;
	float4 lightColorAndIntensity;
	float4 cameraPosition;

	float4x4 VP;
	float4x4 InvVP;
	float4x4 InvWorldToCamera;
	float4x4 prevVP;
	float4x4 LP;

	float near;
	float far;

	float correctU;
	float correctV;

	float4 ProjectionExtents;
	uint lowResFrameIndex;

	uint JitterX;
	uint JitterY;

	float exposure;
	float decay;
	float density;
	float weight;

	uint NUM_SAMPLES;

	float4 skyBetaR;
	float4 skyBetaV;

	float turbidity;
	float rayleigh;
	float mieCoefficient;
	float mieDirectionalG;
}

struct PSIn {
	float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
	float2 VSray: TEXCOORD1;
};

float main(PSIn input) : SV_TARGET
{	
	return 1.0f;
}
/*
float main(PSIn input) : SV_TARGET
{	

	float3 WindDir = float3(WindDirectionX, WindDirectionY, WindDirectionZ);
	float SceneDepth = depthTexture.Sample(PointClampSampler, input.TexCoord);


	//shadow
	if (SceneDepth < 1.0)
	{

		float3 ScreenNDC;

		//shouldn't over 1.0
		ScreenNDC.x = input.TexCoord.x * 2.0 - 1.0;
		ScreenNDC.y = (1.0 - input.TexCoord.y) * 2.0 - 1.0;

		float4 worldPos0 = mul(InvVP, float4(ScreenNDC.xy, SceneDepth, 1.0));
		worldPos0 /= worldPos0.w;

		float3 sunpos = lightDirection.xyz * 100000000.0;


		float yFactor = (CLOUDS_START - worldPos0.y) / (sunpos.y - worldPos0.y);
		float xFactor = yFactor * (sunpos.x - worldPos0.x) + worldPos0.x;
		float zFactor = yFactor * (sunpos.z - worldPos0.z) + worldPos0.z;


#if ALLOW_CONTROL_CLOUD
		float EARTH_RADIUS_ADD_CLOUDS_START = (EARTH_RADIUS + CLOUDS_START);
		float EARTH_RADIUS_ADD_CLOUDS_END = (EARTH_RADIUS + CLOUDS_START + CLOUDS_THICKNESS);

		float3 EARTH_CENTER = float3(0, -EARTH_RADIUS, 0);


		float it1, it2;
		ray_trace_sphere(worldPos0.xyz, lightDirection.xyz, EARTH_CENTER, EARTH_RADIUS_ADD_CLOUDS_START*EARTH_RADIUS_ADD_CLOUDS_START, it1, it2);
		float3 CloudPos = worldPos0.xyz + lightDirection.xyz * it2;

#else 
		float it1, it2;
		ray_trace_sphere(worldPos0.xyz, lightDirection.xyz, _EARTH_CENTER, _EARTH_RADIUS_ADD_CLOUDS_START2, it1, it2);
		float3 CloudPos = worldPos0.xyz + lightDirection.xyz * it2;

#endif
		

		float speed = _Time.y * WindSpeed;

		float3 weatherData = noiseTexture.SampleLevel(BilinearSampler, (CloudPos.xz + speed * (float2(WindDirectionX, WindDirectionZ))) / (WeatherTextureTiling), 0).rgb;

	

		float noiseR = max(saturate(weatherData.r - CloudCoverage * 3.0),  abs(1.0 - lightDirection.y));


		return noiseR;		
	}
	else
	{
		return 1.0f;
	}

}
*/