/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#pragma once

#include "../../../../The-Forge/Common_3/Utilities/ThirdParty/OpenSource/EASTL/vector.h"
#include "../../../../The-Forge/Common_3/Utilities/Math/MathTypes.h"

class HeightData
{
public:
    HeightData(const char* fileName, const char* filePassword, float heightScale);
    virtual ~HeightData(void);

    float getInterpolatedHeight(float col, float row, int step = 1) const;
    
    unsigned int colCount, rowCount;
    int colOffset, rowOffset;
private:
    int levels;
    int patchSize;
    
    eastl::vector<float> data;
};
