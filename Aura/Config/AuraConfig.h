/*
 * Copyright © 2018-2021 Confetti Interactive Inc.
 *
 * This is a part of Aura.
 * 
 * This file(code) is licensed under a 
 * Creative Commons Attribution-NonCommercial 4.0 International License 
 *
 *   (https://creativecommons.org/licenses/by-nc/4.0/legalcode) 
 *
 * Based on a work at https://github.com/ConfettiFX/The-Forge.
 * You may not use the material for commercial purposes.
 *
*/

#ifndef	__LPV_CONFIG_H_INCLUDED__
#define	__LPV_CONFIG_H_INCLUDED__

#define USE_COMPUTE_SHADERS 0

#if USE_COMPUTE_SHADERS
#define RESOURCE_STATE_LPV RESOURCE_STATE_UNORDERED_ACCESS
#else
#define RESOURCE_STATE_LPV RESOURCE_STATE_RENDER_TARGET
#endif

#if USE_COMPUTE_SHADERS
#define RESOURCE_STATE_LPV RESOURCE_STATE_UNORDERED_ACCESS
#else
#define RESOURCE_STATE_LPV RESOURCE_STATE_RENDER_TARGET
#endif

#ifndef USE_COMPUTE_SHADERS
//	This is not supported for CS
//#define PROPAGATE_ACCUMULATE_ONE_PASS
#endif // !USE_COMPUTE_SHADERS

#endif	//	__LPV_CONFIG_H_INCLUDED__