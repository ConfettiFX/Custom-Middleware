/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#ifndef EXAMPLE_COMMON_H
#define EXAMPLE_COMMON_H

PUSH_CONSTANT(ExampleRootConstant, b0)
{
#if defined(DEPTH_LINEARIZATION_PROGRAM)
	DATA(float, nearPlane, None);
	DATA(float, farPlane,  None);
	DATA(float, padding00, None);
	DATA(float, padding01, None);
#elif defined(FXAA_PROGRAM)
	DATA(float2, ScreenSize, None);
	DATA(float,  Use,        None);
	DATA(float,  Time,       None);
#endif
};

RES(Tex2D(float4),   SrcTexture,    UPDATE_FREQ_NONE, t0, binding = 0);
RES(RWTex2D(float4), DstTexture,    UPDATE_FREQ_NONE, u0, binding = 1);
RES(SamplerState,    g_LinearClamp, UPDATE_FREQ_NONE, s0, binding = 2);

#endif // EXAMPLE_COMMON_H
