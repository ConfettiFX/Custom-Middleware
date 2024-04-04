/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Aura.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#ifndef __LPV_CONFIG_H_INCLUDED__
#define __LPV_CONFIG_H_INCLUDED__

#include "../../../The-Forge/Common_3/Graphics/GraphicsConfig.h"
#include "../Shaders/Shared.h"

#if defined(_WINDOWS) || defined(XBOX) || defined(__APPLE__) || defined(__linux__)
#define ENABLE_CPU_PROPAGATION
#elif defined(ORBIS)
// orbis fibers and sanitizer don't work well together at the moment
// enable cpu propagation only if sanitizers are turned off
#if !__has_feature(address_sanitizer) && !__has_feature(undefined_behavior_sanitizer)
#define ENABLE_CPU_PROPAGATION
#endif
#endif

#ifdef ORBIS
// #define ORBIS_TASK_MANAGER
#endif

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
// #define PROPAGATE_ACCUMULATE_ONE_PASS
#endif // !USE_COMPUTE_SHADERS

#endif //	__LPV_CONFIG_H_INCLUDED__