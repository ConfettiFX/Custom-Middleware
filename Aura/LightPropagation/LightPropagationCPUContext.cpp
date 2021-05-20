/*
 * Copyright © 2018-2021 Confetti Interactive Inc.
 *
 * This is a part of Aura.
 * 
 * This file(code) is licensed under a 
 * Creative Commons Attribution-NonCommercial 4.0 International License 
 *
 *   (https://creativecommons.org/licenses/by-nc/4.0/legalcode) 
 *
 * Based on a work at https://github.com/ConfettiFX/The-Forge.
 * You may not use the material for commercial purposes.
 *
*/

#include "LightPropagationCPUContext.h"

using aura::float4;
#define ARRAY_COUNT(a) sizeof(a) / sizeof(a[0])

#ifdef XBOX
#include <DirectXPackedVector.h>
#endif

#define NO_FSL_DEFINITIONS
#include "../Shaders/FSL/lpvCommon.h"
#include "../Shaders/FSL/lightPropagation.h"
#include "../Shaders/FSL/lpvSHMaths.h"

#if !defined(__linux__) && !defined(NX64)
//#ifndef __linux__
#define INTRIN_USE
#endif

#if defined(INTRIN_USE)
#include <xmmintrin.h>
#include <mmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>
#endif

#if defined(_WIN32)
#define DEFINE_ALIGNED(def, a) __declspec(align(a)) def
#elif defined(__OSX__)
#define DEFINE_ALIGNED(def, a) def __attribute__((aligned(a)))
#else
#define DEFINE_ALIGNED(def, a) alignas(a) def
#endif

#if defined(__linux__) || defined(NX64) || defined(__APPLE__)
#define __declspec(x)
#define __forceinline __attribute__((always_inline))
#endif

#if defined ORBIS
#define __forceinline
#endif

// #define USE_VIRTUAL_DIRECTIONS

namespace aura {

	const uint32_t lpvElementCount = GridRes * GridRes * GridRes * 4;
	const uint64_t lpvByteCount = lpvElementCount * sizeof(half);

	void LightPropagationCPUContext::readData(Cmd* pCmd, Renderer* pRenderer, RenderTarget* m_LightGrids[3], uint32_t numGrids)
	{
		// Transition textures into copyable resource states. 
		const int numBarriers = NUM_GRIDS_PER_CASCADE;
		TextureBarrier textureBarriers[numBarriers] = {};
		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			getTextureFromRenderTarget(m_LightGrids[i], &textureBarriers[i].pTexture);
			textureBarriers[i].mNewState = RESOURCE_STATE_COPY_SOURCE;
			textureBarriers[i].mCurrentState = RESOURCE_STATE_RENDER_TARGET;
		}

		cmdResourceBarrier(pCmd, 0, NULL, numBarriers, textureBarriers, 0, NULL);

		// Copy the light grid textures into the CPU visible buffer.
		for (uint32_t i = 0; i < NUM_GRIDS_PER_CASCADE; ++i)
		{
			Texture* lightGridTexture;
			getTextureFromRenderTarget(m_LightGrids[i], &lightGridTexture);
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
			textureBarriers[i].mCurrentState = RESOURCE_STATE_COPY_SOURCE;
			textureBarriers[i].mNewState = RESOURCE_STATE_RENDER_TARGET;
		}
		cmdResourceBarrier(pCmd, 0, NULL, numBarriers, textureBarriers, 0, NULL);
	}

	void LightPropagationCPUContext::processData(Renderer* pRenderer, ITaskManager* pTaskManager, MTTypes propagationMTType)
	{
		convertGPUtoCPU();

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
		cmdBeginDebugMarker(pCmd, 1.0, 0.0, 0.0, "Copy Barriers");
		TextureBarrier textureBarriers[NUM_GRIDS_PER_CASCADE] = {};
		for (uint32_t i = 0; i < 3; ++i)
		{
			getTextureFromRenderTarget(m_LightGrids[i], &textureBarriers[i].pTexture);
			textureBarriers[i].mNewState = RESOURCE_STATE_COPY_DEST;
			textureBarriers[i].mCurrentState = RESOURCE_STATE_RENDER_TARGET;
		}
		cmdResourceBarrier(pCmd, 0, NULL, NUM_GRIDS_PER_CASCADE, textureBarriers, 0, NULL);
		cmdEndDebugMarker(pCmd);

		cmdBeginDebugMarker(pCmd, 1.0, 0.0, 0.0, "Copy to Light Grid Texture");
		for (uint32_t i = 0; i < NUM_GRIDS_PER_CASCADE; i++)
		{
			half* readbackBuffer;
			getCpuMappedAddress(m_ReadbackLightGrids[i], (void**)&readbackBuffer);

			float* floatBuffer = (float*)m_CPUGrids[i];
			for (uint j = 0; j < lpvElementCount; j++)
			{
				readbackBuffer[j] = half(floatBuffer[j]);
			}

			half* uploadBuffer;
			getCpuMappedAddress(m_UploadLightGrids[i], (void**)&uploadBuffer);

			memcpy(uploadBuffer, readbackBuffer, lpvByteCount);

			Texture* lightGridTexture;
			getTextureFromRenderTarget(m_LightGrids[i], &lightGridTexture);
			cmdCopyResource(pCmd, m_UploadLightGrids[i], lightGridTexture);

			unmapBuffer(pRenderer, m_ReadbackLightGrids[i]);
		}
		cmdEndDebugMarker(pCmd);

		cmdBeginDebugMarker(pCmd, 1.0, 0.0, 0.0, "Original States");
		for (uint32_t i = 0; i < NUM_GRIDS_PER_CASCADE; ++i)
		{
			textureBarriers[i].mNewState = RESOURCE_STATE_RENDER_TARGET;
			textureBarriers[i].mCurrentState = RESOURCE_STATE_COPY_DEST;
		}
		cmdResourceBarrier(pCmd, 0, NULL, NUM_GRIDS_PER_CASCADE, textureBarriers, 0, NULL);
		cmdEndDebugMarker(pCmd);
	}

	void LightPropagationCPUContext::convertGPUtoCPU()
	{
		for (uint32_t i = 0; i < NUM_GRIDS_PER_CASCADE; i++)
		{
			half* lightPropagationGridData;
			getCpuMappedAddress(m_ReadbackLightGrids[i], (void**)&lightPropagationGridData);

			if (lightPropagationGridData != NULL)
			{
				float* floatBuf = (float*)m_CPUGrids[i];
				for (int j = 0; j < lpvElementCount; ++j) {
					floatBuf[j] = lightPropagationGridData[j];
				}
			}
		}
	}

	bool LightPropagationCPUContext::load(Renderer* pRenderer)
	{
		m_hLastTask = ITASKSETHANDLE_INVALID;
		m_nPropagationSteps = 12;

		for (int i = 0; i < ARRAY_COUNT(m_CPUGrids); ++i)
			m_CPUGrids[i] = 0;

		eState = APPLIED_PROPAGATION;

		for (int i = 0; i < ARRAY_COUNT(m_ReadbackLightGrids); ++i)
		{
			addReadbackBuffer(pRenderer, lpvByteCount, &m_ReadbackLightGrids[i]);
			addUploadBuffer(pRenderer, lpvByteCount, &m_UploadLightGrids[i]);
		}

		for (int i = 0; i < ARRAY_COUNT(m_CPUGrids); ++i)
			m_CPUGrids[i] = (vec4*)aura::alloc(lpvElementCount * sizeof(vec4));

		return true;
	}

	void LightPropagationCPUContext::unload(Renderer* pRenderer, ITaskManager* pTaskManager)
	{
		if (m_hLastTask != ITASKSETHANDLE_INVALID)
		{
			pTaskManager->waitForTaskSet(m_hLastTask);
#if !defined (ORBIS_TASK_MANAGER)
			pTaskManager->releaseTask(m_hLastTask);
#endif
			m_hLastTask = ITASKSETHANDLE_INVALID;
		}

		SyncToLastTask(pTaskManager);

		for (int i = 0; i < ARRAY_COUNT(m_ReadbackLightGrids); i++)
		{
			void* readbackBuffer;
			getCpuMappedAddress(m_ReadbackLightGrids[i], (void**)&readbackBuffer);
			if (readbackBuffer != nullptr)
				unmapBuffer(pRenderer, m_ReadbackLightGrids[i]);
		}

		for (int i = 0; i < ARRAY_COUNT(m_ReadbackLightGrids); ++i)
		{
			removeBuffer(pRenderer, m_ReadbackLightGrids[i]);
			removeBuffer(pRenderer, m_UploadLightGrids[i]);
		}

		for (int i = 0; i < ARRAY_COUNT(m_CPUGrids); ++i)
			aura::dealloc(m_CPUGrids[i]);
	}

	void LightPropagationCPUContext::launchPropagateSingleTask(ITaskManager* pTaskManager)
	{
		pTaskManager->createTaskSet(0,
			TaskDoPropagate,
			this,
			1,
			NULL,
			0,
			"Single Task Propagate",
			&m_hLastTask);
	}

	void LightPropagationCPUContext::launchPropagateMultiTask(ITaskManager* pTaskManager, const int iTasksPerStep /*= 1*/)
	{
		if (m_nPropagationSteps == 0) { return; }
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

				pTaskManager->createTaskSet(j,
					TaskStep1,
					&m_Contexts[0][j],
					iTasksPerStep,
					NULL,
					0,
					"Propagate first step",
					&m_hTask[0][j]);
			}

			int pTmp;
			pTmp = iSrc;
			iSrc = iTargetStep;
			iTargetStep = pTmp;
		}

		static char taskLabel[m_nMaxPropagationSteps][256];

		unsigned int dependency[3];
		for (int i = 1; i < m_nPropagationSteps; ++i)
		{
			snprintf(taskLabel[i], ARRAY_COUNT(taskLabel[i]), "Propagate step: %d", i);

			for (int j = 0; j < 3; ++j)
			{
				m_Contexts[i][j].pContext = this;
				m_Contexts[i][j].src = m_CPUGrids[iSrc * 3 + j];
				m_Contexts[i][j].targetStep = m_CPUGrids[iTargetStep * 3 + j];
				m_Contexts[i][j].targetAccum = m_CPUGrids[iTsrgetAccum * 3 + j];

				dependency[0] = (i - 1) * 3 + j;
				pTaskManager->createTaskSet(i - 1,
					TaskStepN,
					&m_Contexts[i][j],
					iTasksPerStep,
					&m_hTask[i - 1][j],
					1,
					taskLabel[i],
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
			vec4 *pTmp;
			pTmp = m_CPUGrids[i];
			m_CPUGrids[i] = m_CPUGrids[i + 2 * 3];
			m_CPUGrids[i + 2 * 3] = pTmp;
		}

		dependency[0] = (m_nPropagationSteps - 1) * 3 + 0;
		dependency[1] = (m_nPropagationSteps - 1) * 3 + 1;
		dependency[2] = (m_nPropagationSteps - 1) * 3 + 2;

		pTaskManager->waitAll();


#if !defined (ORBIS_TASK_MANAGER)
		pTaskManager->releaseTasks(m_hTask[0], 3 * m_nPropagationSteps);
#endif
	}

	void LightPropagationCPUContext::doPropagate()
	{
		for (int iChan = 0; iChan < 3; ++iChan)
		{
			propagateStep<true>(m_CPUGrids[iChan], m_CPUGrids[3], m_CPUGrids[4], 0, GridRes);

			vec4	*pTmp;
			pTmp = m_CPUGrids[iChan];
			m_CPUGrids[iChan] = m_CPUGrids[4];
			m_CPUGrids[4] = pTmp;

			const int nPropagationSteps = m_nPropagationSteps;
			//const int nPropagationSteps = 16;

			//	Use ping-pong rt changes to propagate only previous step light
			for (int i = 1; i < nPropagationSteps; ++i)
			{
				propagateStep<false>(m_CPUGrids[3], m_CPUGrids[4], m_CPUGrids[iChan], 0, GridRes);

				vec4	*pTmp;
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
	float4 SHRotate(float3 vcDir, const float2 &vZHCoeffs)
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

	inline float4 SHProjectCone(const float3 &vcDir, const float angle)
	{
		const float2 vZHCoeffs = SHProjectionScale *
			float2(
				.5f * (1.f - cos(angle)), // 1/2 (1 - Cos[\[Alpha]]) 
				0.75f * sin(angle) * sin(angle)); // 3/4 Sin[\[Alpha]]^2 
		return SHRotate(vcDir, vZHCoeffs);
	}

	inline float4 Cone90Degree(const float3 &vcDir)
	{
		return SHProjectCone(vcDir, float(PI / 4.0f));
	}

	DEFINE_ALIGNED( static const float3 vConeDirs[], 16)  = {
		float3(1, 0, 0),
		float3(-1, 0, 0),
		float3(0, 1, 0),
		float3(0, -1, 0),
		float3(0, 0, 1),
		float3(0, 0, -1),
	};

	static const int inputOffset[] = {
		+1,
		-1,
		GridRes,
		-(int)GridRes,
		GridRes*GridRes,
		-(int)(GridRes*GridRes),
	};


	DEFINE_ALIGNED(static const float4 vCone90Degree[], 16) = {
		Cone90Degree(-vConeDirs[0]),
		Cone90Degree(-vConeDirs[1]),
		Cone90Degree(-vConeDirs[2]),
		Cone90Degree(-vConeDirs[3]),
		Cone90Degree(-vConeDirs[4]),
		Cone90Degree(-vConeDirs[5]),
	};

	
#ifdef INTRIN_USE
	inline float4 SHRotate(const __m128& vcDirMM, const float2 &vZHCoeffs)
	{
		DEFINE_ALIGNED(float4, 16) vcDir;
		_mm_store_ps(&vcDir.x, vcDirMM);

		return SHRotate(vcDir.xyz(), vZHCoeffs);
	}

	__declspec(noalias) __forceinline __m128 IVPropagateDirIntrin(const float4 &src, int dirIndex)
	{
		// generate function for incoming direction from adjacent cell 
		const float* pfCone = (float*)(vCone90Degree[dirIndex]);
		const __m128 shIncomingDirFunction = _mm_load_ps(pfCone);
		const __m128 zero = _mm_setzero_ps();
		const __m128 vsrc = _mm_load_ps((float*)src);

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

	__declspec(noalias) inline float getSolidAngle(const __m128& dir, const __m128& faceDir)
	{
		//	4 faces of this kind
		const float faceASolidAngle = 0.42343134f;
		//	1 face of this kind
		const float faceBSolidAngle = 0.40066966f;

		//	dot SSE4
		const __m128 faceType = _mm_dp_ps(dir, faceDir, 0xFF);

		float faceTypeF = 0.0f;
		_mm_store_ss(&faceTypeF, faceType);

		if (faceTypeF < -0.01f)  //	Front face. Light enters cube through this face
			return 0.0f;
		else if (faceTypeF > 0.01f)  //	Back face
			return faceBSolidAngle;
		else
			return faceASolidAngle;  //	Side face
	}

	__declspec(noalias) inline __m128 SHEvaluateFunction(const __m128& vcDir, const __m128& data)
	{
		// return dot(data, float4(1.0f, vcDir.y, vcDir.z, vcDir.x)*SHBasis);

		const float one = 1.0f;

		const __m128 shBasis = _mm_load_ps(SHBasis);
		const __m128 ones = _mm_load_ps1(&one);

		__m128 permVcDir = _mm_permute_ps(vcDir, _MM_SHUFFLE(0, 2, 1, 3));	// [0 vcDir.y vcDir.z vcDir.x]
		permVcDir = _mm_move_ss(permVcDir, ones);							// [1 vcDir.y vcDir.z vcDir.x]

		return _mm_dp_ps(data, _mm_mul_ps(permVcDir, shBasis), 0xFF);
	}

	// Cosine lobe
	__declspec(noalias) inline __m128 SHProjectCone(const __m128& vcDir)
	{
		const float2 vZHCoeffs = SHProjectionScale * float2(0.25f, 0.5f);

		DEFINE_ALIGNED(float4, 16) rot = SHRotate(vcDir, vZHCoeffs);

		return _mm_load_ps(&rot.x);
	}

	__declspec(noalias) inline __m128 SSENormalize(const __m128& vec)
	{
		__m128 dp = _mm_dp_ps(vec, vec, 0x7F);
		dp = _mm_rsqrt_ps(dp);
		return _mm_mul_ps(vec, dp);
	}

	__declspec(noalias) inline __m128 IVPropagateVirtualDirIntrin(const float4 &srcF4, int dirIndex, int virtualDirIndex)
	{
		const float zeroPoint5F = 0.5f;
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

		const float propagationFactorF = solidAngle * 0.5f;//(4*PI);
		const __m128 propagationFactor = _mm_load_ps1(&propagationFactorF);
		//const float reprojectionFactor = 1.0f;// /PI; 	
		//const float reprojLuminance = propagationFactor * reprojectionFactor * max(SHEvaluateFunction(propDir, src), 0.0f);

		const __m128 reprojLuminance = _mm_mul_ps(propagationFactor, _mm_max_ps(SHEvaluateFunction(propDir, src), _mm_setzero_ps()));

		const __m128 shIncomingDirFunction = SHProjectCone(virtDir);

		return _mm_mul_ps(shIncomingDirFunction, reprojLuminance);
	}

	__declspec(noalias) inline __m128 IVPropagateDirAdvancedIntrin(const float4 &src, int dirIndex)
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

#endif  // USE_VIRTUAL_DIRECTIONS

	template<bool bFirstStep, bool isIMin, bool isIMax, bool isJMin, bool isJMax, bool isKMin, bool isKMax>
	__declspec(noalias) __forceinline void propagateCell(vec4* __restrict src, vec4* __restrict targetStep, vec4* __restrict targetAccum, const int i, const int j, const int k, const int readOffset)
	{
		__m128 res = _mm_setzero_ps();

#if defined(USE_VIRTUAL_DIRECTIONS)
		//if (k<GridRes-1)
		if (!isKMax)
			res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[0]], 0));
		//	float3(-1, 0, 0),
		//if (k>0)
		if (!isKMin)
			res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[1]], 1));
		//float3( 0, 1, 0), 
		//if (j<GridRes-1)
		if (!isJMax)
			res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[2]], 2));
		//float3( 0, -1, 0),
		//if (j>0)
		if (!isJMin)
			res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[3]], 3));
		//float3( 0, 0, 1), 
		//if (i<GridRes-1)
		if (!isIMax)
			res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[4]], 4));
		//float3( 0, 0, -1),			
		//if (i>0)
		if (!isIMin)
			res = _mm_add_ps(res, IVPropagateDirAdvancedIntrin(src[readOffset + inputOffset[5]], 5));
#endif
#if !defined(USE_VIRTUAL_DIRECTIONS)
		//if (k<GridRes-1)
		if (!isKMax)
			res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[0]], 0));
		//	float3(-1, 0, 0),
		//if (k>0)
		if (!isKMin)
			res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[1]], 1));
		//float3( 0, 1, 0), 
		//if (j<GridRes-1)
		if (!isJMax)
			res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[2]], 2));
		//float3( 0, -1, 0),
		//if (j>0)
		if (!isJMin)
			res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[3]], 3));
		//float3( 0, 0, 1), 
		//if (i<GridRes-1)
		if (!isIMax)
			res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[4]], 4));
		//float3( 0, 0, -1),			
		//if (i>0)
		if (!isIMin)
			res = _mm_add_ps(res, IVPropagateDirIntrin(src[readOffset + inputOffset[5]], 5));
#endif

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

	__forceinline float4 IVPropagateDir(const float4 &src, int dirIndex)
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
		return dot(data, float4(1.0f, vcDir.y, vcDir.z, vcDir.x)*SHBasis);
	}

	// Cosine lobe
	__forceinline float4 SHProjectCone(const float3& vcDir)
	{
		const float2 vZHCoeffs = SHProjectionScale * float2(0.25f, 0.5f);
		return SHRotate(vcDir, vZHCoeffs);
	}

	__forceinline float4 IVPropagateVirtualDir(const float4 &src, int dirIndex, int virtualDirIndex)
	{
		const float3& nOffset = vConeDirs[dirIndex];
		const float3& virtDir = vConeDirs[virtualDirIndex];

		float solidAngle = getSolidAngle(-nOffset, virtDir);

		if (solidAngle <= 0)
			return float4(0.0f, 0.0f, 0.0f, 0.0f);

		const float3 propDir = normalize(-nOffset + 0.5f*virtDir);

		const float propagationFactor = solidAngle * 0.5f;//(4*PI);
		const float reprojectionFactor = 1.0f;// /PI;
		const float reprojLuminance = propagationFactor * reprojectionFactor * max(SHEvaluateFunction(propDir, src), 0.0f);

		float4 shIncomingDirFunction = SHProjectCone(virtDir);

		return shIncomingDirFunction * reprojLuminance;
	}

	template<bool isIMin, bool isIMax, bool isJMin, bool isJMax, bool isKMin, bool isKMax>
	__forceinline float4 IVPropagateDirAdvanced(const float4 &src, int dirIndex)
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

	template<bool bFirstStep, bool isIMin, bool isIMax, bool isJMin, bool isJMax, bool isKMin, bool isKMax>
	__declspec(noalias) __forceinline void propagateCell(vec4* __restrict src, vec4* __restrict targetStep, vec4* __restrict targetAccum, const int i, const int j, const int k, const int readOffset)
	{
		float4 res = float4(0.0f, 0.0f, 0.0f, 0.0f);

#if defined(USE_VIRTUAL_DIRECTIONS)
		// VIRTUAL DIRECTIONS
		if (!isKMax)
			res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[0]], 0);
		//	float3(-1, 0, 0),
		if (!isKMin)
			res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[1]], 1);
		//float3( 0, 1, 0), 
		if (!isJMax)
			res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[2]], 2);
		//float3( 0, -1, 0),
		if (!isJMin)
			res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[3]], 3);
		//float3( 0, 0, 1), 
		if (!isIMax)
			res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[4]], 4);
		//float3( 0, 0, -1),			
		if (!isIMin)
			res += IVPropagateDirAdvanced<isIMin, isIMax, isJMin, isJMax, isKMin, isKMax>(src[readOffset + inputOffset[5]], 5);
#endif
#if !defined(USE_VIRTUAL_DIRECTIONS)

		//if (k<GridRes-1)
		if (!isKMax)
			res += IVPropagateDir(src[readOffset + inputOffset[0]], 0);
		//	float3(-1, 0, 0),
		//if (k>0)
		if (!isKMin)
			res += IVPropagateDir(src[readOffset + inputOffset[1]], 1);
		//float3( 0, 1, 0), 
		//if (j<GridRes-1)
		if (!isJMax)
			res += IVPropagateDir(src[readOffset + inputOffset[2]], 2);
		//float3( 0, -1, 0),
		//if (j>0)
		if (!isJMin)
			res += IVPropagateDir(src[readOffset + inputOffset[3]], 3);
		//float3( 0, 0, 1), 
		//if (i<GridRes-1)
		if (!isIMax)
			res += IVPropagateDir(src[readOffset + inputOffset[4]], 4);
		//float3( 0, 0, -1),			
		//if (i>0)
		if (!isIMin)
			res += IVPropagateDir(src[readOffset + inputOffset[5]], 5);
#endif

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
#if !defined (ORBIS_TASK_MANAGER)
			pTaskManager->releaseTask(m_hLastTask);
#endif
			m_hLastTask = ITASKSETHANDLE_INVALID;
		}
	}
	/************************************************************************/
	// Templates
	/************************************************************************/
	template<bool bFirstStep, bool isIMin, bool isIMax, bool isJMin, bool isJMax>
	__declspec(noalias) __forceinline void propagateRow(vec4* __restrict src, vec4* __restrict targetStep, vec4* __restrict targetAccum, const int i, const int j, int &readOffset)
	{
		//	Igor: partially unroll the loop. This unroll ifs too.
		propagateCell<bFirstStep, isIMin, isIMax, isJMin, isJMax, true, false>(src, targetStep, targetAccum, i, j, 0, readOffset);
		++readOffset;

		for (int k = 1; k < static_cast<int>(GridRes - 1); ++k, ++readOffset)
		{
			propagateCell<bFirstStep, isIMin, isIMax, isJMin, isJMax, false, false>(src, targetStep, targetAccum, i, j, k, readOffset);
		}

		propagateCell<bFirstStep, isIMin, isIMax, isJMin, isJMax, false, true>(src, targetStep, targetAccum, i, j, GridRes - 1, readOffset);
		++readOffset;
	}

	template<bool bFirstStep, bool isIMin, bool isIMax>
	__declspec(noalias) __forceinline void propagateSlice(vec4* __restrict src, vec4* __restrict targetStep, vec4* __restrict targetAccum, const int i, int &readOffset)
	{
		propagateRow<bFirstStep, isIMin, isIMax, true, false>(src, targetStep, targetAccum, i, 0, readOffset);

		for (int j = 1; j < static_cast<int>(GridRes - 1); ++j)
		{
			propagateRow<bFirstStep, isIMin, isIMax, false, false>(src, targetStep, targetAccum, i, j, readOffset);
		}

		propagateRow<bFirstStep, isIMin, isIMax, false, true>(src, targetStep, targetAccum, i, GridRes - 1, readOffset);
	}

	template<bool bFirstStep>
	__declspec(noalias) void LightPropagationCPUContext::propagateStep(vec4* __restrict src, vec4* __restrict targetStep, vec4* __restrict targetAccum, int iMinSlice/*=0*/, int iMaxSlice/*=GridRes*/)
	{
		int readOffset = iMinSlice * (GridRes*GridRes);

		bool bLastSlice = false;

		if (iMaxSlice == GridRes)
		{
			bLastSlice = true;
			--iMaxSlice;
		}

		if (iMinSlice == 0)
		{
			++iMinSlice;
			propagateSlice<bFirstStep, true, false>(src, targetStep, targetAccum, 0, readOffset);
		}

		for (int i = iMinSlice; i < iMaxSlice; ++i)
		{
			propagateSlice<bFirstStep, false, false>(src, targetStep, targetAccum, i, readOffset);
		}

		if (bLastSlice)
		{
			propagateSlice<bFirstStep, false, true>(src, targetStep, targetAccum, GridRes - 1, readOffset);
		}
	}


	/************************************************************************/
	// Task callbacks
	/************************************************************************/
	void LightPropagationCPUContext::TaskDoPropagate(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount)
	{
		((LightPropagationCPUContext*)pvInfo)->doPropagate();
	}

	void LightPropagationCPUContext::TaskStep1(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount)
	{
		int iMinSlice = uTaskId * GridRes / uTaskCount;
		int iMaxSlice = (uTaskId + 1)*GridRes / uTaskCount;

		StepContext	*pContext = (StepContext*)pvInfo;
		pContext->pContext->propagateStep<true>(pContext->src, pContext->targetStep, pContext->targetAccum, iMinSlice, iMaxSlice);
	}

	void LightPropagationCPUContext::TaskStepN(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount)
	{
		int iMinSlice = uTaskId * GridRes / uTaskCount;
		int iMaxSlice = (uTaskId + 1)*GridRes / uTaskCount;

		StepContext	*pContext = (StepContext*)pvInfo;
		pContext->pContext->propagateStep<false>(pContext->src, pContext->targetStep, pContext->targetAccum, iMinSlice, iMaxSlice);
	}
}
