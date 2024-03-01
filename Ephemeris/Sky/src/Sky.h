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

#include "../../../../The-Forge/Common_3/Application/Interfaces/ICameraController.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IMiddleware.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IProfiler.h"
#include "../../../../The-Forge/Common_3/Graphics/Interfaces/IGraphics.h"

#include "../../src/Perlin.h"

#include "Icosahedron.h"
#include "SkyCommon.h"

typedef struct ParticleData
{
    vec4 ParticlePositions;
    vec4 ParticleColors;
    vec4 ParticleInfo; // x: temperature, y: particle size, z: blink time seed,
} ParticleData;

typedef ParticleData* ParticleStbDsArray;

typedef struct ParticleSystem
{
    Buffer*            pParticleVertexBuffer;
    Buffer*            pParticleInstanceBuffer;
    ParticleStbDsArray particleDataSet;
} ParticleSystem;

class Sky: public IMiddleware
{
public:
    virtual bool Init(Renderer* renderer, PipelineCache* pCache = NULL) final;
    virtual void Exit() final;
    virtual bool Load(RenderTarget** rts, uint32_t count = 1) final;
    virtual void Unload() final;
    virtual void Draw(Cmd* cmd) final;
    virtual void Update(float deltaTime) final;

    virtual void addDescriptorSets();
    virtual void removeDescriptorSets();
    virtual void addRootSignatures();
    virtual void removeRootSignatures();
    virtual void addShaders();
    virtual void removeShaders();
    virtual void addPipelines();
    virtual void removePipelines();
    virtual void addRenderTargets();
    virtual void removeRenderTargets();
    virtual void prepareDescriptorSets(RenderTarget** ppPreStageRenderTargets, uint32_t count = 1);

    void Initialize(ICameraController* InCameraController, ProfileToken InGraphicsGpuProfiler, Buffer* pTransmittanceBuffer);

    void InitializeWithLoad(RenderTarget* InDepthRenderTarget, RenderTarget* InLinearDepthRenderTarget);

    bool   Load(int32_t width, int32_t height);
    void   CalculateLookupData();
    float3 GetSunColor();
    void   GenerateIcosahedron(float** ppPoints, VertexStbDsArray& vertices, IndexStbDsArray& indices, int numberOfDivisions,
                               float radius = 1.0f);

    Buffer*  GetParticleVertexBuffer();
    Buffer*  GetParticleInstanceBuffer();
    uint32_t GetParticleCount();

    uint gFrameIndex = 0;

    Renderer*      pRenderer = NULL;
    PipelineCache* pPipelineCache = NULL;

    // #NOTE: Two sets of resources (one in flight and one being used on CPU)
    static const uint32_t gDataBufferCount = 2;

    uint mWidth = 0;
    uint mHeight = 0;

    Texture* pTransmittanceTexture = NULL;
    Texture* pIrradianceTexture = NULL; // unsigned int irradianceTexture;//unit 2, E table
    Texture* pInscatterTexture = NULL;  // unsigned int inscatterTexture;//unit 3, S table

    Sampler* pLinearClampSampler = NULL;
    Sampler* pLinearBorderSampler = NULL;

    RenderTarget* pSkyRenderTarget = NULL;
    RenderTarget* pDepthBuffer = NULL;
    RenderTarget* pLinearDepthBuffer = NULL;

    ICameraController* pCameraController = NULL;

    float  Azimuth = 0.0f;
    float  Elevation = 0.0f;
    float3 LightDirection;

    mat4 SkyProjectionMatrix;
    mat4 SpaceProjectionMatrix;

    Buffer* pTransmittanceBuffer = NULL;

    Perlin noiseGenerator;

    ProfileToken gGpuProfileToken = {};

    ParticleSystem gParticleSystem = {};
};
