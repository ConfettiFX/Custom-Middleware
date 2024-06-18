/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Aura.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#include "LightPropagationCPUContext.h"

#include "../../../The-Forge/Common_3/Resources/ResourceLoader/ThirdParty/OpenSource/tinyimageformat/tinyimageformat_apis.h"
#include "../../../The-Forge/Common_3/Resources/ResourceLoader/ThirdParty/OpenSource/tinyimageformat/tinyimageformat_query.h"

#define ARRAY_COUNT(a) sizeof(a) / sizeof(a[0])

#ifdef XBOX
#include <DirectXPackedVector.h>
#endif

#define NO_FSL_DEFINITIONS
#include "../Shaders/FSL/lightPropagation.h"
#include "../Shaders/FSL/lpvCommon.h"
#include "../Shaders/FSL/lpvSHMaths.h"

#if !defined(__linux__) && !defined(NX64) && !defined(__aarch64__)
// #ifndef __linux__
#define INTRIN_USE
#endif

#if defined(INTRIN_USE)
#include <emmintrin.h>
#include <immintrin.h>
#include <mmintrin.h>
#include <smmintrin.h>
#include <xmmintrin.h>
#endif

#if defined(__linux__) || defined(NX64) || defined(__APPLE__)
#define __declspec(x)
#define __forceinline __attribute__((always_inline)) inline
#endif

#ifdef ORBIS
#define __forceinline
extern void cmdCopyResourceOrbis(Cmd* p_cmd, const TextureDesc* pDesc, Texture* pSrc, Buffer* pDst);
#endif

#define USE_VIRTUAL_DIRECTIONS

extern PlatformParameters gPlatformParameters;

namespace aura
{
static void cmdCopyResource(Cmd* pCmd, const TextureDesc* pDesc, Texture* pSrc, Buffer* pDst);
static void queryTextureFootprint(const Renderer* pRenderer, const RenderTarget* pRT, TextureFootprint* pFootprint);

const uint32_t lpvElementCount = GridRes * GridRes * GridRes * 4;

void LightPropagationCPUContext::readData(Cmd* pCmd, Renderer* pRenderer, RenderTarget* m_LightGrids[3], uint32_t numGrids)
{
    UNREF_PARAM(pRenderer);
    UNREF_PARAM(numGrids);
    // Transition textures into copyable resource states.
    const int           numBarriers = NUM_GRIDS_PER_CASCADE;
    RenderTargetBarrier rtBarriers[numBarriers] = {};
    for (uint32_t i = 0; i < numBarriers; ++i)
    {
        rtBarriers[i] = { m_LightGrids[i], RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_COPY_SOURCE };
    }

    cmdResourceBarrier(pCmd, 0, NULL, 0, NULL, numBarriers, rtBarriers);

    // Copy the light grid textures into the CPU visible buffer.
    for (uint32_t i = 0; i < NUM_GRIDS_PER_CASCADE; ++i)
    {
        Texture*    lightGridTexture = m_LightGrids[i]->pTexture;
        TextureDesc desc = {};
        desc.mWidth = GridRes;
        desc.mHeight = GridRes;
        desc.mDepth = GridRes;
        desc.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
        cmdCopyResource(pCmd, &desc, lightGridTexture, m_ReadbackLightGrids[i]);
    }

    // Transition light grid textures back to original state.
    for (uint32_t i = 0; i < numBarriers; ++i)
    {
        rtBarriers[i].mCurrentState = RESOURCE_STATE_COPY_SOURCE;
        rtBarriers[i].mNewState = RESOURCE_STATE_RENDER_TARGET;
    }
    cmdResourceBarrier(pCmd, 0, NULL, 0, NULL, numBarriers, rtBarriers);
}

void LightPropagationCPUContext::processData(Renderer* pRenderer, ITaskManager* pTaskManager, MTTypes propagationMTType)
{
    convertGPUtoCPU(pRenderer);

    switch (propagationMTType)
    {
    case MT_None:
        doPropagate();
        break;
    case MT_ExtremeTasks:
        launchPropagateMultiTask(pTaskManager, 32);
        break;
    default:
        return;
    }
}

void LightPropagationCPUContext::applyData(Cmd* pCmd, Renderer* pRenderer, RenderTarget* m_LightGrids[3])
{
    UNREF_PARAM(pRenderer);
    cmdBeginDebugMarker(pCmd, 1.0, 0.0, 0.0, "Copy Barriers");
    RenderTargetBarrier rtBarriers[NUM_GRIDS_PER_CASCADE] = {};
    for (uint32_t i = 0; i < 3; ++i)
    {
        rtBarriers[i] = { m_LightGrids[i], RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_COPY_DEST };
    }
    cmdResourceBarrier(pCmd, 0, NULL, 0, NULL, NUM_GRIDS_PER_CASCADE, rtBarriers);
    cmdEndDebugMarker(pCmd);

    cmdBeginDebugMarker(pCmd, 1.0, 0.0, 0.0, "Copy to Light Grid Texture");
    for (uint32_t i = 0; i < NUM_GRIDS_PER_CASCADE; i++)
    {
        TextureUpdateDesc updateDesc = { m_LightGrids[i]->pTexture };
        updateDesc.mCurrentState = RESOURCE_STATE_COPY_DEST;
        updateDesc.pCmd = pCmd;
        beginUpdateResource(&updateDesc);
        TextureSubresourceUpdate subresource = updateDesc.getSubresourceUpdateDesc(0, 0);

        const uint32_t srcRowSize = GridRes * 4 * sizeof(float);
        const uint32_t srcSliceSize = srcRowSize * GridRes;

        for (uint32_t z = 0; z < GridRes; ++z)
        {
            uint8_t* dstSliceData = subresource.pMappedData + subresource.mDstSliceStride * z;
            uint8_t* srcSliceData = (uint8_t*)m_CPUGrids[i] + srcSliceSize * z;
            for (uint32_t r = 0; r < subresource.mRowCount; ++r)
            {
                half*  dstRowData = (half*)(dstSliceData + subresource.mDstRowStride * r);
                float* srcRowData = (float*)(srcSliceData + srcRowSize * r);
                for (uint32_t c = 0; c < GridRes * 4; ++c)
                {
                    dstRowData[c] = srcRowData[c];
                }
            }
        }

        endUpdateResource(&updateDesc);
    }
    cmdEndDebugMarker(pCmd);

    cmdBeginDebugMarker(pCmd, 1.0, 0.0, 0.0, "Original States");
    for (uint32_t i = 0; i < NUM_GRIDS_PER_CASCADE; ++i)
    {
        rtBarriers[i].mNewState = RESOURCE_STATE_RENDER_TARGET;
        rtBarriers[i].mCurrentState = RESOURCE_STATE_COPY_DEST;
    }
    cmdResourceBarrier(pCmd, 0, NULL, 0, NULL, NUM_GRIDS_PER_CASCADE, rtBarriers);
    cmdEndDebugMarker(pCmd);
}

void LightPropagationCPUContext::convertGPUtoCPU(Renderer* pRenderer)
{
    for (uint32_t i = 0; i < NUM_GRIDS_PER_CASCADE; i++)
    {
        ReadRange range = { 0, m_ReadbackFootprint.mTotalByteCount };
        mapBuffer(pRenderer, m_ReadbackLightGrids[i], &range);
        half* lightPropagationGridData = (half*)m_ReadbackLightGrids[i]->pCpuMappedAddress;
        if (lightPropagationGridData != NULL)
        {
            const size_t rowItemCount = (size_t)m_ReadbackFootprint.mRowPitch / sizeof(half);
            float*       floatBuf = (float*)m_CPUGrids[i];
            for (uint64_t yz = 0; yz < GridRes * GridRes; ++yz) // Y * Z
            {
                const half* src = lightPropagationGridData + yz * rowItemCount;
                for (uint32_t x = 0; x < GridRes * 4; ++x) // X * ChannelCount
                {
                    *floatBuf++ = *src++;
                }
            }

            unmapBuffer(pRenderer, m_ReadbackLightGrids[i]);
        }
    }
}

bool LightPropagationCPUContext::load(Renderer* pRenderer, RenderTarget* m_LightGrids[3])
{
    m_hLastTask = ITASKSETHANDLE_INVALID;
    m_nPropagationSteps = 12;

    for (uint32_t i = 0; i < ARRAY_COUNT(m_CPUGrids); ++i)
        m_CPUGrids[i] = 0;

    eState = APPLIED_PROPAGATION;

    queryTextureFootprint(pRenderer, m_LightGrids[0], &m_ReadbackFootprint);

    for (uint32_t i = 0; i < ARRAY_COUNT(m_ReadbackLightGrids); ++i)
    {
        BufferDesc readbackDesc = {};
        readbackDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_TO_CPU;
        readbackDesc.mFlags = BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
        readbackDesc.mStartState = ::RESOURCE_STATE_COPY_DEST;
        readbackDesc.mSize = m_ReadbackFootprint.mTotalByteCount;
        readbackDesc.mAlignment = 65536;
        readbackDesc.pName = "Readback Buffer";
        addBuffer(pRenderer, &readbackDesc, &m_ReadbackLightGrids[i]);
        ASSERT(m_ReadbackLightGrids[i]->mSize >= m_ReadbackFootprint.mTotalByteCount);
    }

    for (uint32_t i = 0; i < ARRAY_COUNT(m_CPUGrids); ++i)
        m_CPUGrids[i] = (vec4*)aura::alloc(lpvElementCount * sizeof(float));

    return true;
}

void LightPropagationCPUContext::unload(Renderer* pRenderer, ITaskManager* pTaskManager)
{
    if (m_hLastTask != ITASKSETHANDLE_INVALID)
    {
        pTaskManager->waitForTaskSet(m_hLastTask);
#if !defined(ORBIS_TASK_MANAGER)
        pTaskManager->releaseTask(m_hLastTask);
#endif
        m_hLastTask = ITASKSETHANDLE_INVALID;
    }

    SyncToLastTask(pTaskManager);

    for (uint32_t i = 0; i < ARRAY_COUNT(m_ReadbackLightGrids); i++)
    {
        mapBuffer(pRenderer, m_ReadbackLightGrids[i], NULL);
        void* readbackBuffer = m_ReadbackLightGrids[i]->pCpuMappedAddress;
        if (readbackBuffer != nullptr)
            unmapBuffer(pRenderer, m_ReadbackLightGrids[i]);
    }

    for (uint32_t i = 0; i < ARRAY_COUNT(m_ReadbackLightGrids); ++i)
    {
        removeBuffer(pRenderer, m_ReadbackLightGrids[i]);
    }

    for (uint32_t i = 0; i < ARRAY_COUNT(m_CPUGrids); ++i)
        aura::dealloc(m_CPUGrids[i]);
}

void LightPropagationCPUContext::launchPropagateSingleTask(ITaskManager* pTaskManager)
{
    pTaskManager->createTaskSet(0, TaskDoPropagate, this, 1, NULL, 0, "Single Task Propagate", &m_hLastTask);
}

void LightPropagationCPUContext::launchPropagateMultiTask(ITaskManager* pTaskManager, const int iTasksPerStep /*= 1*/)
{
    if (m_nPropagationSteps == 0)
    {
        return;
    }
    ITASKSETHANDLE m_hTask[m_nMaxPropagationSteps][3];

    int iSrc = 0;
    int iTargetStep = 1;
    int iTsrgetAccum = 2;
    {
        for (int j = 0; j < 3; ++j)
        {
            m_Contexts[0][j].pContext = this;
            m_Contexts[0][j].src = m_CPUGrids[iSrc * 3 + j];
            m_Contexts[0][j].targetStep = m_CPUGrids[iTargetStep * 3 + j];
            m_Contexts[0][j].targetAccum = m_CPUGrids[iTsrgetAccum * 3 + j];

            pTaskManager->createTaskSet(j, TaskStep1, &m_Contexts[0][j], iTasksPerStep, NULL, 0, "Propagate first step", &m_hTask[0][j]);
        }

        int pTmp;
        pTmp = iSrc;
        iSrc = iTargetStep;
        iTargetStep = pTmp;
    }

    static char taskLabel[m_nMaxPropagationSteps][256];

    for (int i = 1; i < m_nPropagationSteps; ++i)
    {
        snprintf(taskLabel[i], ARRAY_COUNT(taskLabel[i]), "Propagate step: %d", i);

        for (int j = 0; j < 3; ++j)
        {
            m_Contexts[i][j].pContext = this;
            m_Contexts[i][j].src = m_CPUGrids[iSrc * 3 + j];
            m_Contexts[i][j].targetStep = m_CPUGrids[iTargetStep * 3 + j];
            m_Contexts[i][j].targetAccum = m_CPUGrids[iTsrgetAccum * 3 + j];

            pTaskManager->createTaskSet(i - 1, TaskStepN, &m_Contexts[i][j], iTasksPerStep, &m_hTask[i - 1][j], 1, taskLabel[i],
                                        &m_hTask[i][j]);
        }

        int pTmp;
        pTmp = iSrc;
        iSrc = iTargetStep;
        iTargetStep = pTmp;
    }

    //	Swap src and accum
    for (int i = 0; i < 3; ++i)
    {
        vec4* pTmp;
        pTmp = m_CPUGrids[i];
        m_CPUGrids[i] = m_CPUGrids[i + 2 * 3];
        m_CPUGrids[i + 2 * 3] = pTmp;
    }

    pTaskManager->waitAll();

#if !defined(ORBIS_TASK_MANAGER)
    pTaskManager->releaseTasks(m_hTask[0], 3 * m_nPropagationSteps);
#endif
}

void LightPropagationCPUContext::doPropagate()
{
    for (int iChan = 0; iChan < 3; ++iChan)
    {
        if (m_UseAdvancedDirections)
        {
            propagateStep<true, true>(m_CPUGrids[iChan], m_CPUGrids[3], m_CPUGrids[4], 0, GridRes);
        }
        else
        {
            propagateStep<true, false>(m_CPUGrids[iChan], m_CPUGrids[3], m_CPUGrids[4], 0, GridRes);
        }

        vec4* pTmp;
        pTmp = m_CPUGrids[iChan];
        m_CPUGrids[iChan] = m_CPUGrids[4];
        m_CPUGrids[4] = pTmp;

        const int nPropagationSteps = m_nPropagationSteps;
        // const int nPropagationSteps = 16;

        //	Use ping-pong rt changes to propagate only previous step light
        for (int i = 1; i < nPropagationSteps; ++i)
        {
            if (m_UseAdvancedDirections)
            {
                propagateStep<false, true>(m_CPUGrids[3], m_CPUGrids[4], m_CPUGrids[iChan], 0, GridRes);
            }
            else
            {
                propagateStep<false, false>(m_CPUGrids[3], m_CPUGrids[4], m_CPUGrids[iChan], 0, GridRes);
            }

            pTmp = m_CPUGrids[3];
            m_CPUGrids[3] = m_CPUGrids[4];
            m_CPUGrids[4] = pTmp;
        }
    }

    // convertCPUtoGPU();
}
/************************************************************************/
// Math
/************************************************************************/
float4 SHRotate(float3 vcDir, const float2& vZHCoeffs)
{
    //	Igor: added this to fix singularity problem.
    if (1 - abs(vcDir.z) < 0.001)
    {
        vcDir.x = 0.0f;
        vcDir.y = 1.0f;
    }

    // compute sine and cosine of theta angle
    // beware of singularity when both x and y are 0 (no need to rotate at all)
    float2 theta12_cs = normalize(vcDir.xy());

    // compute sine and cosine of phi angle
    float2 phi12_cs;
    phi12_cs.x = sqrt(1.f - vcDir.z * vcDir.z);
    phi12_cs.y = vcDir.z;
    float4 vResult;

    // The first band is rotation-independent
    vResult.x = vZHCoeffs.x;
    // rotating the second band of SH
    vResult.y = -vZHCoeffs.y * phi12_cs.x * theta12_cs.y;
    vResult.z = vZHCoeffs.y * phi12_cs.y;
    vResult.w = -vZHCoeffs.y * phi12_cs.x * theta12_cs.x;
    return vResult;
}

inline float4 SHProjectCone(const float3& vcDir, const float angle)
{
    const float2 vZHCoeffs = SHProjectionScale * float2(.5f * (1.f - cos(angle)),         // 1/2 (1 - Cos[\[Alpha]])
                                                        0.75f * sin(angle) * sin(angle)); // 3/4 Sin[\[Alpha]]^2
    return SHRotate(vcDir, vZHCoeffs);
}

inline float4 Cone90Degree(const float3& vcDir) { return SHProjectCone(vcDir, float(PI / 4.0f)); }

DEFINE_ALIGNED(static const float3 vConeDirs[], 16) = {
    float3(1, 0, 0), float3(-1, 0, 0), float3(0, 1, 0), float3(0, -1, 0), float3(0, 0, 1), float3(0, 0, -1),
};

static const int inputOffset[] = {
    +1, -1, GridRes, -(int)GridRes, GridRes* GridRes, -(int)(GridRes* GridRes),
};

DEFINE_ALIGNED(static const float4 vCone90Degree[], 16) = {
    Cone90Degree(-vConeDirs[0]), Cone90Degree(-vConeDirs[1]), Cone90Degree(-vConeDirs[2]),
    Cone90Degree(-vConeDirs[3]), Cone90Degree(-vConeDirs[4]), Cone90Degree(-vConeDirs[5]),
};

#ifdef INTRIN_USE
inline float4 SHRotate(__m128 vcDirMM, const float2& vZHCoeffs)
{
    DEFINE_ALIGNED(float4, 16) vcDir;
    _mm_store_ps(&vcDir.x, vcDirMM);

    return SHRotate(vcDir.xyz(), vZHCoeffs);
}

__declspec(noalias) __forceinline __m128 IVPropagateDirIntrin(const float4& src, int dirIndex)
{
    // generate function for incoming direction from adjacent cell
    const float* pfCone = (float*)(&vCone90Degree[dirIndex].x);
    const __m128 shIncomingDirFunction = _mm_load_ps(pfCone);
    const __m128 zero = _mm_setzero_ps();
    const __m128 vsrc = _mm_load_ps((float*)(&src.x));

    /*
    //	dot SSE2
    const __m128 mul  = _mm_mul_ps( vsrc, shIncomingDirFunction); // { a,b,c,d }
    const __m128 hi   = _mm_movehl_ps(mul, mul); // { c,d,c,d }
    const __m128 add  = _mm_add_ps(mul, hi); // { a+c, b+d, c+c, d+d }
    const __m128 half = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(add), _MM_SHUFFLE(1, 1, 1, 1))); // { b+d }
    const __m128 join = _mm_add_ss(add, half); // { a+c+b+d }
    float fDot;
    _mm_store_ss(&fDot, join);

    if( fDot < 0.f ) return zero;

    __m128 vDot = _mm_load_ps1( &fDot );

    */

    //	dot SSE4
    const __m128 vDotTemp = _mm_dp_ps(vsrc, shIncomingDirFunction, 0xFF);

    const __m128 vDot = _mm_max_ps(vDotTemp, zero);

    /**/

    return _mm_mul_ps(vDot, shIncomingDirFunction);
}

#if defined(USE_VIRTUAL_DIRECTIONS)

__declspec(noalias) inline float getSolidAngle(const __m128 dir, const __m128 faceDir)
{
    //	4 faces of this kind
    const float faceASolidAngle = 0.42343134f;
    //	1 face of this kind
    const float faceBSolidAngle = 0.40066966f;

    //	dot SSE4
    const __m128 faceType = _mm_dp_ps(dir, faceDir, 0xFF);

    float faceTypeF = 0.0f;
    _mm_store_ss(&faceTypeF, faceType);

    if (faceTypeF < -0.01f) //	Front face. Light enters cube through this face
        return 0.0f;
    else if (faceTypeF > 0.01f) //	Back face
        return faceBSolidAngle;
    else
        return faceASolidAngle; //	Side face
}

__declspec(noalias) inline __m128 SHEvaluateFunction(const __m128 vcDir, const __m128 data)
{
    // return dot(data, float4(1.0f, vcDir.y, vcDir.z, vcDir.x)*SHBasis);

    const float one = 1.0f;

    const __m128 shBasis = _mm_load_ps(SHBasis);
    const __m128 ones = _mm_load_ps1(&one);

    __m128 permVcDir = _mm_shuffle_ps(vcDir, vcDir, _MM_SHUFFLE(0, 2, 1, 3)); // [0 vcDir.y vcDir.z vcDir.x]
    permVcDir = _mm_move_ss(permVcDir, ones);                                 // [1 vcDir.y vcDir.z vcDir.x]

    return _mm_dp_ps(data, _mm_mul_ps(permVcDir, shBasis), 0xFF);
}

// Cosine lobe
__declspec(noalias) inline __m128 SHProjectCone(const __m128 vcDir)
{
    const float2 vZHCoeffs = SHProjectionScale * float2(0.25f, 0.5f);

    DEFINE_ALIGNED(float4, 16) rot = SHRotate(vcDir, vZHCoeffs);

    return _mm_load_ps(&rot.x);
}

__declspec(noalias) inline __m128 SSENormalize(const __m128 vec)
{
    __m128 dp = _mm_dp_ps(vec, vec, 0x7F);
    dp = _mm_rsqrt_ps(dp);
    return _mm_mul_ps(vec, dp);
}

__declspec(noalias) inline __m128 IVPropagateVirtualDirIntrin(const float4& srcF4, int dirIndex, int virtualDirIndex)
{
    const float   zeroPoint5F = 0.5f;
    const float3& nOffsetFloat3 = vConeDirs[dirIndex];
    const float3& virtDirFloat3 = vConeDirs[virtualDirIndex];

    DEFINE_ALIGNED(const float nOffsetArray[], 16) = { -nOffsetFloat3.x, -nOffsetFloat3.y, -nOffsetFloat3.z, 0.0f };
    DEFINE_ALIGNED(const float virtDirArray[], 16) = { virtDirFloat3.x, virtDirFloat3.y, virtDirFloat3.z, 0.0f };

    __m128 nOffset = _mm_load_ps(nOffsetArray);
    __m128 virtDir = _mm_load_ps(virtDirArray);

    float solidAngle = getSolidAngle(nOffset, virtDir);

    if (solidAngle <= 0)
        return _mm_setzero_ps();

    __m128 zeroPoint5 = _mm_load_ss(&zeroPoint5F);
    __m128 virtDirDiv2 = _mm_mul_ss(zeroPoint5, virtDir);

    const __m128 propDir = SSENormalize(_mm_add_ps(nOffset, virtDirDiv2));
    const __m128 src = _mm_load_ps(&srcF4.x);

    const float  propagationFactorF = solidAngle * 0.5f; //(4*PI);
    const __m128 propagationFactor = _mm_load_ps1(&propagationFactorF);
    // const float reprojectionFactor = 1.0f;// /PI;
    // const float reprojLuminance = propagationFactor * reprojectionFactor * max(SHEvaluateFunction(propDir, src), 0.0f);

    const __m128 reprojLuminance = _mm_mul_ps(propagationFactor, _mm_max_ps(SHEvaluateFunction(propDir, src), _mm_setzero_ps()));

    const __m128 shIncomingDirFunction = SHProjectCone(virtDir);

    return _mm_mul_ps(shIncomingDirFunction, reprojLuminance);
}

__declspec(noalias) inline __m128 IVPropagateDirAdvancedIntrin(const float4& src, int dirIndex)
{
    __m128 res = _mm_setzero_ps();

    res = _mm_add_ps(res, IVPropagateVirtualDirIntrin(src, dirIndex, 0));
    res = _mm_add_ps(res, IVPropagateVirtualDirIntrin(src, dirIndex, 1));
    res = _mm_add_ps(res, IVPropagateVirtualDirIntrin(src, dirIndex, 2));
    res = _mm_add_ps(res, IVPropagateVirtualDirIntrin(src, dirIndex, 3));
    res = _mm_add_ps(res, IVPropagateVirtualDirIntrin(src, dirIndex, 4));
    res = _mm_add_ps(res, IVPropagateVirtualDirIntrin(src, dirIndex, 5));

    return res;
}

#endif // USE_VIRTUAL_DIRECTIONS

template<bool bFirstStep, bool isAdvanced, bool isIMin, bool isIMax, bool isJMin, bool isJMax, bool isKMin, bool isKMax>
__declspec(noalias) __forceinline void propagateCell(vec4* __restrict src, vec4* __restrict targetStep, vec4* __restrict targetAccum,
                                                     const int i, const int j, const int k, const int readOffset)
{
    UNREF_PARAM(i);
    UNREF_PARAM(i);
    UNREF_PARAM(j);
    UNREF_PARAM(k);
    __m128 res = _mm_setzero_ps();

    if (isAdvanced)
    {
        // if (k<GridRes-1)
        if (!isKMax)
            res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[0]], 0));
        //	float3(-1, 0, 0),
        // if (k>0)
        if (!isKMin)
            res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[1]], 1));
        // float3( 0, 1, 0),
        // if (j<GridRes-1)
        if (!isJMax)
            res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[2]], 2));
        // float3( 0, -1, 0),
        // if (j>0)
        if (!isJMin)
            res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[3]], 3));
        // float3( 0, 0, 1),
        // if (i<GridRes-1)
        if (!isIMax)
            res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[4]], 4));
        // float3( 0, 0, -1),
        // if (i>0)
        if (!isIMin)
            res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[5]], 5));
    }
    else
    {
        // if (k<GridRes-1)
        if (!isKMax)
            res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[0]], 0));
        //	float3(-1, 0, 0),
        // if (k>0)
        if (!isKMin)
            res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[1]], 1));
        // float3( 0, 1, 0),
        // if (j<GridRes-1)
        if (!isJMax)
            res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[2]], 2));
        // float3( 0, -1, 0),
        // if (j>0)
        if (!isJMin)
            res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[3]], 3));
        // float3( 0, 0, 1),
        // if (i<GridRes-1)
        if (!isIMax)
            res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[4]], 4));
        // float3( 0, 0, -1),
        // if (i>0)
        if (!isIMin)
            res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[5]], 5));
    }

    _mm_store_ps((float*)(targetStep + readOffset), res);

    if (bFirstStep)
    {
        __m128 vsrc = _mm_load_ps((float*)(src + readOffset));
        __m128 vAccum = _mm_add_ps(res, vsrc);
        _mm_store_ps((float*)(targetAccum + readOffset), vAccum);
    }
    else
    {
        __m128 vAccum = _mm_load_ps((float*)(targetAccum + readOffset));
        vAccum = _mm_add_ps(res, vAccum);
        _mm_store_ps((float*)(targetAccum + readOffset), vAccum);
    }
}
#endif

#if !defined(INTRIN_USE)

__forceinline float4 IVPropagateDir(const float4& src, int dirIndex)
{
    // generate function for incoming direction from adjacent cell
    float4 shIncomingDirFunction = vCone90Degree[dirIndex];

    // integrate incoming radiance with this function
    float incidentLuminance = max(0.0f, dot(src, shIncomingDirFunction));

    return shIncomingDirFunction * incidentLuminance;
}

inline float getSolidAngle(const float3& dir, const float3& faceDir)
{
    //	4 faces of this kind
    const float faceASolidAngle = 0.42343134f;
    //	1 face of this kind
    const float faceBSolidAngle = 0.40066966f;

    float faceType = dot(dir, faceDir);

    //	Front face. Light enters cube through this face
    if (faceType < -0.01f)
        return 0.0f;
    //	Back face
    else if (faceType > 0.01f)
        return faceBSolidAngle;
    //	Side face
    else
        return faceASolidAngle;
}

inline float SHEvaluateFunction(const float3& vcDir, const float4& data)
{
    return dot(data, float4(1.0f, vcDir.y, vcDir.z, vcDir.x) * SHBasis);
}

// Cosine lobe
__forceinline float4 SHProjectCone(const float3& vcDir)
{
    const float2 vZHCoeffs = SHProjectionScale * float2(0.25f, 0.5f);
    return SHRotate(vcDir, vZHCoeffs);
}

__forceinline float4 IVPropagateVirtualDir(const float4& src, int dirIndex, int virtualDirIndex)
{
    const float3& nOffset = vConeDirs[dirIndex];
    const float3& virtDir = vConeDirs[virtualDirIndex];

    float solidAngle = getSolidAngle(-nOffset, virtDir);

    if (solidAngle <= 0)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);

    const float3 propDir = normalize(-nOffset + 0.5f * virtDir);

    const float propagationFactor = solidAngle * 0.5f; //(4*PI);
    const float reprojectionFactor = 1.0f;             // /PI;
    const float reprojLuminance = propagationFactor * reprojectionFactor * max(SHEvaluateFunction(propDir, src), 0.0f);

    float4 shIncomingDirFunction = SHProjectCone(virtDir);

    return shIncomingDirFunction * reprojLuminance;
}

template<bool isIMin, bool isIMax, bool isJMin, bool isJMax, bool isKMin, bool isKMax>
__forceinline float4 IVPropagateDirAdvanced(const float4& src, int dirIndex)
{
    float4 res = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (!isKMax)
        res += IVPropagateVirtualDir(src, dirIndex, 0);
    if (!isKMin)
        res += IVPropagateVirtualDir(src, dirIndex, 1);
    if (!isJMax)
        res += IVPropagateVirtualDir(src, dirIndex, 2);
    if (!isJMin)
        res += IVPropagateVirtualDir(src, dirIndex, 3);
    if (!isIMax)
        res += IVPropagateVirtualDir(src, dirIndex, 4);
    if (!isIMin)
        res += IVPropagateVirtualDir(src, dirIndex, 5);

    return res;
}

template<bool bFirstStep, bool isAdvanced, bool isIMin, bool isIMax, bool isJMin, bool isJMax, bool isKMin, bool isKMax>
__declspec(noalias) __forceinline void propagateCell(vec4* __restrict src, vec4* __restrict targetStep, vec4* __restrict targetAccum,
                                                     const int i, const int j, const int k, const int readOffset)
{
    float4 res = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (isAdvanced)
    {
        // VIRTUAL DIRECTIONS
        if (!isKMax)
            res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[0]], 0);
        //	float3(-1, 0, 0),
        if (!isKMin)
            res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[1]], 1);
        // float3( 0, 1, 0),
        if (!isJMax)
            res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[2]], 2);
        // float3( 0, -1, 0),
        if (!isJMin)
            res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[3]], 3);
        // float3( 0, 0, 1),
        if (!isIMax)
            res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[4]], 4);
        // float3( 0, 0, -1),
        if (!isIMin)
            res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[5]], 5);
    }
    else
    {
        // if (k<GridRes-1)
        if (!isKMax)
            res += IVPropagateDir(src[readOffset + inputOffset[0]], 0);
        //	float3(-1, 0, 0),
        // if (k>0)
        if (!isKMin)
            res += IVPropagateDir(src[readOffset + inputOffset[1]], 1);
        // float3( 0, 1, 0),
        // if (j<GridRes-1)
        if (!isJMax)
            res += IVPropagateDir(src[readOffset + inputOffset[2]], 2);
        // float3( 0, -1, 0),
        // if (j>0)
        if (!isJMin)
            res += IVPropagateDir(src[readOffset + inputOffset[3]], 3);
        // float3( 0, 0, 1),
        // if (i<GridRes-1)
        if (!isIMax)
            res += IVPropagateDir(src[readOffset + inputOffset[4]], 4);
        // float3( 0, 0, -1),
        // if (i>0)
        if (!isIMin)
            res += IVPropagateDir(src[readOffset + inputOffset[5]], 5);
    }

    targetStep[readOffset] = res;

    if (bFirstStep)
        targetAccum[readOffset] = res + src[readOffset];
    else
        targetAccum[readOffset] += res;
}

#endif

/************************************************************************/
/************************************************************************/
void LightPropagationCPUContext::SyncToLastTask(ITaskManager* pTaskManager)
{
    if (m_hLastTask != ITASKSETHANDLE_INVALID)
    {
        pTaskManager->waitForTaskSet(m_hLastTask);
#if !defined(ORBIS_TASK_MANAGER)
        pTaskManager->releaseTask(m_hLastTask);
#endif
        m_hLastTask = ITASKSETHANDLE_INVALID;
    }
}
/************************************************************************/
// Templates
/************************************************************************/
template<bool bFirstStep, bool isAdvanced, bool isIMin, bool isIMax, bool isJMin, bool isJMax>
__declspec(noalias) __forceinline void propagateRow(vec4* __restrict src, vec4* __restrict targetStep, vec4* __restrict targetAccum,
                                                    const int i, const int j, int& readOffset)
{
    //	Igor: partially unroll the loop. This unroll ifs too.
    propagateCell<bFirstStep, isAdvanced, isIMin, isIMax, isJMin, isJMax, true, false>(src, targetStep, targetAccum, i, j, 0, readOffset);
    ++readOffset;

    for (int k = 1; k < static_cast<int>(GridRes - 1); ++k, ++readOffset)
    {
        propagateCell<bFirstStep, isAdvanced, isIMin, isIMax, isJMin, isJMax, false, false>(src, targetStep, targetAccum, i, j, k,
                                                                                            readOffset);
    }

    propagateCell<bFirstStep, isAdvanced, isIMin, isIMax, isJMin, isJMax, false, true>(src, targetStep, targetAccum, i, j, GridRes - 1,
                                                                                       readOffset);
    ++readOffset;
}

template<bool bFirstStep, bool isAdvanced, bool isIMin, bool isIMax>
__declspec(noalias) __forceinline void propagateSlice(vec4* __restrict src, vec4* __restrict targetStep, vec4* __restrict targetAccum,
                                                      const int i, int& readOffset)
{
    propagateRow<bFirstStep, isAdvanced, isIMin, isIMax, true, false>(src, targetStep, targetAccum, i, 0, readOffset);

    for (int j = 1; j < static_cast<int>(GridRes - 1); ++j)
    {
        propagateRow<bFirstStep, isAdvanced, isIMin, isIMax, false, false>(src, targetStep, targetAccum, i, j, readOffset);
    }

    propagateRow<bFirstStep, isAdvanced, isIMin, isIMax, false, true>(src, targetStep, targetAccum, i, GridRes - 1, readOffset);
}

template<bool bFirstStep, bool isAdvanced>
__declspec(noalias) void LightPropagationCPUContext::propagateStep(vec4* __restrict src, vec4* __restrict targetStep,
                                                                   vec4* __restrict targetAccum, int iMinSlice /*=0*/,
                                                                   int iMaxSlice /*=GridRes*/)
{
    int readOffset = iMinSlice * (GridRes * GridRes);

    bool bLastSlice = false;

    if (iMaxSlice == GridRes)
    {
        bLastSlice = true;
        --iMaxSlice;
    }

    if (iMinSlice == 0)
    {
        ++iMinSlice;
        propagateSlice<bFirstStep, isAdvanced, true, false>(src, targetStep, targetAccum, 0, readOffset);
    }

    for (int i = iMinSlice; i < iMaxSlice; ++i)
    {
        propagateSlice<bFirstStep, isAdvanced, false, false>(src, targetStep, targetAccum, i, readOffset);
    }

    if (bLastSlice)
    {
        propagateSlice<bFirstStep, isAdvanced, false, true>(src, targetStep, targetAccum, GridRes - 1, readOffset);
    }
}

/************************************************************************/
// Task callbacks
/************************************************************************/
void LightPropagationCPUContext::TaskDoPropagate(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount)
{
    UNREF_PARAM(iContext);
    UNREF_PARAM(uTaskId);
    UNREF_PARAM(uTaskCount);
    ((LightPropagationCPUContext*)pvInfo)->doPropagate();
}

// On Apple Arm64 light propagation causes heap overflows which trigger ASAN errors/crashes,
// so it is disabled for the time being. Since there are still issues in the math on Apple Arm64,
// the output is unaffected by disabling the propagation code (i.e. the outout is empty).
#if defined(__APPLE__) && defined(__aarch64__)
#define TEMP_DISABLE_CPU_PROPAGATION
#endif

void LightPropagationCPUContext::TaskStep1(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount)
{
    UNREF_PARAM(iContext);
#ifndef TEMP_DISABLE_CPU_PROPAGATION
    int iMinSlice = uTaskId * GridRes / uTaskCount;
    int iMaxSlice = (uTaskId + 1) * GridRes / uTaskCount;

    StepContext* pContext = (StepContext*)pvInfo;
    if (pContext->pContext->m_UseAdvancedDirections)
    {
        pContext->pContext->propagateStep<true, true>(pContext->src, pContext->targetStep, pContext->targetAccum, iMinSlice, iMaxSlice);
    }
    else
    {
        pContext->pContext->propagateStep<true, false>(pContext->src, pContext->targetStep, pContext->targetAccum, iMinSlice, iMaxSlice);
    }
#endif
}

void LightPropagationCPUContext::TaskStepN(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount)
{
    UNREF_PARAM(iContext);
#ifndef TEMP_DISABLE_CPU_PROPAGATION
    int iMinSlice = uTaskId * GridRes / uTaskCount;
    int iMaxSlice = (uTaskId + 1) * GridRes / uTaskCount;

    StepContext* pContext = (StepContext*)pvInfo;
    if (pContext->pContext->m_UseAdvancedDirections)
    {
        pContext->pContext->propagateStep<false, true>(pContext->src, pContext->targetStep, pContext->targetAccum, iMinSlice, iMaxSlice);
    }
    else
    {
        pContext->pContext->propagateStep<false, false>(pContext->src, pContext->targetStep, pContext->targetAccum, iMinSlice, iMaxSlice);
    }

#endif
}

static void queryTextureFootprint(const Renderer* pRenderer, const RenderTarget* pRT, TextureFootprint* pFootprint)
{
    ASSERT(pFootprint);
    ASSERT(pRT);
#if defined(DIRECT3D12)
    if (gPlatformParameters.mSelectedRendererApi == RENDERER_API_D3D12)
    {
        D3D12_RESOURCE_DESC                desc = pRT->pTexture->mDx.pResource->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
        ID3D12Device* const                pDevice = pRenderer->mDx.pDevice;
        pDevice->GetCopyableFootprints(&desc, 0, 1, 0, &layout, NULL, NULL, NULL);
        pFootprint->mTotalByteCount =
            (uint64_t)layout.Footprint.RowPitch * (uint64_t)layout.Footprint.Height * (uint64_t)layout.Footprint.Depth;
        pFootprint->mRowPitch = (uint64_t)layout.Footprint.RowPitch;
        return;
    }
#endif
    const uint64_t bpp = TinyImageFormat_BitSizeOfBlock((TinyImageFormat)pRT->mFormat) / 8;
    const uint64_t rowPitch = (uint64_t)pRT->mWidth * bpp;
    const uint64_t totalBytes = rowPitch * (uint64_t)pRT->mHeight * (uint64_t)pRT->mDepth;
    pFootprint->mTotalByteCount = (uint64_t)totalBytes;
    pFootprint->mRowPitch = (uint64_t)rowPitch;
}

static void cmdCopyResource(Cmd* p_cmd, const TextureDesc* pDesc, Texture* pSrc, Buffer* pDst)
{
    ASSERT(p_cmd);
    ASSERT(pDst);
    ASSERT(pSrc);
    ASSERT(pDesc);

#if defined(DIRECT3D12)
    if (gPlatformParameters.mSelectedRendererApi == RENDERER_API_D3D12)
    {
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
        D3D12_RESOURCE_DESC                Desc = pSrc->mDx.pResource->GetDesc();
        p_cmd->pRenderer->mDx.pDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &layout, NULL, NULL, NULL);
        D3D12_TEXTURE_COPY_LOCATION Src = {};
        Src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        Src.pResource = pSrc->mDx.pResource;
        Src.SubresourceIndex = 0;
        D3D12_TEXTURE_COPY_LOCATION Dst = {};
        Dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        Dst.pResource = pDst->mDx.pResource;
        Dst.PlacedFootprint = layout;
        p_cmd->mDx.pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
    }
#endif
#if defined(VULKAN)
    if (gPlatformParameters.mSelectedRendererApi == RENDERER_API_VULKAN)
    {
        // Vulkan docs guarantee we have tightly packed memory:
        // "bufferRowLength and bufferImageHeight specify in texels a subregion of a larger 2D or 3D image
        // buffer memory, and control the addressing calculations.
        // If either of these values is zero, that aspect of the buffer memory
        // is considered to be tightly packed according to the imageExtent."
        VkBufferImageCopy copy = {};
        copy.bufferOffset = 0;
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;
        copy.imageSubresource.aspectMask = (VkImageAspectFlags)pSrc->mAspectMask;
        copy.imageSubresource.mipLevel = 0;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.imageSubresource.layerCount = 1;
        copy.imageOffset.x = 0;
        copy.imageOffset.y = 0;
        copy.imageOffset.z = 0;
        copy.imageExtent.width = pDesc->mWidth;
        copy.imageExtent.height = pDesc->mHeight;
        copy.imageExtent.depth = pDesc->mDepth;
        vkCmdCopyImageToBuffer(p_cmd->mVk.pCmdBuf, pSrc->mVk.pImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pDst->mVk.pBuffer, 1, &copy);
    }
#endif
#if defined(ORBIS)
    cmdCopyResourceOrbis(p_cmd, pDesc, pSrc, pDst);
#endif
#if defined(PROSPERO)
    // not supported
#endif
}
} // namespace aura
