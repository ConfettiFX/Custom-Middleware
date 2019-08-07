#pragma once

#include "../../../../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/vector.h"


class IndexManager
{
public:
	IndexManager() : m_NextFreeIndex(0) {;}

	uint32	GetFreeIndex();
	void	ReleaseIndex( uint32 handle);
private:
	uint32			m_NextFreeIndex;
	eastl::vector<uint32>	m_FreeHandles;
};

inline uint32 IndexManager::GetFreeIndex()
{
	if (m_FreeHandles.empty())
	{
		return m_NextFreeIndex++;
	}
	else
	{
		uint32 res = m_FreeHandles.back();
		m_FreeHandles.pop_back();
		return res;
	}
}

inline void IndexManager::ReleaseIndex( uint32 handle) 
{ 
	m_FreeHandles.push_back(handle); 
}
