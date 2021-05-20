/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*/

#ifndef SKY_COMMON_H
#define SKY_COMMON_H

CBUFFER(RenderSkyUniformBuffer, UPDATE_FREQ_PER_FRAME, b0, binding = 0)
{
	DATA(f4x4,   invView,         None);
	DATA(f4x4,   invProj,         None);
	DATA(float4, lightDirection,  None);
	DATA(float4, CameraPosition,  None);
	DATA(float4, QNNear,          None);
	DATA(float4, InScatterParams, None);
	DATA(float4, LightIntensity,  None);
};

CBUFFER(SpaceUniform, UPDATE_FREQ_PER_FRAME, b1, binding = 1)
{
	DATA(f4x4,   ViewProjMat,     None);
	DATA(float4, LightDirection,  None);
	DATA(float4, ScreenSize,      None);
	DATA(float4, NebulaHighColor, None);
	DATA(float4, NebulaMidColor,  None);
	DATA(float4, NebulaLowColor,  None);
};

RES(Tex2D(float4),    SceneColorTexture,    UPDATE_FREQ_NONE, t0, binding = 0);
RES(Tex2D(float4),    Depth,                UPDATE_FREQ_NONE, t1, binding = 1);
RES(Tex2D(float4),    TransmittanceTexture, UPDATE_FREQ_NONE, t2, binding = 2);
RES(Tex2D(float4),    IrradianceTexture,    UPDATE_FREQ_NONE, t3, binding = 3);
RES(Tex3D(float4),    InscatterTexture,     UPDATE_FREQ_NONE, t4, binding = 4);
RES(Tex2D(float4),    depthTexture,         UPDATE_FREQ_NONE, t5, binding = 5);
RES(RWBuffer(float4), TransmittanceColor,   UPDATE_FREQ_NONE, u0, binding = 6);
RES(SamplerState,     g_LinearClamp,        UPDATE_FREQ_NONE, s0, binding = 7);

//STATIC const int TRANSMITTANCE_INTEGRAL_SAMPLES = 500;
//STATIC const int INSCATTER_INTEGRAL_SAMPLES = 50;
//STATIC const int IRRADIANCE_INTEGRAL_SAMPLES = 32;
//STATIC const int INSCATTER_SPHERICAL_INTEGRAL_SAMPLES = 16;

STATIC const float M_PI = 3.141592657f;

STATIC const float Rg = 6360.0f;
STATIC const float Rt = 6420.0f;
STATIC const float RL = 6421.0f;

//STATIC const int TRANSMITTANCE_W = 256;
//STATIC const int TRANSMITTANCE_H = 64;

//STATIC const int SKY_W = 64;
//STATIC const int SKY_H = 16;

STATIC const int RES_R = 32;
STATIC const int RES_MU = 128;
STATIC const int RES_MU_S = 32;
STATIC const int RES_NU = 8;

STATIC const float ISun = 100.0f;

#endif // SKY_COMMON_H