/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#pragma once

#include "../../../../The-Forge/Common_3/Utilities/ThirdParty/OpenSource/EASTL/vector.h"


class IndexManager
{
public:
	IndexManager() : m_NextFreeIndex(0) {;}

	uint32_t	GetFreeIndex();
	void	ReleaseIndex( uint32_t handle);
private:
	uint32_t			m_NextFreeIndex;
	eastl::vector<uint32_t>	m_FreeHandles;
};

inline uint32_t IndexManager::GetFreeIndex()
{
	if (m_FreeHandles.empty())
	{
		return m_NextFreeIndex++;
	}
	else
	{
		uint32_t res = m_FreeHandles.back();
		m_FreeHandles.pop_back();
		return res;
	}
}

inline void IndexManager::ReleaseIndex( uint32_t handle) 
{ 
	m_FreeHandles.push_back(handle); 
}
