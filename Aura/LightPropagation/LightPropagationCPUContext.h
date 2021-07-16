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

#include "../Math/AuraMath.h"
#include "../Math/AuraVector.h"
#include "../Interfaces/IAuraRenderer.h"
#include "../Interfaces/IAuraTaskManager.h"
#include "LightPropagationCascade.h"

namespace aura {

	class LightPropagationCPUContext
	{
	public:
		struct StepContext
		{
			LightPropagationCPUContext*	pContext;
			vec4*						src;
			vec4*						targetStep;
			vec4*						targetAccum;
		};

		enum LP_STATE
		{
			CAPTURED_LIGHT,
			PROPAGATED_LIGHT,
			APPLIED_PROPAGATION
		} eState;

	public:
		void    readData(Cmd* pCmd, Renderer* pRenderer, RenderTarget* m_LightGrids[3], uint32_t numGrids);
		void    processData(Renderer* pRenderer, ITaskManager* pTaskManager, MTTypes propagationMTType);
		void    applyData(Cmd* pCmd, Renderer* pRenderer, RenderTarget* m_LightGrids[3]);

		bool	load(Renderer* pRenderer);
		void	unload(Renderer* pRenderer, ITaskManager* pTaskManager);

		// returns false if all staging buffers aren't locked and ready to propagate
		// bool propagateLight(Cmd* pCmd, ITaskManager* pTaskManager, RenderTarget* m_LightGrids[3], MTTypes propagationMTType, int PropagationSetps);

		void applyPropagatedLight(Cmd* pCmd, ITaskManager* pTaskManager, RenderTarget* m_LightGrids[3], bool decoupled);

		const	LightPropagationCascade::State& getApplyState() const { return m_applyState; }
		void	setApplyState(const LightPropagationCascade::State &val) { m_applyState = val; }
	private:
		void	convertGPUtoCPU();
		void	convertCPUtoGPU();

		void	 launchPropagateSingleTask(ITaskManager* pTaskManager);
		void	 launchPropagateMultiTask(ITaskManager* pTaskManager, const int iTasksPerStep = 1);

		void	doPropagate();

		void	SyncToLastTask(ITaskManager* pTaskManager);

		template<bool bFirstStep>
		void	propagateStep(vec4* src, vec4* targetStep, vec4* targetAccum, int iMinSlice, int iMaxSlice);

		//	Task handlers
		static void TaskDoPropagate(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount);
		static void TaskStep1(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount);
		static void TaskStepN(void* pvInfo, int32_t iContext, uint32_t uTaskId, uint32_t uTaskCount);

	private:
		static const int				m_nMaxPropagationSteps = 64;

		Buffer*							m_UploadLightGrids[3];
		Buffer*							m_ReadbackLightGrids[3];
		vec4*							m_CPUGrids[9];
		ITASKSETHANDLE					m_hLastTask;
		int								m_nPropagationSteps;
		StepContext						m_Contexts[m_nMaxPropagationSteps][3];
		LightPropagationCascade::State	m_applyState;
	};
}