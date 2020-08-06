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

const float AVERAGE_GROUND_REFLECTANCE = 0.1f;

// Rayleigh
const float HR = 8.0f;
const vec3 betaR = vec3(5.8e-3, 1.35e-2, 3.31e-2);

// Mie
// DEFAULT
/*
static const float HM = 1.2;
static const float3 betaMSca = (4e-3).xxx;
static const float3 betaMEx = betaMSca / 0.9;
static const float mieG = 0.8;
/**/

// CLEAR SKY
const float HM = 1.2f;
const vec3 betaMSca = (20e-3).xxx;
const vec3 betaMEx = betaMSca / 0.9f;
const float mieG = 0.76f;
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

layout(UPDATE_FREQ_PER_FRAME, binding = 0) uniform RenderSkyUniformBuffer_Block
{
    mat4 invView;
    mat4 invProj;
    vec4 LightDirection;
    vec4 CameraPosition;
    vec4 QNNear;
    vec4 InScatterParams;
    vec4 LightIntensity;
}RenderSkyUniformBuffer;


layout(UPDATE_FREQ_NONE, binding = 0) uniform texture2D SceneColorTexture;
layout(UPDATE_FREQ_NONE, binding = 1) uniform texture2D Depth;
layout(UPDATE_FREQ_NONE, binding = 2) uniform texture2D TransmittanceTexture;
layout(UPDATE_FREQ_NONE, binding = 3) uniform texture2D IrradianceTexture;
layout(UPDATE_FREQ_NONE, binding = 4) uniform texture3D InscatterTexture;
layout(UPDATE_FREQ_NONE, binding = 5) uniform sampler g_LinearClamp;

#define FIX
//	Hides moire effect using clamping low inscattering values
#define IGOR_FIX_MOIRE
//	Hides bright horizontal line on low heights and low sun
#define IGOR_FIX_MU0
//	Disables atmosphere/space camera transition
//	Uncomment this to increase performance or handle transition
//	on CPU

vec2 getTransmittanceUV(float r, float mu)
{
    float uR, uMu;
#ifdef TRANSMITTANCE_NON_LINEAR
    (uR = sqrt(((r - Rg) / (Rt - Rg))));
    (uMu = (atan((((mu + float (0.15)) / float ((1.0 + 0.15))) * float (tan(1.5)))) / float (1.5)));
#else
    (uR = ((r - Rg) / (Rt - Rg)));
    (uMu = ((mu + float (0.15)) / float ((1.0 + 0.15))));
#endif

    return vec2(uMu, uR);
}
void getTransmittanceRMu(vec2 ScreenCoord, out float r, out float muS)
{
    (r = (ScreenCoord).y);
    (muS = (ScreenCoord).x);
#ifdef TRANSMITTANCE_NON_LINEAR
    (r = (Rg + ((r * r) * (Rt - Rg))));
    (muS = (float ((-0.15)) + ((tan((float (1.5) * muS)) / float (tan(1.5))) * float ((1.0 + 0.15)))));
#else
    (r = (Rg + (r * (Rt - Rg))));
    (muS = (float ((-0.15)) + (muS * float ((1.0 + 0.15)))));
#endif
}

vec2 getIrradianceUV(float r, float muS)
{
    float uR = ((r - Rg) / (Rt - Rg));
    float uMuS = ((muS + float (0.2)) / float ((1.0 + 0.2)));
    return vec2(uMuS, uR);
}
void getIrradianceRMuS(vec2 ScreenCoord, out float r, out float muS)
{
    (r = (Rg + ((ScreenCoord).y * (Rt - Rg))));
    (muS = (float ((-0.2)) + ((ScreenCoord).x * float ((1.0 + 0.2)))));
}
float Contrast(float Input, float ContrastPower)
{
    bool IsAboveHalf = (Input > float (0.5));
    float ToRaise = clamp((float (2) * ((IsAboveHalf)?((float (1) - Input)):(Input))), 0.0, 1.0);
    float Output = (float (0.5) * pow(ToRaise,float(ContrastPower)));
    (Output = ((IsAboveHalf)?((float (1) - Output)):(Output)));
    return Output;
}
vec4 texture4D(texture3D table, float r, float mu, float muS, float nu)
{
    float H = sqrt(((Rt * Rt) - (Rg * Rg)));
    float rho = sqrt(((r * r) - (Rg * Rg)));
#ifdef INSCATTER_NON_LINEAR
    float rmu = (r * mu);
    float delta = (((rmu * rmu) - (r * r)) + (Rg * Rg));
    vec4 cst = ((((rmu < float (0.0)) && (delta > float (0.0))))?(vec4(1.0, 0.0, 0.0, (float (0.5) - (float (0.5) / float(RES_MU))))):(vec4((-1.0), (H * H), H, (float (0.5) + (float (0.5) / float(RES_MU))))));
    float uR = ((float (0.5) / float(RES_R)) + ((rho / H) * (float (1.0) - (float (1.0) / float(RES_R)))));
    float uMu = ((cst).w + ((((rmu * (cst).x) + sqrt((delta + (cst).y))) / (rho + (cst).z)) * (float (0.5) - (float (1.0) / float(RES_MU)))));
    float uMuS = ((float (0.5) / float(RES_MU_S)) + ((((atan((max(muS, float ((-0.1975))) * float (tan((1.26 * 1.10000002))))) / float (1.10000002)) + float ((1.0 - 0.26))) * float (0.5)) * (float (1.0) - (float (1.0) / float(RES_MU_S)))));
#else
    float uR = ((float (0.5) / float(RES_R)) + ((rho / H) * (float (1.0) - (float (1.0) / float(RES_R)))));
    float uMu = ((float (0.5) / float(RES_MU)) + (((mu + float (1.0)) / float (2.0)) * (float (1.0) - (float (1.0) / float(RES_MU)))));
    float uMuS = ((float (0.5) / float(RES_MU_S)) + ((max((muS + float (0.2)), float (0.0)) / float (1.20000004)) * (float (1.0) - (float (1.0) / float(RES_MU_S)))));
#endif

    float lerp = (((nu + float (1.0)) / float (2.0)) * (float(RES_NU) - float (1.0)));
    float uNu = floor(lerp);
    (lerp = (lerp - uNu));
    (lerp = Contrast(lerp, float (3)));
#ifdef USE_SAMPLELEVEL
    return ((textureLod(sampler3D( table, g_LinearClamp), vec3(vec3(((uNu + uMuS) / float(RES_NU)), uMu, uR)), float (0)) * vec4 ((float (1.0) - lerp))) + (textureLod(sampler3D( table, g_LinearClamp), vec3(vec3((((uNu + uMuS) + float (1.0)) / float(RES_NU)), uMu, uR)), float (0)) * vec4 (lerp)));
#else
    return ((texture(sampler3D( table, g_LinearClamp), vec3(vec3(((uNu + uMuS) / float(RES_NU)), uMu, uR))) * vec4 ((float (1.0) - lerp))) + (texture(sampler3D( table, g_LinearClamp), vec3(vec3((((uNu + uMuS) + float (1.0)) / float(RES_NU)), uMu, uR))) * vec4 (lerp)));
#endif

}
float mod0(float x, float y)
{
    return (x - (y * floor((x / y))));
}
void getMuMuSNu(vec2 FragCoord, float r, vec4 dhdH, out float mu, out float muS, out float nu)
{
    float x = ((FragCoord).x - float (0.5));
    float y = ((FragCoord).y - float (0.5));
#ifdef INSCATTER_NON_LINEAR
    if((y < (float(RES_MU) / float (2.0))))
    {
        float d = (float (1.0) - (y / float (((float (RES_MU) / 2.0) - 1.0))));
        (d = min(max((dhdH).z, (d * (dhdH).w)), ((dhdH).w * float (0.9990000))));
        (mu = ((((Rg * Rg) - (r * r)) - (d * d)) / ((float (2.0) * r) * d)));
        (mu = min(mu, ((-sqrt((float (1.0) - ((Rg / r) * (Rg / r))))) - float (0.0010000000))));
    }
    else
    {
        float d = ((y - float ((float (RES_MU) / 2.0))) / float (((float (RES_MU) / 2.0) - 1.0)));
        (d = min(max((dhdH).x, (d * (dhdH).y)), ((dhdH).y * float (0.9990000))));
        (mu = ((((Rt * Rt) - (r * r)) - (d * d)) / ((float (2.0) * r) * d)));
    }
    (muS = (mod0(x, float(RES_MU_S)) / (float(RES_MU_S) - float (1.0))));
    (muS = (tan(((((float (2.0) * muS) - float (1.0)) + float (0.26)) * float (1.10000002))) / float (tan((1.26 * 1.10000002)))));
    (nu = (float ((-1.0)) + (floor((x / float(RES_MU_S))) / ((float(RES_NU) - float (1.0)) * float (2.0)))));
#else
    (mu = (float ((-1.0)) + ((float (2.0) * y) / (float(RES_MU) - float (1.0)))));
    (muS = (mod0(x, float(RES_MU_S)) / (float(RES_MU_S) - float (1.0))));
    (muS = (float ((-0.2)) + (muS * float (1.20000004))));
    (nu = (float ((-1.0)) + (floor((x / float(RES_MU_S))) / ((float(RES_NU) - float (1.0)) * float (2.0)))));
#endif

}
float limit(float r, float mu)
{
    float dout = (((-r) * mu) + sqrt((((r * r) * ((mu * mu) - float (1.0))) + (RL * RL))));
    float delta2 = (((r * r) * ((mu * mu) - float (1.0))) + (Rg * Rg));
    if((delta2 >= float (0.0)))
    {
        float din = (((-r) * mu) - sqrt(delta2));
        if((din >= float (0.0)))
        {
            (dout = min(dout, din));
        }
    }
    return dout;
}
vec3 transmittance(float r, float mu)
{
    vec2 uv = getTransmittanceUV(r, mu);
#ifdef USE_SAMPLELEVEL
    return vec3 ((textureLod(sampler2D( TransmittanceTexture, g_LinearClamp), vec2(uv), float (0))).rgb);
#else
    return vec3 ((texture(sampler2D( TransmittanceTexture, g_LinearClamp), vec2(uv))).rgb);
#endif

}
vec3 transmittanceWithShadow(float r, float mu)
{
    return vec3 ((((mu < (-sqrt((float (1.0) - ((Rg / r) * (Rg / r)))))))?(vec3 ((0.0).xxx)):(transmittance(r, mu))));
}
vec3 transmittanceWithShadowSmooth(float r, float mu)
{
    float eps = ((0.5 * M_PI) / 180.0);
    float horizMu = (-sqrt((float (1.0) - ((Rg / r) * (Rg / r)))));
    float horizAlpha = acos(horizMu);
    float horizMuMin = cos((horizAlpha + eps));
    float horizMuMax = cos((horizAlpha - eps));
    return vec3 (mix((0.0).xxx, vec3 (transmittance(r, mu)), vec3 (smoothstep(horizMuMin, horizMuMax, mu))));
}
vec3 transmittance(float r, float mu, vec3 v, vec3 x0)
{
    vec3 result;
    float r1 = length(x0);
    float mu1 = (dot(x0, v) / r);
    if((mu > float (0.0)))
    {
        (result = min((transmittance(r, mu) / transmittance(r1, mu1)), vec3 (1.0)));
    }
    else
    {
        (result = min((transmittance(r1, (-mu1)) / transmittance(r, (-mu))), vec3 (1.0)));
    }
    return result;
}
float opticalDepth(float H, float r, float mu, float d)
{
    float a = sqrt(((float (0.5) / H) * r));
    vec2 a01 = (vec2 (a) * vec2(mu, (mu + (d / r))));
    vec2 a01s = sign(a01);
    vec2 a01sq = (a01 * a01);
    float x = ((((a01s).y > (a01s).x))?(exp((a01sq).x)):(float (0.0)));
    vec2 y = ((a01s / ((vec2 (2.31929994) * abs(a01)) + sqrt(((vec2 (1.52) * a01sq) + vec2 (4.0))))) * vec2(1.0, exp((((-d) / H) * ((d / (float (2.0) * r)) + mu)))));
    return ((sqrt(((float (6.28310012) * H) * r)) * exp(((Rg - r) / H))) * (x + dot(y, vec2(1.0, (-1.0)))));
}
vec3 analyticTransmittance(float r, float mu, float d)
{
    return exp((((-betaR) * vec3 (opticalDepth(HR, r, mu, d))) - (betaMEx * vec3 (opticalDepth(HM, r, mu, d)))));
}
vec3 transmittance(float r, float mu, float d)
{
    vec3 result;
    float r1 = sqrt((((r * r) + (d * d)) + (((float (2.0) * r) * mu) * d)));
    float mu1 = (((r * mu) + d) / r1);
    if((mu > float (0.0)))
    {
        (result = min((transmittance(r, mu) / transmittance(r1, mu1)), vec3 (1.0)));
    }
    else
    {
        (result = min((transmittance(r1, (-mu1)) / transmittance(r, (-mu))), vec3 (1.0)));
    }
    return result;
}
vec3 irradiance(texture2D src, float r, float muS)
{
    vec2 uv = getIrradianceUV(r, muS);
#ifdef USE_SAMPLELEVEL
    return vec3 ((textureLod(sampler2D( src, g_LinearClamp), vec2(uv), float (0))).rgb);
#else
    return vec3 ((texture(sampler2D( src, g_LinearClamp), vec2(uv))).rgb);
#endif

}
float phaseFunctionR(float mu)
{
    return ((float (3.0) / (float (16.0) * M_PI)) * (float (1.0) + (mu * mu)));
}
float phaseFunctionM(float mu)
{
    return (((((float ((1.5 * 1.0)) / (float (4.0) * M_PI)) * (float (1.0) - (mieG * mieG))) * pow(abs(((float (1.0) + (mieG * mieG)) - ((float (2.0) * mieG) * mu))),float((-1.5)))) * (float (1.0) + (mu * mu))) / (float (2.0) + (mieG * mieG)));
}
vec3 getMie(vec4 rayMie)
{
    return ((((rayMie).rgb * vec3 ((rayMie).w)) / vec3 (max((rayMie).r, 0.00010000000))) * (vec3 ((betaR).r) / betaR));
}
vec3 inscatter(inout vec3 x, inout float t, vec3 v, vec3 s, out float r, out float mu, out vec3 attenuation)
{
    (attenuation = vec3(0.0, 0.0, 0.0));
    vec3 result;
    (r = length(x));
    (mu = (dot(x, v) / r));
    float d = (((-r) * mu) - sqrt((((r * r) * ((mu * mu) - float (1.0))) + (Rt * Rt))));
    if((d > float (0.0)))
    {
        (x += (vec3 (d) * v));
        (t -= d);
        (mu = (((r * mu) + d) / Rt));
        (r = Rt);
    }
    if((r <= Rt))
    {
        float nu = dot(v, s);
        float muS = (dot(x, s) / r);
        float phaseR = phaseFunctionR(nu);
        float phaseM = phaseFunctionM(nu);
        vec4 inscatter = max(texture4D(InscatterTexture, r, mu, muS, nu), vec4 (0.0));
        if((t > float (0.0)))
        {
            vec3 x0 = (x + (vec3 (t) * v));
            float r0 = length(x0);
            float rMu0 = dot(x0, v);
            float mu0 = (rMu0 / r0);
            float muS0 = (dot(x0, s) / r0);
#ifdef FIX
            (attenuation = analyticTransmittance(r, mu, t));
#else
            (attenuation = transmittance(r, mu, v, x0));
#endif

            if((r0 > (Rg + float (0.010000000))))
            {
                (inscatter = max((inscatter - ((attenuation).rgbr * texture4D(InscatterTexture, r0, mu0, muS0, nu))), vec4 (0.0)));
#ifdef FIX
#ifdef IGOR_FIX_MU0
                float EPS = float (0.006);
                float muHoriz = float ((-0.0022));
                float muHoriz1 = (-sqrt((float (1.0) - ((Rg / r) * (Rg / r)))));
                if((abs((mu - muHoriz1)) < EPS))
                {
                    (muHoriz = muHoriz1);
                }
#else
                float EPS = float (0.0040000000);
                float muHoriz = (-sqrt((float (1.0) - ((Rg / r) * (Rg / r)))));
#endif

                if((abs((mu - muHoriz)) < EPS))
                {
                    float a = (((mu - muHoriz) + EPS) / (float (2.0) * EPS));
                    (mu = (muHoriz - EPS));
                    (r0 = sqrt((((r * r) + (t * t)) + (((float (2.0) * r) * t) * mu))));
                    (mu0 = (((r * mu) + t) / r0));
                    vec4 inScatter0 = texture4D(InscatterTexture, r, mu, muS, nu);
                    vec4 inScatter1 = texture4D(InscatterTexture, r0, mu0, muS0, nu);
                    vec4 inScatterA = max((inScatter0 - ((attenuation).rgbr * inScatter1)), vec4 (0.0));
                    (mu = (muHoriz + EPS));
                    (r0 = sqrt((((r * r) + (t * t)) + (((float (2.0) * r) * t) * mu))));
                    (mu0 = (((r * mu) + t) / r0));
                    (inScatter0 = texture4D(InscatterTexture, r, mu, muS, nu));
                    (inScatter1 = texture4D(InscatterTexture, r0, mu0, muS0, nu));
                    vec4 inScatterB = max((inScatter0 - ((attenuation).rgbr * inScatter1)), vec4 (0.0));
                    (inscatter = vec4 (mix(vec4 (inScatterA), vec4 (inScatterB), vec4 (a))));
                }
#endif

            }
        }
#ifdef FIX
        ((inscatter).w *= smoothstep(float (0.0), float (0.020000000), muS));
#endif

        (result = max((((inscatter).rgb * vec3 (phaseR)) + (getMie(inscatter) * vec3 (phaseM))), vec3 (0.0)));
    }
    else
    {
        (result = vec3 ((0.0).xxx));
    }
#ifdef IGOR_FIX_MOIRE
    float fixFactor = dot(result, vec3(0.3, 0.3, 0.3));
    (fixFactor = smoothstep(float (0.0006), float (0.0020000000), fixFactor));
    return (result * vec3 ((ISun * fixFactor)));
#else
    return (result * vec3 (ISun));
#endif

}
vec3 groundColor(vec3 x, float t, vec3 v, vec3 s, float r, float mu, vec3 attenuation, vec4 OriginalImage, bool bSky)
{
    vec3 result;
    vec4 reflectance;
    ((reflectance).xyz = (OriginalImage).xyz);
    float SdotN = (OriginalImage).w;
    ((reflectance).w = float (0));
    if((bSky || (t > float (0.0))))
    {
        vec3 x0 = (x + (vec3 (t) * v));
        float r0 = length(x0);
        vec3 n = (x0 / vec3 (r0));
        float muS = dot(n, s);
        vec3 sunLight = transmittanceWithShadowSmooth(r0, muS);
        vec3 groundSkyLight = irradiance(IrradianceTexture, r0, muS);
        vec3 groundColor;
        if(bSky)
        {
            (groundColor = (((((reflectance).rgb * (vec3 (((RenderSkyUniformBuffer.LightIntensity).x * SdotN)) * sunLight)) + (vec3 ((RenderSkyUniformBuffer.LightIntensity).y) * groundSkyLight)) * vec3 (ISun)) / vec3 (M_PI)));
        }
        else
        {
            (groundColor = ((((reflectance).rgb * ((vec3 (SdotN) * sunLight) + groundSkyLight)) * vec3 (ISun)) / vec3 (M_PI)));
        }
        (result = (attenuation * groundColor));
    }
    else
    {
        (result = (0.0).xxx);
    }
    return result;
}
vec3 sunColor(vec3 x, float t, vec3 v, vec3 s, float r, float mu)
{
    if((t > float (0.0)))
    {
        return vec3 ((0.0).xxx);
    }
    else
    {
        vec3 _transmittance = (((r <= Rt))?(transmittance(r, mu)):(vec3 ((1.0).xxx)));
        float isun = (smoothstep(cos(((float (2.5) * M_PI) / float (180.0))), cos(((float (2.0) * M_PI) / float (180.0))), dot(v, s)) * ISun);
        return (_transmittance * vec3 (isun));
    }
}
vec3 HDR_NORM(vec3 L)
{
    return (L / vec3 ((float ((0.3 * 100.0)) / M_PI)));
}
vec3 HDR(vec3 L)
{
    (L = (L * vec3 ((RenderSkyUniformBuffer.LightDirection).w)));
    vec3 rgb = L;
    float fWhiteIntensity = float (1.70000008);
    float fWhiteIntensitySQR = (fWhiteIntensity * fWhiteIntensity);
    float intensity = dot(rgb, vec3(1.0, 1.0, 1.0));
    return (((rgb * vec3 ((1.0 + (intensity / fWhiteIntensitySQR)))) / vec3 ((intensity + float (1))))).xyz;
    ((L).r = ((((L).r < float (1.41300000)))?(pow(((L).r * float (0.38317)),float((1.0 / 2.20000004)))):((float (1.0) - exp((-(L).r))))));
    ((L).g = ((((L).g < float (1.41300000)))?(pow(((L).g * float (0.38317)),float((1.0 / 2.20000004)))):((float (1.0) - exp((-(L).g))))));
    ((L).b = ((((L).b < float (1.41300000)))?(pow(((L).b * float (0.38317)),float((1.0 / 2.20000004)))):((float (1.0) - exp((-(L).b))))));
    return L;
}
vec3 RGBtoXYZ(vec3 RGB)
{
    mat3 RGB2XYZ = {{0.5141363, 0.32387858, 0.16036376}, {0.265068, 0.6702342, 0.06409157}, {0.0241188, 0.1228178, 0.8444266}};
    return (((RGB2XYZ)*(RGB)) + vec3(1.000000e-06, 1.000000e-06, 1.000000e-06));
}
vec3 XYZtoYxy(vec3 XYZ)
{
    vec3 Yxy;
    ((Yxy).r = (XYZ).g);
    float temp = dot(vec3(1.0, 1.0, 1.0), (XYZ).rgb);
    ((Yxy).gb = ((XYZ).rg / vec2 (temp)));
    return Yxy;
}
vec3 XYZtoRGB(vec3 XYZ)
{
    mat3 XYZ2RGB = {{2.5650, (-1.16649997), (-0.3986)}, {(-1.021700024), 1.9777000, 0.0439}, {0.0753, (-0.2543), 1.18920004}};
    return ((XYZ2RGB)*(XYZ));
}
vec3 YxytoXYZ(vec3 Yxy)
{
    vec3 XYZ;
    ((XYZ).r = (((Yxy).r * (Yxy).g) / (Yxy).b));
    ((XYZ).g = (Yxy).r);
    ((XYZ).b = (((Yxy).r * ((float (1) - (Yxy).g) - (Yxy).b)) / (Yxy).b));
    return XYZ;
}
vec3 YxytoRGB(vec3 Yxy)
{
    vec3 XYZ;
    vec3 RGB;
    (XYZ = YxytoXYZ(Yxy));
    (RGB = XYZtoRGB(XYZ));
    return RGB;
}
vec3 RGBtoYxy(vec3 RGB)
{
    vec3 XYZ;
    vec3 Yxy;
    (XYZ = RGBtoXYZ(RGB));
    (Yxy = XYZtoYxy(XYZ));
    return Yxy;
}
vec3 ColorSaturate(vec3 L, float factor)
{
    vec3 LuminanceWeights = vec3(0.299, 0.587, 0.114);
    float luminance = dot(L, LuminanceWeights);
    return vec3 (mix(vec3 (luminance), vec3 (L), vec3 (factor)));
}
vec3 ColorContrast(vec3 color, float tConstrast)
{
    (color = clamp(color, 0.0, 1.0));
    return (color - (((vec3 (tConstrast) * (color - vec3 (1.0))) * color) * (color - vec3 (0.5))));
}