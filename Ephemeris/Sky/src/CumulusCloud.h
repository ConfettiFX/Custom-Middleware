//--------------------------------------------------------------------------------------------
//
// Coyright (C) 2009 - 2013 Confetti Special Effects Inc.
// All rights reserved.
//
// This source may not be distributed and/or modified without expressly written permission
// from Confetti Special Effects Inc.
//
//--------------------------------------------------------------------------------------------

#pragma once

#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/vector.h"
#include "../../../../The-Forge/Common_3/Renderer/IRenderer.h"

#include "CloudsManager.h"
//#include "Containers.h"

class CumulusCloud
{
	public:
		CumulusCloud(const mat4 &Transform, Texture* tex, float ParticlesScale);

		const mat4& Transform() const { return m_Transform; }
		void	setTransform(const mat4 & transform) { m_Transform = transform; }
		float	ParticlesScale() const { return m_ParticlesScale; }
    Texture* GetTexture() const { return m_Texture;}

		//void	pushParticle(const vec3 &offset, float scale, int texID);
		void	setParticles(vec4 * particleOffsetScale, ParticleProps * particleProps, uint32_t particleCount);
		void	centerParticles();

		void	setupConstants( const vec3 &camPos, const char* pszPositionScalesName, const char* pszTexIDsName );
		size_t	getParticlesCount()  const {return m_OffsetScales.size();}
		float	getRadius() const;
		void	moveCloud(const vec3 & direction);
		void	clipCloud(const vec3 & camPos, float XZClipR);

        static const int MaxParticles;

        eastl::vector<vec4>	m_OffsetScales;
        //	TODO: PS3: Igor: haven't figured out yet how to set up ints for PS3 is possible at all.
#ifdef SN_TARGET_PS3
        eastl::vector<float>	m_texIDs;
#else
        eastl::vector<ParticleProps>	m_particleProps;
#endif

	private:
		void	sort(const vec3 &camPos);

	private:
    mat4	m_Transform;
		float		m_ParticlesScale;
    Texture*	m_Texture;
		

};
