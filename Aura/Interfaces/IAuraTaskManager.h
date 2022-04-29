/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Aura.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#ifndef __AURATASKMANAGER_H_9E2034BB_D65C_4EAE_9621_057ACCF12866_INCLUDED__
#define __AURATASKMANAGER_H_9E2034BB_D65C_4EAE_9621_057ACCF12866_INCLUDED__

#include <stdint.h>
#include "../Config/AuraConfig.h"

#ifndef ITASKSETHANDLE_INVALID
//  Value of a ITASKSETHANDLE that indicates an invalid handle
#define ITASKSETHANDLE_INVALID 0xFFFFFFFF
#endif

namespace aura
{
	typedef uint32_t ITASKSETHANDLE;

	//							 (args, context, id, count)
	typedef void (*ITASKSETFUNC)(void*, int32_t, uint32_t, uint32_t);

	class ITaskManager
	{
	public:
		virtual ~ITaskManager() {}
		virtual bool createTaskSet(uint32_t group, ITASKSETFUNC pFunc, void *pArg, uint32_t uTaskCount, ITASKSETHANDLE *pDepends, uint32_t nDepends, const char *setName, ITASKSETHANDLE *pOutHandle) = 0;
		virtual void releaseTask(ITASKSETHANDLE hTaskSet) = 0;
		virtual void releaseTasks(ITASKSETHANDLE *pHTaskSets, uint32_t nSets) = 0;
		virtual void waitForTaskSet(ITASKSETHANDLE hTaskSet) = 0;
		virtual bool isTaskDone(ITASKSETHANDLE hTaskSet) = 0;
		virtual void waitAll() = 0;
	};

	void	initTaskManager(ITaskManager** ppTaskManager);
	void	removeTaskManager(ITaskManager* pTaskManager);
}

#endif //__AURATASKMANAGER_H_9E2034BB_D65C_4EAE_9621_057ACCF12866_INCLUDED__
