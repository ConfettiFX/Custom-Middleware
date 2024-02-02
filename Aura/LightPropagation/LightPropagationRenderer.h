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

#include "../Config/AuraConfig.h"

#include "../../../The-Forge/Common_3/Graphics/Interfaces/IGraphics.h"
#include "../../../The-Forge/Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "../Interfaces/IAuraMemoryManager.h"

DECLARE_RENDERER_FUNCTION(void, addBuffer, Renderer* pRenderer, const BufferDesc* pDesc, Buffer** ppBuffer)
DECLARE_RENDERER_FUNCTION(void, removeBuffer, Renderer* pRenderer, Buffer* pBuffer)
DECLARE_RENDERER_FUNCTION(void, mapBuffer, Renderer* pRenderer, Buffer* pBuffer, ReadRange* pRange)
DECLARE_RENDERER_FUNCTION(void, unmapBuffer, Renderer* pRenderer, Buffer* pBuffer)
