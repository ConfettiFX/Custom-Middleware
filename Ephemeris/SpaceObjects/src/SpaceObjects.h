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

//#include "../../src/Perlin.h"
//#include "B_Spline.h"
//#include "Aurora.h"

#include "../../../../The-Forge/Common_3/Application/Interfaces/ICameraController.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IMiddleware.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IProfiler.h"
#include "../../../../The-Forge/Common_3/Application/Interfaces/IUI.h"
#include "../../../../The-Forge/Common_3/Graphics/Interfaces/IGraphics.h"

class SpaceObjects: public IMiddleware
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
    virtual void prepareDescriptorSets(RenderTarget** ppPreStageRenderTargets, uint32_t count = 1);

    void Initialize(ICameraController* InCameraController, ProfileToken InGraphicsGpuProfiler, Buffer* pTransmittanceBuffer);

    void InitializeWithLoad(RenderTarget* InDepthRenderTarget, RenderTarget* InLinearDepthRenderTarget, Texture* SavePrevTexture,
                            Buffer* ParticleVertexBuffer, Buffer* ParticleInstanceBuffer, uint32_t ParticleCountParam,
                            uint32_t ParticleVertexStride, uint32_t ParticleInstanceStride);
    bool Load(int32_t width, int32_t height);

    // void GenerateRing(eastl::vector<float> &vertices, eastl::vector<uint32_t> &indices, uint32_t WidthDividor, uint32_t HeightDividor,
    // float radius = 1.0f, float height = 1.0f);

    uint gFrameIndex = 0;

    Renderer*      pRenderer = NULL;
    PipelineCache* pPipelineCache = NULL;

    // #NOTE: Two sets of resources (one in flight and one being used on CPU)
    static const uint32_t gDataBufferCount = 2;

    uint mWidth = 0;
    uint mHeight = 0;

    UIComponent* pGuiWindow = nullptr;

    ProfileToken gGpuProfileToken = {};

    Texture* pMoonTexture = NULL;

    Texture* pHighResCloudTexture = NULL;

    Buffer* pGlobalTriangularVertexBuffer = NULL;

    Sampler* pLinearClampSampler = NULL;
    Sampler* pLinearBorderSampler = NULL;
    Sampler* pNearestClampSampler = NULL;

    RenderTarget* pPreStageRenderTarget = NULL;
    // RenderTarget*         pSkyRenderTarget;
    RenderTarget* pDepthBuffer = NULL;
    RenderTarget* pLinearDepthBuffer = NULL;

    ICameraController* pCameraController = NULL;

    Buffer*  pParticleVertexBuffer = NULL;
    Buffer*  pParticleInstanceBuffer = NULL;
    uint32_t ParticleCount = 0;
    uint32_t ParticleVertexStride = 0;
    uint32_t ParticleInstanceStride = 0;

    mat4 SpaceProjectionMatrix;

    float  Azimuth = 0.0f;
    float  Elevation = 0.0f;
    float3 LightDirection;
    float4 LightColorAndIntensity;
};
