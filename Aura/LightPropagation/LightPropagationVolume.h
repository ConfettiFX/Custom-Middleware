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

#pragma once

#include "../Interfaces/IAuraRenderer.h"
#include "../Config/AuraParams.h"
#include "../Math/AuraVector.h"
#include "../Interfaces/IAuraTaskManager.h"

#ifdef XBOX
#include <DirectXPackedVector.h>
#endif

#define NO_FSL_DEFINITIONS
#include "../Shaders/FSL/lpvCommon.h"

#include "LightPropagationCascade.h"
#include "LightPropagationCPUContext.h"

//#include "SSGI/SSGIHandler.h"

//	TODO: Igor: replace this with the interface header include.
//#include "../Include/AuraParams.h"

namespace aura
{
	static const uint32_t MAX_FRAMES = 3U;

	
	/************************************************************************/
	// LPV Interface
	/************************************************************************/
	enum LPVOptions
	{
		LPV_NO_FRESNEL = 0x01,
		LPV_ALLOW_OCCLUSION = 0x02,
		LPV_UNPACK_NORMAL = 0x04,
		LPV_SCALAR_SPECULAR = 0x08,
		LPV_USE_SSGI = 0x10,
	};

	enum CascadeOptions
	{
		CASCADE_NOT_MOVING = 0x01,
	};

	struct Box
	{
		vec3	vMin;
		vec3	vMax;
	};

	typedef struct LightPropagationCascadeDesc
	{
		float		mGridSpan;
		float		mGridIntensity;
		uint32_t	mFlags;
	} LightPropagationCascadeDesc;

	typedef struct Aura
	{
		LightPropagationVolumeParams	mParams;
		CPUPropagationParams			mCPUParams;

		uint32_t						mOptions;
		//	Capture current, propagate current+1, apply current+2
#ifdef ENABLE_CPU_PROPAGATION
		int32_t							mCPUPropagationCurrentContext;
		LightPropagationCPUContext**	m_CPUContexts;
		bool							bUseCPUPropagationPreviousFrame; // Used to detect if switching between CPU and GPU propagation.  
		// The CPU propagation runs behind the GPU by this many frames so that data is always available. 
		uint32_t						mInFlightFrameCount; 
#endif
		int32_t							mGPUPropagationCurrentGrid;

		RenderTarget*					pWorkingGrids[6];
		uint32_t						mCascadeCount;
		LightPropagationCascade**		pCascades;

		uint32_t			mFrameIdx;

		Shader*				pShaderDebugDrawVolume;
		Shader*				pShaderDebugDrawOccluders;
		Shader*				pShaderLPVVisualize;
		Shader*				pShaderAreaLight;
		Shader*				pShaderhInjectOccluder;
		Shader*				pShaderInjectOccluderCube;
		Shader*				pShaderCopyOccluder;
		Shader*				pShaderInjectRSMLight;
		Shader*				pShaderInjectRSMLightCube;
#if !defined(ORBIS) // causes error : private field 'NAME' is not used
		Shader*				pShaderLightPropagate;
#endif
		Shader*				pShaderLightPropagate1[2];
		Shader*				pShaderLightPropagateN[2];
		Shader*				pShaderLightCopy;

		Pipeline*			pPipelineInjectRSMLight;
		Pipeline*			pPipelineLightPropagate1[2];
		Pipeline*			pPipelineLightPropagateN[2];
		Pipeline*			pPipelineLightCopy;
		Pipeline*			pPipelineVisualizeLPV;

		RootSignature*		pRootSignatureInjectRSMLight;
		RootSignature*		pRootSignatureLightPropagate1;
		RootSignature*		pRootSignatureLightPropagateN;
		RootSignature*		pRootSignatureLightCopy;
		RootSignature*		pRootSignatureVisualizeLPV;
		uint32_t			mPropagation1RootConstantIndex;
		uint32_t			mPropagationNRootConstantIndex;

		DescriptorSet*		pDescriptorSetInjectRSMLight;
		DescriptorSet*		pDescriptorSetLightPropagate1;
		DescriptorSet*		pDescriptorSetLightPropagateN;
		DescriptorSet*		pDescriptorSetLightCopy;
		DescriptorSet*      pDescriptorSetVisualizeLPV;

		Buffer**			pUniformBufferInjectRSM[MAX_FRAMES];
		Buffer*			    pUniformBufferVisualizationData[MAX_FRAMES];

		Sampler*			pSamplerLinearBorder;
		Sampler*			pSamplerPointBorder;
	} Aura;

	void initAura(Renderer* pRenderer, PipelineCache* pCache, uint32_t rtWidth, uint32_t rtHeight, LightPropagationVolumeParams params, uint32_t inFlightFrameCount, uint32_t options, TinyImageFormat visualizeFormat, TinyImageFormat visualizeDepthFormat, SampleCount sampleCount, uint32_t sampleQuality,
		uint32_t cascadeCount, LightPropagationCascadeDesc* pCascades, Aura** ppAura);
	void loadCPUPropagationResources(Renderer* pRenderer, Aura* pAura);
	void removeAura(Renderer* pRenderer, ITaskManager* pTaskManager, Aura* pAura);
	void getShaderMacros(Aura* pAura, ShaderMacro pMacros[4], uint32_t* pCount);

	void setCascadeCenter(Aura* pAura, uint32_t Cascade, const vec3& center);
	void getGridBounds(Aura* pAura, const mat4& worldToLocal, Box* bounds);
	uint32_t getCascadesToUpdateMask(Aura* pAura);

	void beginFrame(Renderer* pRenderer, Aura* pAura, const vec3& camPos, const vec3& camDir);
	void endFrame(Renderer* pRenderer, Aura* pAura);
	void mapAsyncResources(Renderer* pRenderer);

	void injectRSM(Cmd* pCmd, Renderer* pRenderer, Aura* pAura, uint32_t iVolume, const mat4& invVP, const vec3& camDir, uint32_t rtWidth, uint32_t rtHeight, float viewAreaForUnitDepth, Texture* baseRT, Texture* normalRT, Texture* depthRT);
	// CPU propagation
	void captureLight(Cmd* pCmd, Aura* pAura);

	void propagateLight(Cmd* pCmd, Renderer* pRenderer, ITaskManager* pTaskManager, Aura* pAura);
	void applyLight(Cmd* pCmd, Renderer* pRenderer, Aura* pAura, const mat4& invVP, const vec3& camPos, Texture* normalRT, Texture* depthRT, Texture* ambientOcclusionRT);
	void getLightApplyData(Aura* pAura, const mat4& invVP, const vec3& camPos, LightApplyData* data);

	void drawLpvVisualization(Cmd* cmd, Renderer* pRenderer, Aura* pAura, RenderTarget* renderTarget, RenderTarget* depthRenderTarget, const mat4& projection, const mat4& view, const mat4& inverseView, int cascadeIndex, float probeSize);

	/************************************************************************/
	/************************************************************************/
}
