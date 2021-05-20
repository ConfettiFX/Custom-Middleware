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

#include "LightPropagationCascade.h"
#include "LightPropagationGrid.h"

namespace aura {
	void addLightPropagationCascade(Renderer* pRenderer, float gridSpan, float gridIntensity, uint32_t flags, LightPropagationCascade** ppCascade)
	{
		LightPropagationCascade* pCascade = (LightPropagationCascade*)aura::alloc(sizeof(*pCascade));

		pCascade->mGridSpan = gridSpan;
		pCascade->mGridIntensity = gridIntensity;
		pCascade->mFlags = flags;

		for (uint32_t i = 0; i < NUM_GRIDS_PER_CASCADE; ++i)
			addLightPropagationGrid(pRenderer, &pCascade->pLightGrids[i], "LPV Grid RT");

		addLightPropagationGrid(pRenderer, &pCascade->pOccluderGrid, "LPV Occlusion Grid RT");

		*ppCascade = pCascade;
	}

	void removeLightPropagationCascade(Renderer* pRenderer, LightPropagationCascade* pCascade)
	{
		for (uint32_t i = 0; i < sizeof(pCascade->pLightGrids) / sizeof(pCascade->pLightGrids[0]); ++i)
			removeLightPropagationGrid(pRenderer, pCascade->pLightGrids[i]);

		removeLightPropagationGrid(pRenderer, pCascade->pOccluderGrid);

		aura::dealloc(pCascade);
	}
}