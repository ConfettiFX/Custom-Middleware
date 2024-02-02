/*
* Copyright (c) 2017-2024 The Forge Interactive Inc.
*
* This is a part of Aura.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#ifndef LPV_VISUALIZE_COMMON_H
#define LPV_VISUALIZE_COMMON_H


// VisualizationData
CBUFFER(uniforms, UPDATE_FREQ_NONE, b0, binding = 0)
{
	DATA(f4x4,  GridToCamera, None);
	DATA(f4x4,  projection,   None);
	DATA(f4x4,  invView,      None);
	DATA(float, probeRadius,  None);
	DATA(float, lightScale,   None);
};

RES(SamplerState, linearBorder, UPDATE_FREQ_NONE, s0, binding = 1);

#endif // LPV_VISUALIZE_COMMON_H