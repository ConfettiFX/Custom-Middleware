/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#pragma once
#include "../../../../The-Forge/Common_3/Utilities/ThirdParty/OpenSource/Nothings/stb_ds.h"

class IndexManager
{
public:
    ~IndexManager() { arrfree(m_FreeHandles); }

    uint32_t GetFreeIndex()
    {
        size_t len = arrlenu(m_FreeHandles);
        if (len == 0)
        {
            return m_NextFreeIndex++;
        }
        else
        {
            uint32_t res = m_FreeHandles[len - 1];
            arrpop(m_FreeHandles);
            return res;
        }
    }

    void ReleaseIndex(uint32_t handle) { arrpush(m_FreeHandles, handle); }

private:
    uint32_t  m_NextFreeIndex = 0;
    uint32_t* m_FreeHandles = NULL;
};
