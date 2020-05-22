//--------------------------------------------------------------------------------------------
//
// Coyright (C) 2009 - 2013 Confetti Special Effects Inc.
// All rights reserved.
//
// This source may not be distributed and/or modified without expressly written permission
// from Confetti Special Effects Inc.
//
//--------------------------------------------------------------------------------------------

#include "CumulusCloud.h"

#include "../../../../The-Forge/Common_3/Renderer/IResourceLoader.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"
//#include "Singletons.h"

//#include "../Include/Renderer.h"
#include <assert.h>

const int CumulusCloud::MaxParticles = 100;
static const int MaxTexID = 15;

CumulusCloud::CumulusCloud(const mat4 &Transform, Texture* tex, float ParticlesScale):
	m_Transform(Transform), m_ParticlesScale(ParticlesScale), m_Texture(tex)
{
}

CumulusCloud::~CumulusCloud(void)
{
}

// void CumulusCloud::pushParticle( const vec3 &offset, float scale, int texID )
// {
// 	assert(texID<=MaxTexID);
// 	assert(m_OffsetScales.size()==m_texIDs.size());
// 	assert(m_texIDs.size()<MaxParticles);
// 
// 	m_OffsetScales.push_back(vec4(offset, scale));
// 	m_texIDs.push_back(texID);
// }


void CumulusCloud::setParticles( vec4 * particleOffsetScale, ParticleProps * particleProps, uint32 particleCount )
{
	ASSERT(particleOffsetScale);
	ASSERT(particleProps);
	ASSERT(particleCount <= MaxParticles);

	m_OffsetScales.clear();
	m_particleProps.clear();
	m_OffsetScales.reserve(particleCount);
	m_particleProps.reserve(particleCount);

	for (uint32 i=0; i<particleCount; ++i)
	{
		UNREF_PARAM(MaxTexID);

		m_OffsetScales.push_back(particleOffsetScale[i]);
		ASSERT(particleProps[i].texID<=MaxTexID);
		m_particleProps.push_back(particleProps[i]);
	}
}

/*
void CumulusCloud::setupConstants( const vec3 &camPos, const char* pszPositionScalesName, const char* pszTexIDsName )
{
	assert(m_OffsetScales.size()==m_particleProps.size());
	assert(m_particleProps.size()<=MaxParticles);

	sort(camPos);

// 	renderer.setShaderConstantArray4f(pszPositionScalesName, &m_OffsetScales.front(), (uint)m_OffsetScales.size());
// 	renderer.setShaderConstantArray1i(pszTexIDsName, &m_texIDs.front(), (uint)m_texIDs.size());

	pRenderer->setShaderConstantArray4f(pszPositionScalesName, &m_OffsetScales.front(), (uint)m_OffsetScales.size());
#ifdef SN_TARGET_PS3
	//	Igor: this is a hack. We pass floats and they will be interpreted as floats by the shader.
	pRenderer->setShaderConstantArray1i(pszTexIDsName, (int*)&m_texIDs.front(), (uint)m_texIDs.size());
#else
	assert(sizeof(ParticleProps)==sizeof(vec4));
	pRenderer->setShaderConstantArray4f(pszTexIDsName, (vec4 *)&m_particleProps.front(), (uint)m_particleProps.size());
#endif
}
*/

void CumulusCloud::moveCloud(const vec3 direction)
{
  //m_Transform.rows[0].w += direction.x;
	//m_Transform.rows[1].w += direction.y;
	//m_Transform.rows[2].w += direction.z;

  m_Transform[3][0] += direction.getX();
  m_Transform[3][1] += direction.getY();
  m_Transform[3][2] += direction.getZ();

}

void CumulusCloud::clipCloud( const vec3 & camPos, float XZClipR )
{
	//vec2 XZCloudPos = vec2(m_Transform.rows[0].w, m_Transform.rows[2].w);
  vec2 XZCloudPos = vec2((float)m_Transform[3][0], m_Transform[3][2]);

	//vec2 XZCamPos = vec2(camPos.x, camPos.z);
  vec2 XZCamPos = vec2(camPos.getX(), camPos.getZ());

	vec2 offset = XZCamPos - XZCloudPos;

	float offsetLength = length(offset);

	const float ClipThreashold = 1.01f;

	//	Igor: need this delta to prevent clouds flickering
	if (offsetLength>(XZClipR*ClipThreashold))
	{
		//	TODO: Igor: this is iterative schema, but it converges fast over several frames :)
		offset *= (1.0f+ClipThreashold)*XZClipR/offsetLength;
		XZCloudPos += offset;

		//m_Transform.rows[0].w = XZCloudPos.x;
    m_Transform[3][0] = XZCloudPos.getX();

		//m_Transform.rows[2].w = XZCloudPos.y;
    m_Transform[3][2] = XZCloudPos.getY();
	}
}

void CumulusCloud::sort( const vec3 &camPos )
{
	size_t a = 0;
	size_t b = m_OffsetScales.size();
	//vec3 localCamPos = (!m_Transform * vec4(camPos, 1.0f)) .xyz(); 

  vec4 temp = inverse(m_Transform) * vec4(camPos, 1.0f);
  vec3 localCamPos = temp.getXYZ();

	eastl::vector<float>	distSqr;
	distSqr.resize(b);
	for (uint i=0; i<b; ++i)
	{
		//vec3 delta = m_OffsetScales[i].xyz()-localCamPos;
    vec3 delta = m_OffsetScales[i].getXYZ() - localCamPos;
		distSqr[i] = dot(delta,delta);
	}

	//	Use bubble sort since cloud particles should have
	//	high distance from the camera coherency 
	--b;
	while (a<b)
	{
		size_t lastChanged = b;
		for (size_t i=b; i>a; --i)
		{
			if (distSqr[i]>distSqr[i-1])
			{
				float fTmp = distSqr[i-1];
				distSqr[i-1] = distSqr[i];
				distSqr[i] = fTmp;

				ParticleProps iTmp = m_particleProps[i-1];
				m_particleProps[i-1] = m_particleProps[i];
				m_particleProps[i] = iTmp;

				vec4 vTmp = m_OffsetScales[i-1];
				m_OffsetScales[i-1] = m_OffsetScales[i];
				m_OffsetScales[i] = vTmp;

				lastChanged = i;
			}
		}

		a = lastChanged;
	}
}

void CumulusCloud::centerParticles()
{
	size_t particlesCount = m_OffsetScales.size();
	if (!particlesCount) return;

  vec3 cloudMin = m_OffsetScales[0].getXYZ();
  vec3 cloudMax = m_OffsetScales[0].getXYZ();

	for (size_t i=1; i<particlesCount; ++i)
	{
		cloudMin = min(cloudMin, m_OffsetScales[i].getXYZ());
		cloudMax = max(cloudMax, m_OffsetScales[i].getXYZ());
	}

	vec3 cloudDelta = (cloudMin+cloudMax)*0.5f;

	for (size_t i=0; i<particlesCount; ++i)
	{
		m_OffsetScales[i][0] -= cloudDelta.getX();
		m_OffsetScales[i][1] -= cloudDelta.getY();
		m_OffsetScales[i][2] -= cloudDelta.getZ();
	}

}

float CumulusCloud::getRadius() const
{
	float r = 0;

	size_t particlesCount = m_OffsetScales.size();

	//	TODO: Igor: if the cloud is guaranteed to be scaled uniformly, simplify particle distance computations.
	for (size_t i=0; i<particlesCount; ++i)
	{
    vec4 temp = m_Transform * m_OffsetScales[i];
		vec3 delta = temp.getXYZ();
		//	Igor: since max is a template, this is perfectly fast and ok
		r = max(r, sqrt(dot(delta, delta))+m_OffsetScales[i].getW()*m_ParticlesScale);
	}

	return r;
/*
	//	Faster version.
	//	Currently it is assumed that all
	//	particle sizes are <=1.
	float rSQR = 0;

	size_t particlesCount = m_OffsetScales.size();

	for (size_t i=0; i<particlesCount; ++i)
	{
		vec3 delta = m_Transform * m_OffsetScales[i].xyz();
		//	Igor: since max is a template, this is perfectly fast and ok
		rSQR = max(rSQR, dot(delta, delta));
	}

	return sqrtf(rSQR)+m_ParticlesScale;
	*/
}

