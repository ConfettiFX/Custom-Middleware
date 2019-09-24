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
	float2 VSray: TEXCOORD1;
};

struct PSOut
{
	float4 VolumetricClouds : SV_Target0;
	float4 ResultColor : SV_Target1;
};

Texture2D<float4> g_SrcTexture2D : register(t7);
Texture2D<float4> g_SkyBackgroudTexture: register(t8);

RWStructuredBuffer<float4> TransmittanceColor : register(u1);


float4 main(PSIn input) : SV_TARGET
{
	float4 volumetricCloudsResult = g_SrcTexture2D.Sample(g_LinearClampSampler, input.TexCoord);

	float intensity = volumetricCloudsResult.r;
	float density = volumetricCloudsResult.a;

	float3 BackgroudColor = g_SkyBackgroudTexture.Sample(g_LinearClampSampler, input.TexCoord).rgb;

	float3 TransmittanceRGB = TransmittanceColor[0].rgb;

	float4 PostProcessedResult;
	PostProcessedResult.a = density;
	PostProcessedResult.rgb = intensity * lerp(TransmittanceRGB , lerp(g_VolumetricClouds.lightColorAndIntensity.rgb, TransmittanceRGB, pow(saturate(1.0 - g_VolumetricClouds.lightDirection.y), 0.5)), g_VolumetricClouds.Test00) * g_VolumetricClouds.lightColorAndIntensity.a * g_VolumetricClouds.CloudBrightness;
	PostProcessedResult.rgb = lerp(saturate(BackgroudColor) + PostProcessedResult.rgb, PostProcessedResult.rgb, min(PostProcessedResult.a, g_VolumetricClouds.BackgroundBlendFactor));
	PostProcessedResult.rgb = lerp(BackgroudColor, PostProcessedResult.rgb, g_VolumetricClouds.BackgroundBlendFactor);

	return float4(PostProcessedResult.rgb, 1.0);
}



/*
float Rayleigh(float cosTheta)
{
	return THREE_OVER_16PI * (1 + cosTheta * cosTheta);
}

// A simplied version of the total Reayleigh scattering to works on browsers that use ANGLE
float3 simplifiedRayleigh()
{
	return float3(5.3191489361702127659574468085106E-6, 0.0000125, 2.7777777777777777777777777777778E-5);
}
float rayleighPhase(float cosTheta)
{
	return THREE_OVER_16PI * (1.0 + pow(cosTheta, 2.0));
}

float hgPhase(float cosTheta, float g)
{
	float g2 = g * g;
	return ONE_OVER_FOURPI * ((1.0 - g2) / pow(1.0 - 2.0*g*cosTheta + g2, 1.5));
}

float sunIntensity(float zenithAngleCos)
{
	float cutoffAngle = 1.6110731556870734;
	float steepness = 1.5;
	float EE = 1000.0;

	float diff = (cutoffAngle - acos(zenithAngleCos));	
	float x = -(diff / steepness);
	return EE * max(0.0, 1.0 - exp(x));
	//return EE * max(0.0, 1.0 - pow(e, -((cutoffAngle - acos(zenithAngleCos)) / steepness)));
}

float3 totalMie( float T )
{
	float c =  T  * 20E-19;
	return 0.434 * c * float3(1.8399918514433978E14, 2.7798023919660528E14, 4.0790479543861094E14);
}

float3 getAtmosphereColorPhysical(float3 dir, float3 sunDir, float cosTheta, float vSunE, out float3 Fex)
{
	float luminance = 1.0f;

	float3 up = float3(0.0, 1.0, 0.0);

	// optical length at zenith for molecules
	float rayleighZenithLength = 8.4E3;
	float mieZenithLength = 1.25E3;

	float rayleighCoefficient = rayleigh - (1.0 * (1.0 - skyBetaR.a));
	float3 totalRayleigh = float3(5.804542996261093E-6, 1.3562911419845635E-5, 3.0265902468824876E-5);

	float3 vBetaR = totalRayleigh * rayleighCoefficient;
	float3 vBetaM = totalMie(turbidity) * mieCoefficient;
	
	// optical length
	// cutoff angle at 90 to avoid singularity in next formula.
	float zenithAngle = acos( max( 0.0, dir.y) );
	float inverse = 1.0 / ( cos( zenithAngle ) + 0.15 * pow( 93.885 - ( ( zenithAngle * 180.0 ) / PI), -1.253 ) );
	float sR = rayleighZenithLength * inverse;
	float sM = mieZenithLength * inverse;

	// combined extinction factor
	Fex = exp( -(vBetaR * sR + vBetaM * sM ) );

	// in scattering

	float rPhase = rayleighPhase( cosTheta * 0.5 + 0.5 );
	float3 betaRTheta = vBetaR * rPhase;
		
	float mPhase = HenryGreenstein( lerp(mieDirectionalG, mieDirectionalG * 0.5, sunDir.y), cosTheta);
	float3 betaMTheta = vBetaM * mPhase;
	
	float oneMinusSunDir = 1.0 - sunDir.y;
	float oneMinusSunDir2 = oneMinusSunDir * oneMinusSunDir;
	float oneMinusSunDir4 = oneMinusSunDir2 * oneMinusSunDir2;
	
	float factor = (betaRTheta + betaMTheta) / (vBetaR + vBetaM);

	float3 Lin = pow( vSunE * (factor) * ( 1.0 - Fex ), 1.5);
	Lin *= lerp( lerp(Fex * 3.0, float3( 1.0, 1.0, 1.0 ), sunDir.y) , pow( vSunE * (factor) * Fex, float3( 0.5, 0.5, 0.5) ), saturate(oneMinusSunDir4 * oneMinusSunDir));

	// nightsky	
	float3 L0 = Fex * 0.1;

	float3 texColor = ( Lin + L0 ) * 0.04 + float3( 0.0, 0.0003, 0.00075 );	

	float3 retColor = pow(texColor, 1.0 / (1.2 + (1.2 * skyBetaR.a)));
	return retColor * 0.2;
}

float4 main(PSIn input) : SV_TARGET
{
	float2 ScreenUV = input.TexCoord;


	float3 ScreenNDC;

	//shouldn't over 1.0
	ScreenNDC.x = ScreenUV.x * 2.0 - 1.0;
	ScreenNDC.y = (1.0 - ScreenUV.y) * 2.0 - 1.0;

	float3 projectedPosition = float3(ScreenNDC.xy, 0.0);
	
	float4 worldPos = mul(InvVP, float4(projectedPosition, 1.0));
	worldPos /= worldPos.w;


	float3 viewDir = normalize(worldPos.xyz - cameraPosition.xyz);


	float4 currSample = highResCloudTexture.Sample(BilinearClampSampler, input.TexCoord);	

	
	float cosTheta = saturate(dot(viewDir, lightDirection.xyz));
	float3 Fex = float3(0.0, 0.0, 0.0);

	float vSunE = sunIntensity(lightDirection.y);
	float3 skyColor = getAtmosphereColorPhysical(viewDir, lightDirection.xyz, cosTheta, vSunE, Fex);
	//float3 skyColor = float3(0.0, 0.0, 0.0);

	float atmosphericBlendFactor = saturate(currSample.g);
	

	float4 result;
	result.a = currSample.a;
	result.rgb = currSample.r * lightColorAndIntensity.rgb * lightColorAndIntensity.a;
	result.rgb = lerp(skyColor + result.rgb, result.rgb, result.a);
	result.rgb = lerp(result.rgb, skyColor, atmosphericBlendFactor);	


	float cloudDensity = saturate(currSample.a);
	float oneMinuscloudDensity = 1.0 - cloudDensity;	

	float sunDistance = 10000000000.0;


	float4 sunPos = mul(VP, float4(lightDirection.xyz * sunDistance, 1.0));
	sunPos.xy /= sunPos.w;	

	float glow = 0.0;
	float3 sun = float3(0.0, 0.0, 0.0);   
	

	if (sunPos.z > 0.0)
	{	
		Fex *= 3.0;

		float FexIntensity = dot(Fex, float3(1, 1, 1));

		// composition + solar disc
		float sunAngularDiameterCos = 0.999;//0.999956676946448443553574619906976478926848692873900859324;
		float glowAngularDiameterCos = 0.9;
		// 66 arc seconds -> degrees, and the cosine of that

		float glowdisk = smoothstep(glowAngularDiameterCos, glowAngularDiameterCos + 0.09, cosTheta);
		glow = glowdisk * FexIntensity;

		float sundisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos + 0.004, cosTheta);
		sundisk = sundisk * FexIntensity;

		sun = oneMinuscloudDensity * float3(sundisk, sundisk, sundisk);

		float vignette = 1.0 - dot(input.TexCoord.xy - float2(0.5, 0.5), input.TexCoord.xy - float2(0.5, 0.5));
		glow *= vignette;
		glow *= oneMinuscloudDensity;
		glow *= atmosphericBlendFactor * atmosphericBlendFactor;
	}

	return float4(result.rgb + sun, glow);
}
*/