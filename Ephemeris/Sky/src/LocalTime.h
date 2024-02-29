/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#ifndef __LOCALTIME_H_471C6858_07D7_43D6_89E6_F4BFC1B8749E_INCLUDED__
#define __LOCALTIME_H_471C6858_07D7_43D6_89E6_F4BFC1B8749E_INCLUDED__

#include "../../../../The-Forge/Common_3/Application/Config.h"

namespace confetti
{
class LocalTime
{
public:
    LocalTime(bool bNow = false);

    //	TODO: B: do wee need validation???
    void setLocalYear(int localYear) { m_localYear = localYear; }
    void setLocalMonth(int localMonth) { m_localMonth = localMonth; }
    void setLocalDay(int localDay) { m_localDay = localDay; }

    void setLocalHours(int localHours) { m_localHours = localHours; }
    void setLocalMinutes(int localMinutes) { m_localMinutes = localMinutes; }
    void setLocalSeconds(double localSeconds) { m_localSeconds = localSeconds; }

    void setGMTOffset(int GMTOffset) { m_GMTOffset = GMTOffset; }

    void setDayLightSavingEnabled(bool bDSTEnabled) { m_isDST = bDSTEnabled; }

    double getJ200Centuries(bool bAtomic) const;

private:
    double m_localSeconds;
    double m_GMTOffset;
    int    m_localYear;
    int    m_localMonth;
    int    m_localDay;
    int    m_localHours;
    int    m_localMinutes;

    //	Rename this to something more system-like
    bool m_isDST;
};

} // namespace confetti

#endif //__LOCALTIME_H_471C6858_07D7_43D6_89E6_F4BFC1B8749E_INCLUDED__
