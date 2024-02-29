/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Aura.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#pragma once

#include "../Interfaces/IAuraTaskManager.h"

#include "../Math/AuraMath.h"
#include "../Math/AuraVector.h"

#include "LightPropagationCascade.h"
#include "LightPropagationRenderer.h"

namespace aura
{
struct TextureFootprint
{
    uint64_t mTotalByteCount;
    uint64_t mRowPitch;
};

class LightPropagationCPUContext
{
public:
    struct StepContext
    {
        LightPropagationCPUContext* pContext;
        vec4*                       src;
        vec4*                       targetStep;
        vec4*                       targetAccum;
    };

    enum LP_STATE
    {
        CAPTURED_LIGHT,
        PROPAGATED_LIGHT,
        APPLIED_PROPAGATION
    } eState;

public:
    void readData(Cmd* pCmd, Renderer* pRenderer, RenderTarget* m_LightGrids[3], uint32_t numGrids);
    void processData(Renderer* pRenderer, ITaskManager* pTaskManager, MTTypes propagationMTType);
    void applyData(Cmd* pCmd, Renderer* pRenderer, RenderTarget* m_LightGrids[3]);

    bool load(Renderer* pRenderer, RenderTarget* m_LightGrids[3]);
    void unload(Renderer* pRenderer, ITaskManager* pTaskManager);

    // returns false if all staging buffers aren't locked and ready to propagate
    // bool propagateLight(Cmd* pCmd, ITaskManager* pTaskManager, RenderTarget* m_LightGrids[3], MTTypes propagationMTType, int
    // PropagationSetps);

    void applyPropagatedLight(Cmd* pCmd, ITaskManager* pTaskManager, RenderTarget* m_LightGrids[3], bool decoupled);

    const LightPropagationCascade::State& getApplyState() const { return m_applyState; }
    void                                  setApplyState(const LightPropagationCascade::State& val) { m_applyState = val; }

private:
    void convertGPUtoCPU(Renderer* pRenderer);
    void convertCPUtoGPU();

    void launchPropagateSingleTask(ITaskManager* pTaskManager);
    void launchPropagateMultiTask(ITaskManager* pTaskManager, const int iTasksPerStep = 1);

    void doPropagate();

    void SyncToLastTask(ITaskManager* pTaskManager);

    template<bool bFirstStep>
    void propagateStep(vec4* src, vec4* targetStep, vec4* targetAccum, int iMinSlice, int iMaxSlice);

    //	Task handlers
    static void TaskDoPropagate(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount);
    static void TaskStep1(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount);
    static void TaskStepN(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount);

private:
    static const int m_nMaxPropagationSteps = 64;

    Buffer*                        m_ReadbackLightGrids[3];
    TextureFootprint               m_ReadbackFootprint;
    vec4*                          m_CPUGrids[9];
    ITASKSETHANDLE                 m_hLastTask;
    int                            m_nPropagationSteps;
    StepContext                    m_Contexts[m_nMaxPropagationSteps][3];
    LightPropagationCascade::State m_applyState;
};
} // namespace aura