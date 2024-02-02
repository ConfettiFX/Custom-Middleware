/*
* Copyright (c) 2017-2024 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#ifndef SPACE_OBJECTS_COMMON_H
#define SPACE_OBJECTS_COMMON_H

RES(Tex2D(float4), depthTexture,            UPDATE_FREQ_NONE, t0, binding = 0);
RES(Tex2D(float4), volumetricCloudsTexture, UPDATE_FREQ_NONE, t1, binding = 1);
RES(SamplerState,  g_LinearClamp,           UPDATE_FREQ_NONE, s0, binding = 2);
RES(SamplerState,  g_LinearBorder,          UPDATE_FREQ_NONE, s1, binding = 3);

#endif // SPACE_OBJECTS_COMMON_H
