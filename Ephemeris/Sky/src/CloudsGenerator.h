/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#pragma once

#include "SkyDomeParams.h"
#include "CloudsManager.h"

	typedef unsigned int uint;

	struct DistantCloudParams
	{
		DistantCloudParams() : cloudCount(15), scaleMultiplier(50.0f), baseHeight(20000.0f) {}
		uint cloudCount;
		float scaleMultiplier;
		float baseHeight;
	};

	struct CumulusCloudParams
	{
		CumulusCloudParams()
			: scaleMultiplier(1.0f), cloudCountX(10), cloudCountZ(10), baseHeight(10000.0f) {}
		float scaleMultiplier;
		float baseHeight;
		uint cloudCountZ;
		uint cloudCountX;
	};

	class ISkyDome;

	void generateClouds(ICloudsManager &CloudsManager, const CumulusCloudParams& cClParams, const DistantCloudParams& dClParams, uint seed = 81936);


