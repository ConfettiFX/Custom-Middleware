/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Aura.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#include "LightPropagationGrid.h"

#ifdef XBOX
#include <DirectXPackedVector.h>
#endif

#define NO_FSL_DEFINITIONS
#include "../Shaders/FSL/lightPropagation.h"

namespace aura
{
void addLightPropagationGrid(Renderer* pRenderer, RenderTarget** ppGrid, const char* pName)
{
    RenderTargetDesc gridRTDesc = {};
    gridRTDesc.mArraySize = 1;
    gridRTDesc.mClearValue = {};
    gridRTDesc.mDepth = GridRes;
    gridRTDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
#if defined(XBOX)
    gridRTDesc.mFlags |= TEXTURE_CREATION_FLAG_ESRAM;
#endif
    gridRTDesc.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
    gridRTDesc.mHeight = GridRes;
    gridRTDesc.mSampleCount = SAMPLE_COUNT_1;
#if USE_COMPUTE_SHADERS
    gridRTDesc.mDescriptors |= DESCRIPTOR_TYPE_RW_TEXTURE;
    gridRTDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
#else
    gridRTDesc.mStartState = RESOURCE_STATE_RENDER_TARGET;
#endif
    gridRTDesc.mWidth = GridRes;
    gridRTDesc.pName = pName;
    addRenderTarget(pRenderer, &gridRTDesc, ppGrid);
}

void removeLightPropagationGrid(Renderer* pRenderer, RenderTarget* pGrid) { removeRenderTarget(pRenderer, pGrid); }
} // namespace aura
