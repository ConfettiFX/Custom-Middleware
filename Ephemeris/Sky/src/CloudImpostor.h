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

#include "SkyDomeParams.h"

enum ImpostorUpdateMode
{
    IUM_EveryFrame,
    IUM_PositionBased
};

class CumulusCloud;

class CloudImpostor
{
public:
    CloudImpostor();
    ~CloudImpostor(void);

    void InitFromCloud(CumulusCloud* pCloud);

    const mat4& Transform() const { return m_Transform; }
    float       SortDepth() const { return m_SortDepth; }
#ifdef USE_CLOUDS_DEPTH_RECONSTRUCTION
    const float2& UnpackDepthParams() const { return m_UnpackDepthParams; }
#endif //	USE_CLOUDS_DEPTH_RECONSTRUCTION
#ifdef CLAMP_IMPOSTOR_PROJ
    const float4& ClampWindow() const { return m_ClampWindow; }
#endif //	CLAMP_IMPOSTOR_PROJ

    RenderTarget* getImpostorTexture() const { return m_tImpostor; }

    //	Impostor is cleared here, RT and shader params are set up
    void setupRenderer(Cmd* cmd, CumulusCloud* pCloud, const vec3& camPos, float camNear, const mat4& view, mat4& v, mat4& vp, vec3& dx,
                       vec3& dy, ImpostorUpdateMode updateMode, bool& needUpdate
#ifdef CLAMP_IMPOSTOR_PROJ
                       ,
                       vec3 cameraCornerDirs[4]
#endif //	CLAMP_IMPOSTOR_PROJ
#ifdef USE_CLOUDS_DEPTH_RECONSTRUCTION
                       ,
                       vec2& packDepthParams
#endif
#ifdef STABLISE_PARTICLE_ROTATION
                       ,
                       float& masterParticleRotation
#endif
    );
    bool resolve();

    uint gFrameIndex;

    Renderer* pRenderer;

    uint gImageCount;
    uint mWidth;
    uint mHeight;

private:
    float m_Radius;
    int   m_TextureSize;

    RenderTarget* m_tImpostor;

    mat4 m_Transform;
#ifdef USE_CLOUDS_DEPTH_RECONSTRUCTION
    float2 m_UnpackDepthParams; //	depthUnpacked = depthPacked*x+y;
#endif                          //	USE_CLOUDS_DEPTH_RECONSTRUCTION
#ifdef CLAMP_IMPOSTOR_PROJ
    float4 m_ClampWindow;
#endif //	CLAMP_IMPOSTOR_PROJ

    //	Store this for depth sort
    float m_SortDepth;

    //	The next data and impostor's texture is valid only if m_Valid==true
    bool  m_Valid;
    float m_OldDistance;
    vec3  m_OldNearPos;
    vec3  m_OldFarPos;

#ifdef STABLISE_PARTICLE_ROTATION
    class ParticleStabilizeData
    {
    public:
        ParticleStabilizeData();
        float updateMasterParticleRotation(const vec3& vUp, const vec3& vRight, const vec3& vForward);

    private:
        bool  m_Valid;
        float m_OldMasterParticleRotation;
        float m_MasterParticleRotation;
        vec3  m_OldRight;
    } m_ParticleStabilize;
#endif //	STABLISE_PARTICLE_ROTATION

    // Buffer*   pImposterUniformBuffer[3] = { NULL };
};