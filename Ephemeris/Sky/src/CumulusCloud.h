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
#error unused code
#include "../../../../The-Forge/Common_3/Graphics/Interfaces/IGraphics.h"

#include "CloudsManager.h"
// #include "Containers.h"

class CumulusCloud
{
public:
    CumulusCloud(const mat4& Transform, Texture* tex, float ParticlesScale);

    ~CumulusCloud()
    {
        arrfree(m_OffsetScales);
        arrfree(m_particleProps);
    }

    const mat4& Transform() const { return m_Transform; }
    void        setTransform(const mat4& transform) { m_Transform = transform; }
    float       ParticlesScale() const { return m_ParticlesScale; }
    Texture*    GetTexture() const { return m_Texture; }

    // void	pushParticle(const vec3 &offset, float scale, int texID);
    void setParticles(vec4* particleOffsetScale, ParticleProps* particleProps, uint32_t particleCount);
    void centerParticles();

    void  setupConstants(const vec3& camPos, const char* pszPositionScalesName, const char* pszTexIDsName);
    float getRadius() const;
    void  moveCloud(const vec3& direction);
    void  clipCloud(const vec3& camPos, float XZClipR);

    static const int MaxParticles;

    uint32_t m_particleCount;
    vec4*    m_OffsetScales = NULL;
    //	TODO: PS3: Igor: haven't figured out yet how to set up ints for PS3 is possible at all.
#ifdef SN_TARGET_PS3
    eastl::vector<float> m_texIDs;
#else
    ParticleProps* m_particleProps = NULL;
#endif

private:
    void sort(const vec3& camPos);

private:
    mat4     m_Transform;
    float    m_ParticlesScale;
    Texture* m_Texture;
};
