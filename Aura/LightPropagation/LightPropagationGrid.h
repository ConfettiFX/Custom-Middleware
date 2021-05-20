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

#include "../Interfaces/IAuraRenderer.h"

namespace aura {
	void addLightPropagationGrid(Renderer* pRenderer, RenderTarget** ppGrid, const char* pName);
	void removeLightPropagationGrid(Renderer* pRenderer, RenderTarget* pGrid);
}
