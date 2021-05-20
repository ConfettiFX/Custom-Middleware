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

#ifndef __AURAMEMORYMANAGER_H_D70DE54E_FCDE_44B9_A627_3B56D1457959_INCLUDED__
#define __AURAMEMORYMANAGER_H_D70DE54E_FCDE_44B9_A627_3B56D1457959_INCLUDED__

#if defined(_WIN32) || defined(__WIN32) || defined(__linux__) || defined(NX64)
// For conf_alloca().
#include <malloc.h>
#else
//#include <conf_alloca.h>
#endif

#include <stdlib.h>

namespace aura
{
	void* alloc(size_t size);
	void dealloc(void* ptr);
}

#endif //__AURAMEMORYMANAGER_H_D70DE54E_FCDE_44B9_A627_3B56D1457959_INCLUDED__