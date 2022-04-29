/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#pragma once

#include "../../../../The-Forge/Common_3/OS/Math/MathTypes.h"

#include "../../../../The-Forge/Common_3/Renderer/IRenderer.h"
#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/vector.h"

// referred to https://github.com/thibauts/b-spline#readme
class B_Spline
{
public:    
    B_Spline() {}
    ~B_Spline() {}
   
    void interpolate(eastl::vector<float> &t, uint degree, eastl::vector<vec3> &points, eastl::vector<float> &knots, eastl::vector<float> &weights, eastl::vector<vec3> &result);
};


