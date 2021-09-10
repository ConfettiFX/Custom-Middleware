/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#pragma once

#include "../../../../The-Forge/Common_3/OS/Math/MathTypes.h"

#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/vector.h"
#include "../../../../The-Forge/Common_3/Renderer/IRenderer.h"

class DistantCloud
{
	public:
		DistantCloud(const mat4 &Transform, Texture* tex);

		void	moveCloud(const vec3& direction);

		const mat4& Transform() const { return m_Transform; }
		void	setTransform(const mat4 & transform) { m_Transform = transform; }
		
    Texture*	m_Texture;


	private:
    mat4	m_Transform;
    
};
