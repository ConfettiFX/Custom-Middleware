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

#include "../Math/AuraVector.h"
#include "../Interfaces/IAuraRenderer.h"
#include "../Config/AuraParams.h"

namespace aura
{
	typedef struct LightPropagationCascade
	{
		struct State
		{
			mat4	mWorldToGrid;
			vec3	mWorldToGridScale;
			vec3	mWorldToGridTranslate;
			mat4	mGridToWorld;
			vec3	mSmoothTCOffset;	//	offset to calculate grid coords as if grid was not snapped
		};

		RenderTarget*	pLightGrids[NUM_GRIDS_PER_CASCADE];
		RenderTarget*	pOccluderGrid;

		float			mGridSpan;
		float			mGridIntensity;
		uint32_t		mFlags;

		State			mInjectState;
		State			mApplyState;

		bool			mOccludersInjected;
	} LightPropagationCascade;

	void addLightPropagationCascade(Renderer* pRenderer, float gridSpan, float gridIntensity, uint32_t flags, LightPropagationCascade** ppCascade);
	void removeLightPropagationCascade(Renderer* pRenderer, LightPropagationCascade* pCascade);
}
