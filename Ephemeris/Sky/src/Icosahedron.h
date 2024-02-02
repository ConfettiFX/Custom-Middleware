/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#pragma once
#include "../../../../The-Forge/Common_3/Utilities/ThirdParty/OpenSource/Nothings/stb_ds.h"

struct VertexF3
{
    float pos[3];
};

typedef VertexF3* VertexStbDsArray;
typedef uint32_t* IndexStbDsArray;

void CreateIcosphere(uint32_t subdivisions, VertexStbDsArray* outVertices, IndexStbDsArray* outIndices);
