/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "SkyCommon.h"

// ----------------------------------------------------------------------------
// PHYSICAL MODEL PARAMETERS
// ----------------------------------------------------------------------------

static const float AVERAGE_GROUND_REFLECTANCE = 0.1f;

// Rayleigh
static const float HR = 8.0f;
static const float3 betaR = float3(5.8e-3, 1.35e-2, 3.31e-2);

// Mie
// DEFAULT
/*
static const float HM = 1.2;
static const float3 betaMSca = (4e-3).xxx;
static const float3 betaMEx = betaMSca / 0.9;
static const float mieG = 0.8;
/**/

// CLEAR SKY
static const float HM = 1.2f;
static const float3 betaMSca = (20e-3).xxx;
static const float3 betaMEx = betaMSca / 0.9f;
static const float mieG = 0.76f;
/**/
// PARTLY CLOUDY
/*
static const float HM = 3.0;
static const float3 betaMSca = (3e-3).xxx;
static const float3 betaMEx = betaMSca / 0.9;
static const float mieG = 0.65;
/**/

// ----------------------------------------------------------------------------
// NUMERICAL INTEGRATION PARAMETERS
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// PARAMETERIZATION OPTIONS
// ----------------------------------------------------------------------------

#define TRANSMITTANCE_NON_LINEAR
#define INSCATTER_NON_LINEAR
#define USE_SAMPLELEVEL

cbuffer RenderSkyUniformBuffer : register(b0, UPDATE_FREQ_PER_FRAME)
{
	float4x4 invView;
	float4x4 invProj;

	float4	LightDirection; // w:exposure
	float4	CameraPosition;
	float4	QNNear;
	float4	InScatterParams;

	float4	LightIntensity;
}


Texture2D SceneColorTexture : register(t0);
Texture2D Depth : register(t1);

Texture2D TransmittanceTexture : register(t2);
Texture2D IrradianceTexture : register(t3);//precomputed skylight irradiance (E table)
Texture3D InscatterTexture : register(t4);//precomputed inscattered light (S table)

SamplerState g_LinearClamp : register(s0);

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
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;
#else
	uR = (r - Rg) / (Rt - Rg);
	uMu = (mu + 0.15) / (1.0 + 0.15);
#endif
	return float2(uMu, uR);
}

void getTransmittanceRMu(float2 ScreenCoord, out float r, out float muS) {
	r = ScreenCoord.y;
	muS = ScreenCoord.x;
#ifdef TRANSMITTANCE_NON_LINEAR
	r = Rg + (r * r) * (Rt - Rg);
	muS = -0.15 + tan(1.5 * muS) / tan(1.5) * (1.0 + 0.15);
#else
	r = Rg + r * (Rt - Rg);
	muS = -0.15 + muS * (1.0 + 0.15);
#endif
}

float2 getIrradianceUV(float r, float muS) {
	float uR = (r - Rg) / (Rt - Rg);
	float uMuS = (muS + 0.2) / (1.0 + 0.2);
	return float2(uMuS, uR);
}

void getIrradianceRMuS(float2 ScreenCoord, out float r, out float muS) {
	//	Igor: current mapping differs from the original a bit.
	//	However, this makes difference only on the extreme
	//	end of the function definition area. I dond't experience any problems with that.
	//	Use SV_Position, if you want the original mapping, but I experienced some
	//	significant problems with it in some shaders.
	r = Rg + ScreenCoord.y * (Rt - Rg);
	muS = -0.2 + ScreenCoord.x * (1.0 + 0.2);
}

//	Igor: contrast function
float Contrast(float Input, float ContrastPower)
{
	//piecewise contrast function
	bool IsAboveHalf = Input > 0.5;
	float ToRaise = saturate(2 * (IsAboveHalf ? 1 - Input : Input));
	float Output = 0.5*pow(ToRaise, ContrastPower);
	Output = IsAboveHalf ? 1 - Output : Output;
	return Output;
}

//	Igor: use contrast function for interpolation along nu! 
float4 texture4D(Texture3D table, float r, float mu, float muS, float nu)
{
	float H = sqrt(Rt * Rt - Rg * Rg);
	float rho = sqrt(r * r - Rg * Rg);
#ifdef INSCATTER_NON_LINEAR
	float rmu = r * mu;
	float delta = rmu * rmu - r * r + Rg * Rg;
	float4 cst = rmu < 0.0 && delta > 0.0 ? float4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : float4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU));
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R));
	float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU));
	// paper formula
	//float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S));
	// better formula
	float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S));
#else
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R));
	float uMu = 0.5 / float(RES_MU) + (mu + 1.0) / 2.0 * (1.0 - 1.0 / float(RES_MU));
	float uMuS = 0.5 / float(RES_MU_S) + max(muS + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(RES_MU_S));
#endif
	float lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0);
	float uNu = floor(lerp);
	lerp = lerp - uNu;
	//	Igor: use contrast function here instead of linear interpolation
	lerp = Contrast(lerp, 3);
	//lerp = 0;
#ifdef	USE_SAMPLELEVEL
	return table.SampleLevel(g_LinearClamp, float3((uNu + uMuS) / float(RES_NU), uMu, uR), 0) * (1.0 - lerp) +
		table.SampleLevel(g_LinearClamp, float3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR), 0) * lerp;
#else	//	USE_SAMPLELEVEL
	return table.Sample(g_LinearClamp, float3((uNu + uMuS) / float(RES_NU), uMu, uR)) * (1.0 - lerp) +
		table.Sample(g_LinearClamp, float3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR)) * lerp;
#endif	//	USE_SAMPLELEVEL
}

//	Igor: this is an exact maths for GL's mod
float mod(float x, float y)
{
	return x - y * floor(x / y);
}

#ifdef	USE_GL_FRAGCOORD
void getMuMuSNu(float2  gl_FragCoord, float r, float4 dhdH, out float mu, out float muS, out float nu) {
	float x = gl_FragCoord.x - 0.5;
	float y = gl_FragCoord.y - 0.5;
#else	//	USE_GL_FRAGCOORD
void getMuMuSNu(float2 ScreenCoord, float r, float4 dhdH, out float mu, out float muS, out float nu) {
	//	Igor: this is an exact emulation of the SV_Position behavior.
	//	This appeared to be more consistent, than using SV_Position.
	float x = (ScreenCoord.x*(RES_MU_S * RES_NU)) - 0.5;
	float y = (ScreenCoord.y*RES_MU) - 0.5;
#endif	//	USE_GL_FRAGCOORD

#ifdef INSCATTER_NON_LINEAR
	if (y < float(RES_MU) / 2.0) {
		float d = 1.0 - y / (float(RES_MU) / 2.0 - 1.0);
		d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999);
		mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d);
		mu = min(mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001);
	}
	else {
		float d = (y - float(RES_MU) / 2.0) / (float(RES_MU) / 2.0 - 1.0);
		d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999);
		mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d);
	}
	muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0);
	// paper formula
	//muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0;
	// better formula
	muS = tan((2.0 * muS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1);
	nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0;
#else
	mu = -1.0 + 2.0 * y / (float(RES_MU) - 1.0);
	muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0);
	muS = -0.2 + muS * 1.2;
	nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0;
#endif
}


// ----------------------------------------------------------------------------
// UTILITY FUNCTIONS
// ----------------------------------------------------------------------------

// nearest intersection of ray r,mu with ground or top atmosphere boundary
// mu=cos(ray zenith angle at ray origin)
float limit(float r, float mu) {
	float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0) + RL * RL);
	float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg;
	if (delta2 >= 0.0) {
		float din = -r * mu - sqrt(delta2);
		if (din >= 0.0) {
			dout = min(dout, din);
		}
	}
	return dout;
}


// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), intersections with ground ignored
float3 transmittance(float r, float mu) {
	float2 uv = getTransmittanceUV(r, mu);
#ifdef	USE_SAMPLELEVEL
	return TransmittanceTexture.SampleLevel(g_LinearClamp, uv, 0).rgb;
#else	//	USE_SAMPLELEVEL
	return TransmittanceTexture.Sample(g_LinearClamp, uv).rgb;
#endif	//	USE_SAMPLELEVEL
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), or zero if ray intersects ground
float3 transmittanceWithShadow(float r, float mu) {
	return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? (0.0).xxx : transmittance(r, mu);
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), or zero if ray intersects ground
float3 transmittanceWithShadowSmooth(float r, float mu) {
	//	TODO: check if it is reasonably fast
	//	TODO: check if it is mathematically correct
	//    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? (0.0).xxx : transmittance(r, mu);

	float eps = 0.5f*M_PI / 180.0f;
	float horizMu = -sqrt(1.0 - (Rg / r) * (Rg / r));
	float horizAlpha = acos(horizMu);
	float horizMuMin = cos(horizAlpha + eps);
	float horizMuMax = cos(horizAlpha - eps);

	return lerp((0.0).xxx, transmittance(r, mu),
		smoothstep(horizMuMin, horizMuMax, mu));
}

// transmittance(=transparency) of atmosphere between x and x0
// assume segment x,x0 not intersecting ground
// r=||x||, mu=cos(zenith angle of [x,x0) ray at x), v=unit direction vector of [x,x0) ray
float3 transmittance(float r, float mu, float3 v, float3 x0) {
	float3 result;
	float r1 = length(x0);
	float mu1 = dot(x0, v) / r;
	if (mu > 0.0) {
		result = min(transmittance(r, mu) / transmittance(r1, mu1), 1.0);
	}
	else {
		result = min(transmittance(r1, -mu1) / transmittance(r, -mu), 1.0);
	}
	return result;
}

// optical depth for ray (r,mu) of length d, using analytic formula
// (mu=cos(view zenith angle)), intersections with ground ignored
// H=height scale of exponential density function
float opticalDepth(float H, float r, float mu, float d) {
	float a = sqrt((0.5 / H)*r);
	float2 a01 = a * float2(mu, mu + d / r);
	float2 a01s = sign(a01);
	float2 a01sq = a01 * a01;
	float x = a01s.y > a01s.x ? exp(a01sq.x) : 0.0;
	float2 y = a01s / (2.3193*abs(a01) + sqrt(1.52*a01sq + 4.0)) * float2(1.0, exp(-d / H * (d / (2.0*r) + mu)));
	return sqrt((6.2831*H)*r) * exp((Rg - r) / H) * (x + dot(y, float2(1.0, -1.0)));
}

// transmittance(=transparency) of atmosphere for ray (r,mu) of length d
// (mu=cos(view zenith angle)), intersections with ground ignored
// uses analytic formula instead of transmittance texture
float3 analyticTransmittance(float r, float mu, float d) {
	return exp(-betaR * opticalDepth(HR, r, mu, d) - betaMEx * opticalDepth(HM, r, mu, d));
}

// transmittance(=transparency) of atmosphere between x and x0
// assume segment x,x0 not intersecting ground
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x)
float3 transmittance(float r, float mu, float d) {
	float3 result;
	float r1 = sqrt(r * r + d * d + 2.0 * r * mu * d);
	float mu1 = (r * mu + d) / r1;
	if (mu > 0.0) {
		result = min(transmittance(r, mu) / transmittance(r1, mu1), 1.0);
	}
	else {
		result = min(transmittance(r1, -mu1) / transmittance(r, -mu), 1.0);
	}
	return result;
}

float3 irradiance(Texture2D src, float r, float muS) {
	float2 uv = getIrradianceUV(r, muS);
#ifdef	USE_SAMPLELEVEL
	return src.SampleLevel(g_LinearClamp, uv, 0).rgb;
#else	//	USE_SAMPLELEVEL
	return src.Sample(g_LinearClamp, uv).rgb;
#endif	//	USE_SAMPLELEVEL

}

// Rayleigh phase function
float phaseFunctionR(float mu) {
	return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

// Mie phase function
float phaseFunctionM(float mu) {
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG * mieG) * pow(abs(1.0 + (mieG*mieG) - 2.0*mieG*mu), -1.5f) * (1.0 + mu * mu) / (2.0 + mieG * mieG);
}

// approximated single Mie scattering
float3 getMie(float4 rayMie) { // rayMie.rgb=C*, rayMie.w=Cm,r
	return rayMie.rgb * rayMie.w / max(rayMie.r, 1e-4) * (betaR.r / betaR);
}


//inscattered light along ray x+tv, when sun in direction s (=S[L]-T(x,x0)S[L]|x0)
float3 inscatter(inout float3 x, inout float t, float3 v, float3 s, out float r, out float mu, out float3 attenuation)
{
	attenuation = float3(0.0, 0.0, 0.0);
	float3 result;
	r = length(x);
	mu = dot(x, v) / r;

	float d = -r * mu - sqrt(r * r * (mu * mu - 1.0) + Rt * Rt);

	if (d > 0.0) { // if x in space and ray intersects atmosphere
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
		float4 inscatter = max(texture4D(InscatterTexture, r, mu, muS, nu), 0.0);

		

		if (t > 0.0) {
			float3 x0 = x + t * v;
			float r0 = length(x0);
			float rMu0 = dot(x0, v);
			float mu0 = rMu0 / r0;
			float muS0 = dot(x0, s) / r0;
#ifdef FIX
			// avoids imprecision problems in transmittance computations based on textures
			attenuation = analyticTransmittance(r, mu, t);
#else
			attenuation = transmittance(r, mu, v, x0);
#endif
			if (r0 > Rg + 0.01)
			{

				// computes S[L]-T(x,x0)S[L]|x0
				inscatter = max(inscatter - attenuation.rgbr * texture4D(InscatterTexture, r0, mu0, muS0, nu), 0.0);
#ifdef FIX
				// avoids imprecision problems near horizon by interpolating between two points above and below horizon
#ifdef	IGOR_FIX_MU0
				//	const float EPS = 0.004;	//	OK
				const float EPS = 0.006;	//	Better
				float muHoriz = -0.0022;
				float muHoriz1 = -sqrt(1.0 - (Rg / r) * (Rg / r));
				if (abs(mu - muHoriz1) < EPS) muHoriz = muHoriz1;
#else	//	IGOR_FIX_MU0
				const float EPS = 0.004;
				float muHoriz = -sqrt(1.0 - (Rg / r) * (Rg / r));
#endif	//	IGOR_FIX_MU0

				if (abs(mu - muHoriz) < EPS) {
					//if (0) {
					float a = ((mu - muHoriz) + EPS) / (2.0 * EPS);

					mu = muHoriz - EPS;
					r0 = sqrt(r * r + t * t + 2.0 * r * t * mu);
					mu0 = (r * mu + t) / r0;
					float4 inScatter0 = texture4D(InscatterTexture, r, mu, muS, nu);
					float4 inScatter1 = texture4D(InscatterTexture, r0, mu0, muS0, nu);
					float4 inScatterA = max(inScatter0 - attenuation.rgbr * inScatter1, 0.0);

					mu = muHoriz + EPS;
					r0 = sqrt(r * r + t * t + 2.0 * r * t * mu);
					mu0 = (r * mu + t) / r0;
					inScatter0 = texture4D(InscatterTexture, r, mu, muS, nu);
					inScatter1 = texture4D(InscatterTexture, r0, mu0, muS0, nu);
					float4 inScatterB = max(inScatter0 - attenuation.rgbr * inScatter1, 0.0);

					inscatter = lerp(inScatterA, inScatterB, a);
				}
#endif
			}
		}

#ifdef FIX
		// avoids imprecision problems in Mie scattering when sun is below horizon
		inscatter.w *= smoothstep(0.00, 0.02, muS);
#endif
		result = max(inscatter.rgb * phaseR + getMie(inscatter) * phaseM, 0.0);
	}
	else { // x in space and ray looking in space
		result = (0.0).xxx;
	}
#ifdef	IGOR_FIX_MOIRE
	//	Igor: fix inscatter imprecision
	float fixFactor = dot(result, float3(0.3f, 0.3f, 0.3f));
	fixFactor = smoothstep(0.0007, 0.002, fixFactor);
	//	fixFactor = 1;
	return result * (ISun*fixFactor);
#else	//	IGOR_FIX_MOIRE
	return result * ISun;
#endif	//	IGOR_FIX_MOIRE
}


//ground radiance at end of ray x+tv, when sun in direction s
//attenuated bewteen ground and viewer (=R[L0]+R[L*])
float3 groundColor(float3 x, float t, float3 v, float3 s, float r, float mu, float3 attenuation, float4 OriginalImage, bool bSky = false)
{
	float3 result;
	float4 reflectance;
	reflectance.xyz = OriginalImage.xyz;
	float  SdotN = OriginalImage.w;
	reflectance.w = 0;
	if (bSky || (t > 0.0)) { // if ray hits ground surface
		// ground reflectance at end of ray, x0
		float3 x0 = x + t * v;
		float r0 = length(x0);
		float3 n = x0 / r0;

		// direct sun light (radiance) reaching x0
		float muS = dot(n, s);
		//float3 sunLight = transmittanceWithShadow(r0, muS);
		float3 sunLight = transmittanceWithShadowSmooth(r0, muS);

		// precomputed sky light (irradiance) (=E[L*]) at x0
		float3 groundSkyLight = irradiance(IrradianceTexture, r0, muS);

		float3 groundColor;
		if (bSky)
			groundColor = (reflectance.rgb * (LightIntensity.x * SdotN * sunLight) + LightIntensity.y * groundSkyLight) * ISun / M_PI;
		else
			groundColor = reflectance.rgb * (SdotN * sunLight + groundSkyLight) * ISun / M_PI;

		result = attenuation * groundColor; //=R[L0]+R[L*]
	}
	else
		result = (0.0f).xxx;
	return result;
}

// direct sun light for ray x+tv, when sun in direction s (=L0)
float3 sunColor(float3 x, float t, float3 v, float3 s, float r, float mu) {
	if (t > 0.0) {
		return (0.0).xxx;
	}
	else {
		float3 _transmittance = r <= Rt ? transmittance(r, mu) : (1.0).xxx; // T(x,xo)
		float isun = smoothstep(cos(2.5*M_PI / 180.0), cos(2.0*M_PI / 180.0), dot(v, s)) * ISun; // Lsun Igor

		//	Real sun size
		//float isun = smoothstep(cos(1.0f/108.f/2), cos(1.0f/118.f/2), dot(v, s)) * ISun; // Lsun Igor
		return _transmittance * isun; // Eq (9)
	}
}

float3 HDR_NORM(float3 L)
{
	//	TODO: Igor: Game post-process HDR is used
	return L / (0.3*100.0 / M_PI);
}

float3 HDR(float3 L) {

	L = L * LightDirection.w;
	
	float3 rgb = L;

	const float fWhiteIntensity = 1.7;
	//const float fWhiteIntensity = 4.;

	const float fWhiteIntensitySQR = fWhiteIntensity * fWhiteIntensity;

	const float intensity = dot(rgb, float3(1.0, 1.0, 1.0));

	//return	(rgb/(rgb + 1)).xyz;

	return	((rgb*(1.0f + intensity / fWhiteIntensitySQR)) / (intensity + 1)).xyz;

	L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
	L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
	L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
	return L;
}

float3 RGBtoXYZ(float3 RGB)
{
	// RGB -> XYZ conversion
	// http://www.w3.org/Graphics/Color/sRGB
	// The official sRGB to XYZ conversion matrix is (following ITU-R BT.709)
	// 0.4125 0.3576 0.1805
	// 0.2126 0.7152 0.0722
	// 0.0193 0.1192 0.9505	
	const float3x3 RGB2XYZ = { 0.5141364, 0.3238786,  0.16036376,
							  0.265068,  0.67023428, 0.06409157,
							  0.0241188, 0.1228178,  0.84442666 };

	// Make sure none of XYZ can be zero
	// This prevents temp from ever being zero, or Yxy from ever being zero        
	return mul(RGB2XYZ, RGB) + float3(0.000001f, 0.000001f, 0.000001f);
}

float3 XYZtoYxy(float3 XYZ)
{
	// XYZ -> Yxy conversion
	float3 Yxy;
	Yxy.r = XYZ.g;									// copy luminance Y

	float temp = dot(float3(1.0, 1.0, 1.0), XYZ.rgb);
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
	const float3x3 XYZ2RGB = { 2.5651, -1.1665, -0.3986,
							   -1.0217,  1.9777,  0.0439,
								0.0753, -0.2543,  1.1892 };

	return mul(XYZ2RGB, XYZ);
}

float3 YxytoXYZ(float3 Yxy)
{
	float3 XYZ;

	// Yxy -> XYZ conversion
	XYZ.r = Yxy.r * Yxy.g / Yxy.b;                // X = Y * x / y
	XYZ.g = Yxy.r;                                // copy luminance Y
	XYZ.b = Yxy.r * (1 - Yxy.g - Yxy.b) / Yxy.b;  // Z = Y * (1-x-y) / y

	return XYZ;
}

float3 YxytoRGB(float3 Yxy)
{
	float3 XYZ;
	float3 RGB;
	XYZ = YxytoXYZ(Yxy);
	RGB = XYZtoRGB(XYZ);
	return RGB;
}

float3 RGBtoYxy(float3 RGB)
{
	float3 XYZ;
	float3 Yxy;
	XYZ = RGBtoXYZ(RGB);
	Yxy = XYZtoYxy(XYZ);
	return Yxy;
}

float3 ColorSaturate(float3 L, float factor)
{
	float3 LuminanceWeights = float3(0.299, 0.587, 0.114);
	float  luminance = dot(L, LuminanceWeights);
	return lerp(luminance, L, factor);
}

float3 ColorContrast(float3 color, float tConstrast)
{
	color = saturate(color);
	return color - tConstrast * (color - 1.0f) * color * (color - 0.5f);
}