/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Aura.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#pragma once

#include "LightPropagationRenderer.h"

namespace aura
{
void addLightPropagationGrid(Renderer* pRenderer, RenderTarget** ppGrid, const char* pName);
void removeLightPropagationGrid(Renderer* pRenderer, RenderTarget* pGrid);
} // namespace aura
