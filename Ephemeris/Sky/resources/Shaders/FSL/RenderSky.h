/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#ifndef RENDER_SKY_H
#define RENDER_SKY_H

#include "SkyCommon.h"

// ----------------------------------------------------------------------------
// PHYSICAL MODEL PARAMETERS
// ----------------------------------------------------------------------------
//STATIC const float AVERAGE_GROUND_REFLECTANCE = 0.1f;

// Rayleigh
STATIC const float HR = 8.0f;
STATIC const float3 betaR = float3(5.8e-3, 1.35e-2, 3.31e-2);

// Mie
// DEFAULT
/*
STATIC const float HM = 1.2f;
STATIC const float3 betaMSca = f3(4e-3);
STATIC const float3 betaMEx  = betaMSca / f3(0.9f);
STATIC const float mieG = 0.8f;
*/

// CLEAR SKY
STATIC const float HM = 1.2f;
STATIC const float3 betaMSca = f3(20e-3);
STATIC const float3 betaMEx  = betaMSca / f3(0.9f);
STATIC const float mieG = 0.76f;

// PARTLY CLOUDY
/*
STATIC const float HM = 3.0f;
STATIC const float3 betaMSca = f3(3e-3);
STATIC const float3 betaMEx  = betaMSca / f3(0.9f);
STATIC const float mieG = 0.65f;
*/

// ----------------------------------------------------------------------------
// NUMERICAL INTEGRATION PARAMETERS
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// PARAMETERIZATION OPTIONS
// ----------------------------------------------------------------------------

#define TRANSMITTANCE_NON_LINEAR
#define INSCATTER_NON_LINEAR
#define USE_SAMPLELEVEL

#define FIX
//	Hides moire effect using clamping low inscattering values
#define IGOR_FIX_MOIRE
//	Hides bright horizontal line on low heights and low sun
#define IGOR_FIX_MU0
//	Disables atmosphere/space camera transition
//	Uncomment this to increase performance or handle transition
//	on CPU

float2 getTransmittanceUV(float r, float mu)
{
	float uR, uMu;
#ifdef TRANSMITTANCE_NON_LINEAR
	uR = sqrt((r - Rg) / (Rt - Rg));
	uMu = atan((mu + 0.15f) / (1.0f + 0.15f) * tan(1.5f)) / 1.5f;
#else
	uR = (r - Rg) / (Rt - Rg);
	uMu = (mu + 0.15f) / (1.0f + 0.15f);
#endif

	return float2(uMu, uR);
}

void getTransmittanceRMu(float2 ScreenCoord, out(float) r, out(float) muS) 
{
	r = ScreenCoord.y;
	muS = ScreenCoord.x;
#ifdef TRANSMITTANCE_NON_LINEAR
	r = Rg + (r * r) * (Rt - Rg);
	muS = -0.15f + tan(1.5f * muS) / tan(1.5f) * (1.0f + 0.15f);
#else
	r = Rg + r * (Rt - Rg);
	muS = -0.15f + muS * (1.0f + 0.15f);
#endif
}

float2 getIrradianceUV(float r, float muS) 
{
	float uR = (r - Rg) / (Rt - Rg);
	float uMuS = (muS + 0.2f) / (1.0f + 0.2f);

	return float2(uMuS, uR);
}

void getIrradianceRMuS(float2 ScreenCoord, out(float) r, out(float) muS) 
{
	//	Igor: current mapping differs from the original a bit.
	//	However, this makes difference only on the extreme
	//	end of the function definition area. I dond't experience any problems with that.
	//	Use SV_Position, if you want the original mapping, but I experienced some
	//	significant problems with it in some shaders.
	r = Rg + ScreenCoord.y * (Rt - Rg);
	muS = -0.2f + ScreenCoord.x * (1.0f + 0.2f);
}

//	Igor: contrast function
float Contrast(float Input, float ContrastPower)
{
	//piecewise contrast function
	bool IsAboveHalf = Input > 0.5f;
	float ToRaise = saturate(2 * (IsAboveHalf ? 1 - Input : Input));
	float Output = 0.5f * pow(ToRaise, ContrastPower);
	Output = IsAboveHalf ? 1.0f - Output : Output;

	return Output;
}

//	Igor: use contrast function for interpolation along nu! 
float4 texture4D(Tex3D(float4) table, float r, float mu, float muS, float nu)
{
	float H = sqrt(Rt * Rt - Rg * Rg);
	float rho = sqrt(r * r - Rg * Rg);
#ifdef INSCATTER_NON_LINEAR
	float rmu = r * mu;
	float delta = rmu * rmu - r * r + Rg * Rg;
	float4 cst = rmu < 0.0f && delta > 0.0f ? float4(1.0f, 0.0f, 0.0f, 0.5f - 0.5f / float(RES_MU)) : float4(-1.0f, H * H, H, 0.5f + 0.5f / float(RES_MU));
	float uR = 0.5f / float(RES_R) + rho / H * (1.0f - 1.0f / float(RES_R));
	float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5f - 1.0f / float(RES_MU));
	// paper formula
	//float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S));
	// better formula
	float uMuS = 0.5f / float(RES_MU_S) + (atan(max(muS, -0.1975f) * tan(1.26 * 1.1f)) / 1.1f + (1.0f - 0.26f)) * 0.5f * (1.0f - 1.0f / float(RES_MU_S));
#else
	float uR = 0.5f / float(RES_R) + rho / H * (1.0f - 1.0f / float(RES_R));
	float uMu = 0.5 / float(RES_MU) + (mu + 1.0f) / 2.0f * (1.0f - 1.0f / float(RES_MU));
	float uMuS = 0.5 / float(RES_MU_S) + max(muS + 0.2f, 0.0f) / 1.2f * (1.0f - 1.0f / float(RES_MU_S));
#endif
	float lerp = (nu + 1.0f) / 2.0f * (float(RES_NU) - 1.0f);
	float uNu = floor(lerp);
	lerp = lerp - uNu;
	//	Igor: use contrast function here instead of linear interpolation
	lerp = Contrast(lerp, 3);
	//lerp = 0;
#ifdef USE_SAMPLELEVEL
	return SampleLvlTex3D(table, Get(g_LinearClamp), float3((uNu + uMuS) / float(RES_NU), uMu, uR),        0) * (1.0f - lerp) +
           SampleLvlTex3D(table, Get(g_LinearClamp), float3((uNu + uMuS + 1.0f) / float(RES_NU), uMu, uR), 0) * lerp;
#else	//	USE_SAMPLELEVEL
	return SampleTex3D(table, Get(g_LinearClamp), float3((uNu + uMuS) / float(RES_NU), uMu, uR))        * (1.0f - lerp) +
           SampleTex3D(table, Get(g_LinearClamp), float3((uNu + uMuS + 1.0f) / float(RES_NU), uMu, uR)) * lerp;
#endif	//	USE_SAMPLELEVEL
}

//	Igor: this is an exact maths for GL's mod
float mod0(float x, float y)
{
	return x - y * floor(x / y);
}

void getMuMuSNu(float2 ScreenCoord, float r, float4 dhdH, out(float) mu, out(float) muS, out(float) nu) 
{
#ifdef USE_GL_FRAGCOORD
	float x = ScreenCoord.x - 0.5f;
	float y = ScreenCoord.y - 0.5f;
#else
	//	Igor: this is an exact emulation of the SV_Position behavior.
	//	This appeared to be more consistent, than using SV_Position.
	float x = (ScreenCoord.x * (RES_MU_S * RES_NU)) - 0.5f;
	float y = (ScreenCoord.y * RES_MU) - 0.5f;
#endif // USE_GL_FRAGCOORD

#ifdef INSCATTER_NON_LINEAR
	if (y < float(RES_MU) / 2.0f) 
	{
		float d = 1.0f - y / (float(RES_MU) / 2.0f - 1.0f);
		d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999f);
		mu = (Rg * Rg - r * r - d * d) / (2.0f * r * d);
		mu = min(mu, -sqrt(1.0f - (Rg / r) * (Rg / r)) - 0.001f);
	}
	else 
	{
		float d = (y - float(RES_MU) / 2.0f) / (float(RES_MU) / 2.0f - 1.0f);
		d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999f);
		mu = (Rt * Rt - r * r - d * d) / (2.0f * r * d);
	}

	muS = mod0(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0f);
	// paper formula
	//muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0;
	// better formula
	muS = tan((2.0f * muS - 1.0f + 0.26f) * 1.1f) / tan(1.26f * 1.1f);
	nu = -1.0f + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0f) * 2.0f;
#else
	mu = -1.0f + 2.0f * y / (float(RES_MU) - 1.0f);
	muS = mod0(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0f);
	muS = -0.2f + muS * 1.2f;
	nu = -1.0f + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0f) * 2.0f;
#endif
}

// ----------------------------------------------------------------------------
// UTILITY FUNCTIONS
// ----------------------------------------------------------------------------

// nearest intersection of ray r,mu with ground or top atmosphere boundary
// mu=cos(ray zenith angle at ray origin)
float limit(float r, float mu) 
{
	float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0f) + RL * RL);

	float delta2 = r * r * (mu * mu - 1.0f) + Rg * Rg;
	if (delta2 >= 0.0f) 
	{
		float din = -r * mu - sqrt(delta2);
		if (din >= 0.0f) 
		{
			dout = min(dout, din);
		}
	}

	return dout;
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), intersections with ground ignored
float3 transmittance(float r, float mu) 
{
	float2 uv = getTransmittanceUV(r, mu);
#ifdef USE_SAMPLELEVEL
	return SampleLvlTex2D(Get(TransmittanceTexture), Get(g_LinearClamp), uv, 0).rgb;
#else	//	USE_SAMPLELEVEL
	return SampleTex2D(Get(TransmittanceTexture), Get(g_LinearClamp), uv).rgb;
#endif	//	USE_SAMPLELEVEL
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), or zero if ray intersects ground
float3 transmittanceWithShadow(float r, float mu) 
{
	return mu < -sqrt(1.0f - (Rg / r) * (Rg / r)) ? f3(0.0f) : transmittance(r, mu);
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), or zero if ray intersects ground
float3 transmittanceWithShadowSmooth(float r, float mu) 
{
	//	TODO: check if it is reasonably fast
	//	TODO: check if it is mathematically correct
	//    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? (0.0).xxx : transmittance(r, mu);

	float eps = 0.5f * M_PI / 180.0f;
	float horizMu = -sqrt(1.0f - (Rg / r) * (Rg / r));
	float horizAlpha = acos(horizMu);
	float horizMuMin = cos(horizAlpha + eps);
	float horizMuMax = cos(horizAlpha - eps);

	return lerp(f3(0.0f), transmittance(r, mu), smoothstep(horizMuMin, horizMuMax, mu));
}

// transmittance(=transparency) of atmosphere between x and x0
// assume segment x,x0 not intersecting ground
// r=||x||, mu=cos(zenith angle of [x,x0) ray at x), v=unit direction vector of [x,x0) ray
float3 transmittance1(float r, float mu, float3 v, float3 x0) 
{
	float r1 = length(x0);
	float mu1 = dot(x0, v) / r;

	float3 tr1, tr2;
	
	if (mu > 0.0f)
	{
		tr1 = transmittance(r, mu);
		tr2 = transmittance(r1, mu1);
	}
	else
	{
		tr1 = transmittance(r1, -mu1);
		tr2 = transmittance(r, -mu);
	}
	
	return min(tr1 / tr2, 1.0f);
}

// optical depth for ray (r,mu) of length d, using analytic formula
// (mu=cos(view zenith angle)), intersections with ground ignored
// H=height scale of exponential density function
float opticalDepth(float H, float r, float mu, float d) 
{
	float a = sqrt((0.5f / H) * r);
	float2 a01 = a * float2(mu, mu + d / r);
	float2 a01s = float2(sign(a01));
	float2 a01sq = a01 * a01;
	float x = a01s.y > a01s.x ? exp(a01sq.x) : 0.0f;
	float2 y = a01s / (2.3193f * abs(a01) + sqrt(1.52f * a01sq + 4.0f)) * float2(1.0f, exp(-d / H * (d / (2.0f * r) + mu)));

	return sqrt((6.2831f * H) * r) * exp((Rg - r) / H) * (x + dot(y, float2(1.0f, -1.0f)));
}

// transmittance(=transparency) of atmosphere for ray (r,mu) of length d
// (mu=cos(view zenith angle)), intersections with ground ignored
// uses analytic formula instead of transmittance texture
float3 analyticTransmittance(float r, float mu, float d) 
{
	return exp(-betaR * opticalDepth(HR, r, mu, d) - betaMEx * opticalDepth(HM, r, mu, d));
}

// transmittance(=transparency) of atmosphere between x and x0
// assume segment x,x0 not intersecting ground
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x)
float3 transmittance2(float r, float mu, float d) 
{
	float r1 = sqrt(r * r + d * d + 2.0f * r * mu * d);
	float mu1 = (r * mu + d) / r1;

	float3 tr1, tr2;
	
	if (mu > 0.0f)
	{
		tr1 = transmittance(r, mu);
		tr2 = transmittance(r1, mu1);
	}
	else
	{
		tr1 = transmittance(r1, -mu1);
		tr2 = transmittance(r, -mu);
	}

	return min(tr1 / tr2, 1.0f);
}

float3 irradiance(Tex2D(float4) src, float r, float muS) 
{
	float2 uv = getIrradianceUV(r, muS);
#ifdef USE_SAMPLELEVEL
	return SampleLvlTex2D(src, Get(g_LinearClamp), uv, 0).rgb;
#else	//	USE_SAMPLELEVEL
	return SampleTex2D(src, Get(g_LinearClamp), uv).rgb;
#endif	//	USE_SAMPLELEVEL
}

// Rayleigh phase function
float phaseFunctionR(float mu) 
{
	return (3.0f / (16.0f * M_PI)) * (1.0f + mu * mu);
}

// Mie phase function
float phaseFunctionM(float mu) 
{
	return 1.5f * 1.0f / (4.0f * M_PI) * (1.0f - mieG * mieG) * pow(abs(1.0f + (mieG*mieG) - 2.0f * mieG * mu), -1.5f) * (1.0f + mu * mu) / (2.0f + mieG * mieG);
}

// approximated single Mie scattering
float3 getMie(float4 rayMie) // rayMie.rgb=C*, rayMie.w=Cm,r
{ 
	return rayMie.rgb * rayMie.w / max(rayMie.r, 1e-4) * (betaR.r / betaR);
}

//inscattered light along ray x+tv, when sun in direction s (=S[L]-T(x,x0)S[L]|x0)
float3 inscatter(inout(float3) x, inout(float) t, float3 v, float3 s, out(float) r, out(float) mu, out(float3) attenuation)
{
	float3 result = f3(0.0f);

	attenuation = f3(0.0f);
	r = length(x);
	mu = dot(x, v) / r;

	float d = -r * mu - sqrt(r * r * (mu * mu - 1.0f) + Rt * Rt);

	if (d > 0.0f) // if x in space and ray intersects atmosphere
	{ 
		// move x to nearest intersection of ray with top atmosphere boundary
		x += d * v;
		t -= d;
		mu = (r * mu + d) / Rt;
		r = Rt;
	}

	if (r <= Rt) // if ray intersects atmosphere
	{
		float nu = dot(v, s);
		float muS = dot(x, s) / r;
		float phaseR = phaseFunctionR(nu);
		float phaseM = phaseFunctionM(nu);
		float4 inscatter_ = max(texture4D(Get(InscatterTexture), r, mu, muS, nu), 0.0f);

		if (t > 0.0f) 
		{
			float3 x0 = x + t * v;
			float r0 = length(x0);
			float rMu0 = dot(x0, v);
			float mu0 = rMu0 / r0;
			float muS0 = dot(x0, s) / r0;
#ifdef FIX
			// avoids imprecision problems in transmittance computations based on textures
			attenuation = analyticTransmittance(r, mu, t);
#else
			attenuation = transmittance1(r, mu, v, x0);
#endif
			if (r0 > Rg + 0.01f)
			{
				// computes S[L]-T(x,x0)S[L]|x0
				inscatter_ = max(inscatter_ - attenuation.rgbr * texture4D(Get(InscatterTexture), r0, mu0, muS0, nu), 0.0f);
#ifdef FIX
				// avoids imprecision problems near horizon by interpolating between two points above and below horizon
#ifdef IGOR_FIX_MU0
				//	const float EPS = 0.004;	//	OK
				const float EPS = 0.006f;	//	Better
				float muHoriz = -0.0022f;
				float muHoriz1 = -sqrt(1.0f - (Rg / r) * (Rg / r));
				if (abs(mu - muHoriz1) < EPS) muHoriz = muHoriz1;
#else	//	IGOR_FIX_MU0
				const float EPS = 0.004f;
				float muHoriz = -sqrt(1.0f - (Rg / r) * (Rg / r));
#endif	//	IGOR_FIX_MU0

				if (abs(mu - muHoriz) < EPS) 
				{
					//if (0) {
					float a = ((mu - muHoriz) + EPS) / (2.0f * EPS);

					mu = muHoriz - EPS;
					r0 = sqrt(r * r + t * t + 2.0f * r * t * mu);
					mu0 = (r * mu + t) / r0;
					float4 inScatter0 = texture4D(Get(InscatterTexture), r, mu, muS, nu);
					float4 inScatter1 = texture4D(Get(InscatterTexture), r0, mu0, muS0, nu);
					float4 inScatterA = max(inScatter0 - attenuation.rgbr * inScatter1, 0.0f);

					mu = muHoriz + EPS;
					r0 = sqrt(r * r + t * t + 2.0f * r * t * mu);
					mu0 = (r * mu + t) / r0;
					inScatter0 = texture4D(Get(InscatterTexture), r, mu, muS, nu);
					inScatter1 = texture4D(Get(InscatterTexture), r0, mu0, muS0, nu);
					float4 inScatterB = max(inScatter0 - attenuation.rgbr * inScatter1, 0.0f);

					inscatter_ = lerp(inScatterA, inScatterB, a);
				}
#endif
			}
		}

#ifdef FIX
		// avoids imprecision problems in Mie scattering when sun is below horizon
		inscatter_.w *= smoothstep(0.00f, 0.02f, muS);
#endif
		result = max(inscatter_.rgb * phaseR + getMie(inscatter_) * phaseM, 0.0f);
	}

#ifdef IGOR_FIX_MOIRE
	//	Igor: fix inscatter imprecision
	float fixFactor = dot(result, f3(0.3f));
	fixFactor = smoothstep(0.0007f, 0.002f, fixFactor);
	//	fixFactor = 1;
	return result * (ISun*fixFactor);
#else	//	IGOR_FIX_MOIRE
	return result * ISun;
#endif	//	IGOR_FIX_MOIRE
}

//ground radiance at end of ray x+tv, when sun in direction s
//attenuated bewteen ground and viewer (=R[L0]+R[L*])
float3 groundColor(float3 x, float t, float3 v, float3 s, float r, float mu, float3 attenuation, float4 OriginalImage, bool bSky)
{
	float3 result = f3(0.0f);

	float4 reflectance;
	reflectance.xyz = OriginalImage.xyz;
	reflectance.w = 0.0f;

	float SdotN = OriginalImage.w;

	if (bSky || (t > 0.0f)) // if ray hits ground surface
	{ 
		// ground reflectance at end of ray, x0
		float3 x0 = x + t * v;
		float r0 = length(x0);
		float3 n = x0 / r0;

		// direct sun light (radiance) reaching x0
		float muS = dot(n, s);
		//float3 sunLight = transmittanceWithShadow(r0, muS);
		float3 sunLight = transmittanceWithShadowSmooth(r0, muS);

		// precomputed sky light (irradiance) (=E[L*]) at x0
		float3 groundSkyLight = irradiance(Get(IrradianceTexture), r0, muS);

		float3 groundColor_ = bSky
                            ? (reflectance.rgb * (Get(LightIntensity).x * SdotN * sunLight) + Get(LightIntensity).y * groundSkyLight) * ISun / M_PI
                            :  reflectance.rgb * (SdotN * sunLight + groundSkyLight) * ISun / M_PI;

		result = attenuation * groundColor_; //=R[L0]+R[L*]
	}

	return result;
}

// direct sun light for ray x+tv, when sun in direction s (=L0)
float3 sunColor(float3 x, float t, float3 v, float3 s, float r, float mu) 
{
	if (t > 0.0f) 
	{
		return f3(0.0f);
	}
	else 
	{
		float3 _transmittance = r <= Rt ? transmittance(r, mu) : f3(1.0f); // T(x,xo)
		float isun = smoothstep(cos(2.5f * M_PI / 180.0f), cos(2.0f * M_PI / 180.0f), dot(v, s)) * ISun; // Lsun Igor

		//	Real sun size
		//float isun = smoothstep(cos(1.0f/108.f/2), cos(1.0f/118.f/2), dot(v, s)) * ISun; // Lsun Igor
		return _transmittance * isun; // Eq (9)
	}
}

float3 HDR_NORM(float3 L)
{
	//	TODO: Igor: Game post-process HDR is used
	return L / (0.3f * 100.0f / M_PI);
}

float3 HDR(float3 L) 
{
	L = L * Get(lightDirection).w;

	float3 rgb = L;

	const float fWhiteIntensity = 1.7f;
	//const float fWhiteIntensity = 4.;

	const float fWhiteIntensitySQR = fWhiteIntensity * fWhiteIntensity;

	const float intensity = dot(rgb, f3(1.0f));

	//return	(rgb/(rgb + 1)).xyz;

	return	((rgb * (1.0f + intensity / fWhiteIntensitySQR)) / (intensity + 1.0f)).xyz;
}

float3 RGBtoXYZ(float3 RGB)
{
	// RGB -> XYZ conversion
	// http://www.w3.org/Graphics/Color/sRGB
	// The official sRGB to XYZ conversion matrix is (following ITU-R BT.709)
	// 0.4125 0.3576 0.1805
	// 0.2126 0.7152 0.0722
	// 0.0193 0.1192 0.9505	
	const f3x3 RGB2XYZ = make_f3x3_row_elems(
		0.5141364f, 0.3238786f,  0.16036376f,
		0.265068f,  0.67023428f, 0.06409157f,
		0.0241188f, 0.1228178f,  0.84442666f 
	);

	// Make sure none of XYZ can be zero
	// This prevents temp from ever being zero, or Yxy from ever being zero        
	return mul(RGB2XYZ, RGB) + f3(0.000001f);
}

float3 XYZtoYxy(float3 XYZ)
{
	// XYZ -> Yxy conversion
	float3 Yxy;

	Yxy.r = XYZ.g;									// copy luminance Y

	float temp = dot(f3(1.0f), XYZ.rgb);
	// x = X / (X + Y + Z)
	// y = X / (X + Y + Z)	
	Yxy.gb = XYZ.rg / temp;

	return Yxy;
}

float3 XYZtoRGB(float3 XYZ)
{
	// XYZ -> RGB conversion
	// The official XYZ to sRGB conversion matrix is (following ITU-R BT.709)
	//  3.2410 -1.5374 -0.4986
	// -0.9692  1.8760  0.0416
	//  0.0556 -0.2040  1.0570		
	const f3x3 XYZ2RGB = make_f3x3_row_elems(
		 2.5651f, -1.1665f, -0.3986f,
		-1.0217f,  1.9777f,  0.0439f,
		 0.0753f, -0.2543f,  1.1892f 
	);

	return mul(XYZ2RGB, XYZ);
}

float3 YxytoXYZ(float3 Yxy)
{
	float3 XYZ;

	// Yxy -> XYZ conversion
	XYZ.r = Yxy.r * Yxy.g / Yxy.b;                // X = Y * x / y
	XYZ.g = Yxy.r;                                // copy luminance Y
	XYZ.b = Yxy.r * (1.0f - Yxy.g - Yxy.b) / Yxy.b;  // Z = Y * (1-x-y) / y

	return XYZ;
}

float3 YxytoRGB(float3 Yxy)
{
	float3 RGB;

	float3 XYZ;
	XYZ = YxytoXYZ(Yxy);
	RGB = XYZtoRGB(XYZ);

	return RGB;
}

float3 RGBtoYxy(float3 RGB)
{
	float3 Yxy;

	float3 XYZ;
	XYZ = RGBtoXYZ(RGB);
	Yxy = XYZtoYxy(XYZ);

	return Yxy;
}

float3 ColorSaturate(float3 L, float factor)
{
	float3 LuminanceWeights = float3(0.299f, 0.587f, 0.114f);
	float  luminance = dot(L, LuminanceWeights);

	return lerp(f3(luminance), L, factor);
}

float3 ColorContrast(float3 color, float tConstrast)
{
	color = saturate(color);

	return color - tConstrast * (color - 1.0f) * color * (color - 0.5f);
}

#endif // RENDER_SKY_H