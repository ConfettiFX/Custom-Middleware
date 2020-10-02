#pragma once

#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/vector.h"


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
