#pragma once

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


