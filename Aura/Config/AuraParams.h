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

#pragma once

#include <stdint.h>
#include "../Math/AuraMath.h"

#define NUM_GRIDS_PER_CASCADE 3U

namespace aura
{
	const int SPECULAR_QUALITY_DEFAULT = 3;

	enum MTTypes
	{
		MT_None = 0,
		MT_ExtremeTasks,
		MT_MAX

		/* Limiting Multi-threading options for GI Demo
		MT_None = 0,
		MT_SingleTask,
		MT_3Tasks,
		MT_6Tasks,
		MT_ExtremeTasks,
		MT_MAX
		*/
	};

	struct LightPropagationVolumeParams
	{
		bool	bUseMultipleReflections;
		bool	bUseCPUPropagation;
		bool	bAlternateGPUUpdates;	//	TODO: Igor: remove this from the LPV library, since this is the client who is responcible for this.
		float	fPropagationScale;		//	TODO: Igor: remove this debug attribute
		bool    bDebugLight;
		bool    bDebugOccluder;
		float	fGIStrength;
		uint32_t	iPropagationSteps;
		uint32_t	iSpecularQuality;
		float	fLightScale[3];
		float	fSpecScale;
		float	fSpecPow;
		float	fFresnel;
		uint32_t	userDebug;
	};

	struct ScreenSpaceGIParams
	{
		bool	bUseDiscDithering;
		bool	bUseScreenSpacePositions;
		float	fSSGIIntencity;
	};

	struct CPUPropagationParams
	{
		MTTypes	eMTMode;
		bool	bDecoupled;
	};

	struct Params
	{
		LightPropagationVolumeParams	LPVParams;
		ScreenSpaceGIParams				SSGIParams;
		CPUPropagationParams			CPUParams;
	};

	const char* const MT_STRINGS[] =
	{
		"Syncronous propagation",
		"Extreme-tasked propagation",

		/* Limiting Multi-threading options for GI Demo
		"Syncronous propagation",
		"Single-tasked propagation",
		"3-tasked propagation",
		"6-tasked propagation",
		"Extreme-tasked propagation",
		*/
	};

	const char* const SPECULAR_QUALITY_STRINGS[] =
	{
		"Off",
		"Minimum",
		"Medium",
		"Maximum - default",
		nullptr
	};

	const uint32_t SPECULAR_QUALITY_VALS[] =
	{
		0,
		1,
		2,
		3
	};

}
