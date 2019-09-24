/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "SkyCommon.h"
#include <metal_stdlib>

constant float AVERAGE_GROUND_REFLECTANCE = 0.1;
constant float HR = 8.0;
constant float3 betaR = float3(0.0058000000, 0.0135, 0.033100002);
constant float HM = 1.20000004;
constant float3 betaMSca = float3(0.020000000).xxx;
constant float3 betaMEx = (betaMSca / (const float3)(0.9));
constant float mieG = 0.76;

using namespace metal;

inline float3x3 matrix_ctor(float4x4 m)
{
        return float3x3(m[0].xyz, m[1].xyz, m[2].xyz);
}

struct Fragment_Shader
{
#define TRANSMITTANCE_NON_LINEAR
#define INSCATTER_NON_LINEAR
#define USE_SAMPLELEVEL
	
	struct Uniforms_RenderSkyUniformBuffer
	{
		float4x4 invView;
		float4x4 invProj;
		float4 LightDirection;
		float4 CameraPosition;
		float4 QNNear;
		float4 InScatterParams;
		float4 LightIntensity;
	};
		
	constant Uniforms_RenderSkyUniformBuffer & RenderSkyUniformBuffer;
		
	texture2d<float> SceneColorTexture;
	texture2d<float> Depth;
	texture2d<float> TransmittanceTexture;
	//texture2d<float> IrradianceTexture;
	texture3d<float> InscatterTexture;
		
	sampler g_LinearClamp;
#define FIX
#define IGOR_FIX_MOIRE
#define IGOR_FIX_MU0
	float2 getTransmittanceUV(float r, float mu)
		{
			float uR, uMu;
#ifdef TRANSMITTANCE_NON_LINEAR
			(uR = sqrt(((r - Rg) / (Rt - Rg))));
			(uMu = (atan((((mu + (float)(0.15)) / (float)((1.0 + 0.15))) * (float)(tan(1.5)))) / (float)(1.5)));
#else
			(uR = ((r - Rg) / (Rt - Rg)));
			(uMu = ((mu + (float)(0.15)) / (float)((1.0 + 0.15))));
#endif
			return float2(uMu, uR);
		};
		
	void getTransmittanceRMu(float2 ScreenCoord, thread float(& r), thread float(& muS))
		{
			(r = (ScreenCoord).y);
			(muS = (ScreenCoord).x);
#ifdef TRANSMITTANCE_NON_LINEAR
			(r = (Rg + ((r * r) * (Rt - Rg))));
			(muS = ((float)((-0.15)) + ((tan(((float)(1.5) * muS)) / (float)(tan(1.5))) * (float)((1.0 + 0.15)))));
#else
			(r = (Rg + (r * (Rt - Rg))));
			(muS = ((float)((-0.15)) + (muS * (float)((1.0 + 0.15)))));
#endif
		};
		
	float2 getIrradianceUV(float r, float muS)
		{
			float uR = ((r - Rg) / (Rt - Rg));
			float uMuS = ((muS + (float)(0.2)) / (float)((1.0 + 0.2)));
			return float2(uMuS, uR);
		};
		
	void getIrradianceRMuS(float2 ScreenCoord, thread float(& r), thread float(& muS))
		{
			(r = (Rg + ((ScreenCoord).y * (Rt - Rg))));
			(muS = ((float)((-0.2)) + ((ScreenCoord).x * (float)((1.0 + 0.2)))));
		};
		
	float Contrast(float Input, float ContrastPower)
		{
			bool IsAboveHalf = (Input > (float)(0.5));
			float ToRaise = saturate(((float)(2) * ((IsAboveHalf)?(((float)(1) - Input)):(Input))));
			float Output = ((float)(0.5) * pow(ToRaise, ContrastPower));
			(Output = ((IsAboveHalf)?(((float)(1) - Output)):(Output)));
			return Output;
		};
		
		float4 texture4D(texture3d<float> table, float r, float mu, float muS, float nu)
		{
			float H = sqrt(((Rt * Rt) - (Rg * Rg)));
			float rho = sqrt(((r * r) - (Rg * Rg)));
#ifdef INSCATTER_NON_LINEAR
			float rmu = (r * mu);
			float delta = (((rmu * rmu) - (r * r)) + (Rg * Rg));
			float4 cst = ((((rmu < (float)(0.0)) && (delta > (float)(0.0))))?(float4(1.0, 0.0, 0.0, ((const float)(0.5) - ((const float)(0.5) / float(RES_MU))))):(float4((-1.0), (H * H), H, ((const float)(0.5) + ((const float)(0.5) / float(RES_MU))))));
			float uR = (((const float)(0.5) / float(RES_R)) + ((rho / H) * ((const float)(1.0) - ((const float)(1.0) / float(RES_R)))));
			float uMu = ((cst).w + ((((rmu * (cst).x) + sqrt((delta + (cst).y))) / (rho + (cst).z)) * ((const float)(0.5) - ((const float)(1.0) / float(RES_MU)))));
			float uMuS = (((const float)(0.5) / float(RES_MU_S)) + ((((atan((max(muS, (-0.1975)) * (float)(tan((1.26 * 1.10000002))))) / (float)(1.10000002)) + (float)((1.0 - 0.26))) * (float)(0.5)) * ((const float)(1.0) - ((const float)(1.0) / float(RES_MU_S)))));
#else
			float uR = (((const float)(0.5) / float(RES_R)) + ((rho / H) * ((const float)(1.0) - ((const float)(1.0) / float(RES_R)))));
			float uMu = (((const float)(0.5) / float(RES_MU)) + (((mu + (float)(1.0)) / (float)(2.0)) * ((const float)(1.0) - ((const float)(1.0) / float(RES_MU)))));
			float uMuS = (((const float)(0.5) / float(RES_MU_S)) + ((max((muS + (float)(0.2)), 0.0) / (float)(1.20000004)) * ((const float)(1.0) - ((const float)(1.0) / float(RES_MU_S)))));
#endif
			
			float lerp = (((nu + (float)(1.0)) / (float)(2.0)) * (float(RES_NU) - (const float)(1.0)));
			float uNu = floor(lerp);
			(lerp = (lerp - uNu));
			(lerp = Contrast(lerp, 3));
#ifdef USE_SAMPLELEVEL
			return table.sample(g_LinearClamp, float3(((uNu + uMuS) / float(RES_NU)), uMu, uR), level(0)) * (float4)(((float)(1.0) - lerp)) + (float4)(table.sample(g_LinearClamp, float3((((uNu + uMuS) + (float)(1.0)) / float(RES_NU)), uMu, uR), level(0)) * (float4)(lerp));
#else
			return table.sample(g_LinearClamp, float3(((uNu + uMuS) / float(RES_NU)), uMu, uR)) * (float4)(((float)(1.0) - lerp)) + (float4)(table.sample(g_LinearClamp, float3((((uNu + uMuS) + (float)(1.0)) / float(RES_NU)), uMu, uR)) * (float4)(lerp));
#endif
		};
		
		float mod(float x, float y)
		{
			return (x - (y * floor((x / y))));
		};
		
		void getMuMuSNu(float2 ScreenCoord, float r, float4 dhdH, thread float(& mu), thread float(& muS), thread float(& nu))
		{
			float x = (((ScreenCoord).x * (float)((RES_MU_S * RES_NU))) - (float)(0.5));
			float y = (((ScreenCoord).y * (float)(RES_MU)) - (float)(0.5));
#ifdef INSCATTER_NON_LINEAR
			if ((y < (float)((const float)(RES_MU) / 2.0)))
			{
				float d = ((float)(1.0) - (y / (float)((const float)(RES_MU) / 2.0 - 1.0)));
				(d = min(max((dhdH).z, (d * (dhdH).w)), ((dhdH).w * (float)(0.9990000))));
				(mu = ((((Rg * Rg) - (r * r)) - (d * d)) / (((float)(2.0) * r) * d)));
				(mu = min(mu, ((-sqrt(((float)(1.0) - ((Rg / r) * (Rg / r))))) - (float)(0.0010000000))));
			}
			else
			{
				float d = ((y - (float)((const float)(RES_MU) / 2.0)) / (float)((const float)(RES_MU) / 2.0 - 1.0));
				(d = min(max((dhdH).x, (d * (dhdH).y)), ((dhdH).y * (float)(0.9990000))));
				(mu = ((((Rt * Rt) - (r * r)) - (d * d)) / (((float)(2.0) * r) * d)));
			}
			(muS = (mod(x, (float)(RES_MU_S)) / (float)((const float)(RES_MU_S) - 1.0)));
			(muS = (tan((((((float)(2.0) * muS) - (float)(1.0)) + (float)(0.26)) * (float)(1.10000002))) / (float)(tan((1.26 * 1.10000002)))));
			(nu = ((float)((-1.0)) + ((floor((x / (float)(RES_MU_S))) / (float)((const float)(RES_NU) - 1.0)) * (float)(2.0))));
#else
			(mu = ((float)((-1.0)) + (((float)(2.0) * y) / (float)((const float)(RES_MU) - 1.0))));
			(muS = (mod(x, (float)(RES_MU_S)) / (float)((const float)(RES_MU_S) - 1.0)));
			(muS = ((float)((-0.2)) + (muS * (float)(1.20000004))));
			(nu = ((float)((-1.0)) + ((floor((x / (float)(RES_MU_S))) / (float)((const float)(RES_NU) - 1.0)) * (float)(2.0))));
#endif
		};
		
		float limit(float r, float mu)
		{
			float dout = (((-r) * mu) + sqrt((((r * r) * ((mu * mu) - (float)(1.0))) + (RL * RL))));
			float delta2 = (((r * r) * ((mu * mu) - (float)(1.0))) + (Rg * Rg));
			if ((delta2 >= (float)(0.0)))
			{
				float din = (((-r) * mu) - sqrt(delta2));
				if ((din >= (float)(0.0)))
				{
					(dout = min(dout, din));
				}
			}
			return dout;
		};
		
		float3 transmittance(float r, float mu)
		{
			float2 uv = getTransmittanceUV(r, mu);
#ifdef USE_SAMPLELEVEL
			return TransmittanceTexture.sample(g_LinearClamp, uv, level(0)).rgb;
#else
			return TransmittanceTexture.sample(g_LinearClamp, uv).rgb;
#endif
		};
		
		float3 transmittanceWithShadow(float r, float mu)
		{
			return (((mu < (-sqrt(((float)(1.0) - ((Rg / r) * (Rg / r)))))))?(float3(0.0).xxx):(transmittance(r, mu)));
		};
		
		float3 transmittanceWithShadowSmooth(float r, float mu)
		{
			float eps = ((0.5 * M_PI) / 180.0);
			float horizMu = (-sqrt(((float)(1.0) - ((Rg / r) * (Rg / r)))));
			float horizAlpha = acos(horizMu);
			float horizMuMin = cos((horizAlpha + eps));
			float horizMuMax = cos((horizAlpha - eps));
			return mix(float3(0.0).xxx, transmittance(r, mu), smoothstep(horizMuMin, horizMuMax, mu));
		};
		
		float3 transmittance(float r, float mu, float3 v, float3 x0)
		{
			float3 result;
			float r1 = length(x0);
			float mu1 = (dot(x0, v) / r);
			if ((mu > (float)(0.0)))
			{
				(result = min((transmittance(r, mu) / transmittance(r1, mu1)), 1.0));
			}
			else
			{
				(result = min((transmittance(r1, (-mu1)) / transmittance(r, (-mu))), 1.0));
			}
			return result;
		};
		
		float opticalDepth(float H, float r, float mu, float d)
		{
			float a = sqrt((((float)(0.5) / H) * r));
			float2 a01 = ((float2)(a) * float2(mu, (mu + (d / r))));
			float2 a01s = sign(a01);
			float2 a01sq = (a01 * a01);
			float x = ((((a01s).y > (a01s).x))?(exp((a01sq).x)):(0.0));
			float2 y = ((a01s / (((float2)(2.31929994) * abs(a01)) + sqrt((((float2)(1.52) * a01sq) + (float2)(4.0))))) * float2(1.0, exp((((-d) / H) * ((d / ((float)(2.0) * r)) + mu)))));
			return ((sqrt((((float)(6.28310012) * H) * r)) * exp(((Rg - r) / H))) * (x + dot(y, float2(1.0, (-1.0)))));
		};
		
		float3 analyticTransmittance(float r, float mu, float d)
		{
			return exp((((-betaR) * (float3)(opticalDepth(HR, r, mu, d))) - (betaMEx * (float3)(opticalDepth(HM, r, mu, d)))));
		};
		
		float3 transmittance(float r, float mu, float d)
		{
			float3 result;
			float r1 = sqrt((((r * r) + (d * d)) + ((((float)(2.0) * r) * mu) * d)));
			float mu1 = (((r * mu) + d) / r1);
			if ((mu > (float)(0.0)))
			{
				(result = min((transmittance(r, mu) / transmittance(r1, mu1)), 1.0));
			}
			else
			{
				(result = min((transmittance(r1, (-mu1)) / transmittance(r, (-mu))), 1.0));
			}
			return result;
		};
		
		float3 irradiance(texture2d<float> src, float r, float muS)
		{
			float2 uv = getIrradianceUV(r, muS);
#ifdef USE_SAMPLELEVEL
			return src.sample(g_LinearClamp, uv, level(0)).rgb;
#else
			return src.sample(g_LinearClamp, uv).rgb;
#endif
			
		};
		
		float phaseFunctionR(float mu)
		{
			return (((const float)(3.0) / ((const float)(16.0) * M_PI)) * ((float)(1.0) + (mu * mu)));
		};
		
		float phaseFunctionM(float mu)
		{
			return ((((((const float)((1.5 * 1.0)) / ((const float)(4.0) * M_PI)) * ((const float)(1.0) - (mieG * mieG))) * pow(abs((((const float)(1.0) + (mieG * mieG)) - (((const float)(2.0) * mieG) * mu))), (-1.5))) * ((float)(1.0) + (mu * mu))) / ((const float)(2.0) + (mieG * mieG)));
		};
	
		float3 getMie(float4 rayMie)
		{
			return ((((rayMie).rgb * (float3)((rayMie).w)) / (float3)(max((rayMie).r, 0.00010000000))) * ((float3)((betaR).r) / betaR));
		};
	
		float3 inscatter( float3 x, float t, float3 v, float3 s, float r, float mu, float3 attenuation )
		{
			(attenuation = float3(0.0, 0.0, 0.0));
			float3 result;
			(r = length(x));
			(mu = (dot(x, v) / r));
			float d = (((-r) * mu) - sqrt((((r * r) * ((mu * mu) - (float)(1.0))) + (Rt * Rt))));
			if ((d > (float)(0.0)))
			{
				(x += ((float3)(d) * v));
				(t -= d);
				(mu = (((r * mu) + d) / Rt));
				(r = Rt);
			}
			if ((r <= Rt))
			{
				float nu = dot(v, s);
				float muS = (dot(x, s) / r);
				float phaseR = phaseFunctionR(nu);
				float phaseM = phaseFunctionM(nu);
				float4 inscatter = max(texture4D(InscatterTexture, r, mu, muS, nu), 0.0);
				if ((t > (float)(0.0)))
				{
					float3 x0 = (x + ((float3)(t) * v));
					float r0 = length(x0);
					float rMu0 = dot(x0, v);
					float mu0 = (rMu0 / r0);
					float muS0 = (dot(x0, s) / r0);
#ifdef FIX
					(attenuation = analyticTransmittance(r, mu, t));
#else
					(attenuation = transmittance(r, mu, v, x0));
#endif
					
					if ((r0 > (Rg + (const float)(0.010000000))))
					{
						(inscatter = max((inscatter - (attenuation.rgbr * texture4D(InscatterTexture, r0, mu0, muS0, nu))), 0.0));
#ifdef FIX
#ifdef IGOR_FIX_MU0
						const float EPS = 0.006;
						float muHoriz = (-0.0022);
						float muHoriz1 = (-sqrt(((float)(1.0) - ((Rg / r) * (Rg / r)))));
						if ((abs((mu - muHoriz1)) < EPS))
						{
							(muHoriz = muHoriz1);
						}
#else
						const float EPS = 0.0040000000;
						float muHoriz = (-sqrt(((float)(1.0) - ((Rg / r) * (Rg / r)))));
#endif
						
						if ((abs((mu - muHoriz)) < EPS))
						{
							float a = (((mu - muHoriz) + EPS) / ((const float)(2.0) * EPS));
							(mu = (muHoriz - EPS));
							(r0 = sqrt((((r * r) + (t * t)) + ((((float)(2.0) * r) * t) * mu))));
							(mu0 = (((r * mu) + t) / r0));
							float4 inScatter0 = texture4D(InscatterTexture, r, mu, muS, nu);
							float4 inScatter1 = texture4D(InscatterTexture, r0, mu0, muS0, nu);
							float4 inScatterA = max((inScatter0 - (attenuation.rgbr * inScatter1)), 0.0);
							(mu = (muHoriz + EPS));
							(r0 = sqrt((((r * r) + (t * t)) + ((((float)(2.0) * r) * t) * mu))));
							(mu0 = (((r * mu) + t) / r0));
							(inScatter0 = texture4D(InscatterTexture, r, mu, muS, nu));
							(inScatter1 = texture4D(InscatterTexture, r0, mu0, muS0, nu));
							float4 inScatterB = max((inScatter0 - (attenuation.rgbr * inScatter1)), 0.0);
							(inscatter = (float4)(mix(inScatterA, inScatterB, a)));
						}
#endif
						
					}
				}
#ifdef FIX
				((inscatter).w *= smoothstep(0.0, 0.020000000, muS));
#endif
				
				(result = max((((inscatter).rgb * (float3)(phaseR)) + (getMie(inscatter) * (float3)(phaseM))), 0.0));
			}
			else
			{
				(result = (float3)(float3(0.0).xxx));
			}
#ifdef IGOR_FIX_MOIRE
			float fixFactor = dot(result, float3(0.3, 0.3, 0.3));
			(fixFactor = smoothstep(0.0006, 0.0020000000, fixFactor));
			return (result * (float3)((ISun * fixFactor)));
#else
			return (result * (float3)(ISun));
#endif
			
		};
	
	/*
		float3 groundColor(float3 x, float t, float3 v, float3 s, float r, float mu, float3 attenuation, float4 OriginalImage, bool bSky = false)
		{
			float3 result;
			float4 reflectance;
			((reflectance).xyz = (OriginalImage).xyz);
			float SdotN = (OriginalImage).w;
			((reflectance).w = (float)(0));
			if ((bSky || (t > (float)(0.0))))
			{
				float3 x0 = (x + ((float3)(t) * v));
				float r0 = length(x0);
				float3 n = (x0 / (float3)(r0));
				float muS = dot(n, s);
				float3 sunLight = transmittanceWithShadowSmooth(r0, muS);
				float3 groundSkyLight = irradiance(IrradianceTexture, r0, muS);
				float3 groundColor;
				if (bSky)
				{
					(groundColor = (((((reflectance).rgb * ((float3)(((RenderSkyUniformBuffer.LightIntensity).x * SdotN)) * sunLight)) + ((float3)((RenderSkyUniformBuffer.LightIntensity).y) * groundSkyLight)) * (float3)(ISun)) / (float3)(M_PI)));
				}
				else
				{
					(groundColor = ((((reflectance).rgb * (((float3)(SdotN) * sunLight) + groundSkyLight)) * (float3)(ISun)) / (float3)(M_PI)));
				}
				(result = (attenuation * groundColor));
			}
			else
			{
				(result = float3(0.0).xxx);
			}
			return result;
		};
	 */
		
		float3 sunColor(float3 x, float t, float3 v, float3 s, float r, float mu)
		{
			if ((t > (float)(0.0)))
			{
				return float3(0.0).xxx;
			}
			else
			{
				float3 _transmittance = (((r <= Rt))?(transmittance(r, mu)):(float3(1.0).xxx));
				float isun = (smoothstep(cos((((const float)(2.5) * M_PI) / (const float)(180.0))), cos((((const float)(2.0) * M_PI) / (const float)(180.0))), dot(v, s)) * ISun);
				return (_transmittance * (float3)(isun));
			}
		};
		
		float3 HDR_NORM(float3 L)
		{
			return (L / (float3)(((const float)((0.3 * 100.0)) / M_PI)));
		};
		
		float3 HDR(float3 L)
		{
			(L = (L * (float3)((RenderSkyUniformBuffer.LightDirection).w)));
			float3 rgb = L;
			const float fWhiteIntensity = 1.70000008;
			const float fWhiteIntensitySQR = (fWhiteIntensity * fWhiteIntensity);
			const float intensity = dot(rgb, float3(1.0, 1.0, 1.0));
			return (((rgb * (float3)((1.0 + (intensity / fWhiteIntensitySQR)))) / (float3)((intensity + (const float)(1))))).xyz;
			((L).r = ((((L).r < (float)(1.41300000)))?(pow(((L).r * (float)(0.38317)), (1.0 / 2.20000004))):(((float)(1.0) - exp((-(L).r))))));
			((L).g = ((((L).g < (float)(1.41300000)))?(pow(((L).g * (float)(0.38317)), (1.0 / 2.20000004))):(((float)(1.0) - exp((-(L).g))))));
			((L).b = ((((L).b < (float)(1.41300000)))?(pow(((L).b * (float)(0.38317)), (1.0 / 2.20000004))):(((float)(1.0) - exp((-(L).b))))));
			return L;
		};
		
		float3 RGBtoXYZ(float3 RGB)
		{
			const float3x3 RGB2XYZ = { 0.5141363, 0.32387858, 0.16036376, 0.265068, 0.6702342, 0.06409157, 0.0241188, 0.1228178, 0.8444266 };
			return (((RGB2XYZ)*(RGB)) + float3(1.000000e-06, 1.000000e-06, 1.000000e-06));
		};
		
		float3 XYZtoYxy(float3 XYZ)
		{
			float3 Yxy;
			((Yxy).r = (XYZ).g);
			float temp = dot(float3(1.0, 1.0, 1.0), (XYZ).rgb);
			((Yxy).gb = ((XYZ).rg / (float2)(temp)));
			return Yxy;
		};
		
		float3 XYZtoRGB(float3 XYZ)
		{
			const float3x3 XYZ2RGB = { 2.5650, (-1.16649997), (-0.3986), (-1.021700024), 1.9777000, 0.0439, 0.0753, (-0.2543), 1.18920004 };
			return ((XYZ2RGB)*(XYZ));
		};
		
		float3 YxytoXYZ(float3 Yxy)
		{
			float3 XYZ;
			((XYZ).r = (((Yxy).r * (Yxy).g) / (Yxy).b));
			((XYZ).g = (Yxy).r);
			((XYZ).b = (((Yxy).r * (((float)(1) - (Yxy).g) - (Yxy).b)) / (Yxy).b));
			return XYZ;
		};
		
		float3 YxytoRGB(float3 Yxy)
		{
			float3 XYZ;
			float3 RGB;
			(XYZ = YxytoXYZ(Yxy));
			(RGB = XYZtoRGB(XYZ));
			return RGB;
		};
		
		float3 RGBtoYxy(float3 RGB)
		{
			float3 XYZ;
			float3 Yxy;
			(XYZ = RGBtoXYZ(RGB));
			(Yxy = XYZtoYxy(XYZ));
			return Yxy;
		};
		
		float3 ColorSaturate(float3 L, float factor)
		{
			float3 LuminanceWeights = float3(0.299, 0.587, 0.114);
			float luminance = dot(L, LuminanceWeights);
			return mix(luminance, L, factor);
		};
		
		float3 ColorContrast(float3 color, float tConstrast)
		{
			(color = saturate(color));
			return (color - ((((float3)(tConstrast) * (color - (float3)(1.0))) * color) * (color - (float3)(0.5))));
		};
	
    struct PsIn
    {
        float4 position [[position]];
        float3 texCoord;
        float2 ScreenCoord;
    };
    float DepthLinearization(float depth)
    {
        return (((float)(2.0) * (RenderSkyUniformBuffer.QNNear).z) / (((RenderSkyUniformBuffer.QNNear).w + (RenderSkyUniformBuffer.QNNear).z) - (depth * ((RenderSkyUniformBuffer.QNNear).w - (RenderSkyUniformBuffer.QNNear).z))));
    };
    float4 main(PsIn In)
    {
        uint3 LoadCoord = uint3(In.position.x, In.position.y, 0);
        float4 OriginalImage = SceneColorTexture.read(LoadCoord.xy);
		float3 ReduceOriginalImage = ((OriginalImage).rgb * (float3)(0.3));
		
		float3 _groundColor = float3(0, 0, 0);
		float3 _spaceColor = float3(0, 0, 0);
		float3 inscatterColor = float3(0, 0, 0);

		if(RenderSkyUniformBuffer.LightDirection.y < -0.3f)
		{
			(_groundColor = (((ReduceOriginalImage).xyz * (float3)(ISun)) / (float3)(M_PI)));
			return float4(((HDR_NORM(_groundColor) + HDR((_spaceColor + inscatterColor))) + (OriginalImage).rgb), 0.0);
		}

        float depth = Depth.read(LoadCoord.xy).x;
       
        float fixedDepth = (((-(RenderSkyUniformBuffer.QNNear).y) * (RenderSkyUniformBuffer.QNNear).x) / (depth - (RenderSkyUniformBuffer.QNNear).x));
        float3 ray = (In).texCoord;
        float3 x = (RenderSkyUniformBuffer.CameraPosition).xyz;
        float3 v = normalize(ray);
        float r = length(x);
        float mu = (dot(x, v) / r);
        float VoRay = dot(v, ray);
        float t = (((depth < 1.0))?((VoRay * fixedDepth)):(0.0));
        float3 attenuation = float3(0,0,0);
        inscatterColor = inscatter(x, t, v, (RenderSkyUniformBuffer.LightDirection).xyz, r, mu, attenuation);
        
        //float3 _sunColor = sunColor(x, t, v, (RenderSkyUniformBuffer.LightDirection).xyz, r, mu);
        
        if ((t <= (float)(0.0)))
        {
            float3 _transmittance = (((r <= Rt))?(transmittance(r, mu)):(float3(1.0).xxx));
            (_spaceColor = ((ReduceOriginalImage).xyz * _transmittance));
        }
        else
        {
            (_groundColor = (((ReduceOriginalImage).xyz * (float3)(ISun)) / (float3)(M_PI)));
        }
        (inscatterColor *= (float3)(((RenderSkyUniformBuffer.InScatterParams).x + (RenderSkyUniformBuffer.InScatterParams).y)));
        return float4(((HDR_NORM(_groundColor) + HDR((_spaceColor + inscatterColor))) + (OriginalImage).rgb), 0.0);
    };

    Fragment_Shader(
					constant Uniforms_RenderSkyUniformBuffer & RenderSkyUniformBuffer,
					texture2d<float> SceneColorTexture,
					texture2d<float> Depth,
					texture2d<float> TransmittanceTexture,
					//texture2d<float> IrradianceTexture,
					texture3d<float> InscatterTexture,
					sampler g_LinearClamp) :
					RenderSkyUniformBuffer(RenderSkyUniformBuffer),
					SceneColorTexture(SceneColorTexture),
					Depth(Depth),
					TransmittanceTexture(TransmittanceTexture),
					//IrradianceTexture(IrradianceTexture),
					InscatterTexture(InscatterTexture),
					g_LinearClamp(g_LinearClamp)
					{}
};

struct ArgsData
{
    texture2d<float> SceneColorTexture;
    texture2d<float> Depth;
    texture2d<float> TransmittanceTexture;
    texture3d<float> InscatterTexture;
    sampler g_LinearClamp;
};

struct ArgsPerFrame
{
	constant Fragment_Shader::Uniforms_RenderSkyUniformBuffer & RenderSkyUniformBuffer;
};

fragment float4 stageMain(
    Fragment_Shader::PsIn In [[stage_in]],
	constant ArgsData& argBufferStatic [[buffer(UPDATE_FREQ_NONE)]],
    constant ArgsPerFrame& argBufferPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    Fragment_Shader::PsIn In0;
    In0.position = float4(In.position.xyz, 1.0 / In.position.w);
    In0.texCoord = In.texCoord;
    In0.ScreenCoord = In.ScreenCoord;
    Fragment_Shader main(
    argBufferPerFrame.RenderSkyUniformBuffer,
    argBufferStatic.SceneColorTexture,
    argBufferStatic.Depth,
    argBufferStatic.TransmittanceTexture,
    //argBufferStatic.IrradianceTexture,
    argBufferStatic.InscatterTexture,
    argBufferStatic.g_LinearClamp);
    return main.main(In0);
}
