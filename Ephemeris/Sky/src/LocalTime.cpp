/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#if !defined(__APPLE__) && !defined(__linux__)

#include "LocalTime.h"

#include <time.h>
#if !defined(ORBIS) && !defined(PROSPERO)
#include <sys/timeb.h>
#endif	//	SN_TARGET_PS3
#include <math.h>

namespace confetti
{

const int FirstMonth = 1;
const int FirstDay = 1;

//const int	DaysSinceYearStart[] = 
//{
//	0,
//	31,
//	31+28,
//	31+28+31,
//	31+28+31+30,
//	31+28+31+30+31,
//	31+28+31+30+31+30,
//	31+28+31+30+31+30+31,
//	31+28+31+30+31+30+31+31,
//	31+28+31+30+31+30+31+31+30,
//	31+28+31+30+31+30+31+31+30+31,
//	31+28+31+30+31+30+31+31+30+31+30,
//};

LocalTime::LocalTime( bool bNow/*=false*/ ):
	m_localSeconds(0),
	m_GMTOffset(0),
	m_localYear(2000),
	m_localMonth(1),
	m_localDay(1),
	m_localHours(0),
	m_localMinutes(0),
// 	m_localHours(6),
// 	m_localMinutes(35),

//	m_GMTOffset(8),

	m_isDST(false)
{
	if (bNow)
	{
#if defined(ORBIS) || defined(PROSPERO)
		time_t currentTime = time(NULL);
		tm			sysTime;
		sysTime = *localtime(&currentTime);

#else	//	SN_TARGET_PS3
		__timeb64	currentTime;
		tm			sysTime;
		_ftime64_s(&currentTime);
		_localtime64_s(&sysTime,&currentTime.time);
#endif	//	SN_TARGET_PS3

		m_localYear = sysTime.tm_year + 1900;
		m_localMonth = sysTime.tm_mon + FirstMonth;
		m_localDay = sysTime.tm_mday - 1 + FirstDay;  
		m_localHours = sysTime.tm_hour;
		m_localMinutes = sysTime.tm_min;
		m_localSeconds = sysTime.tm_sec;


#if defined(ORBIS) || defined(PROSPERO)
		tm			utcTime;
		utcTime = *gmtime(&currentTime);
		time_t offsTime = mktime(&utcTime);
		
		m_GMTOffset = difftime(currentTime, offsTime) / (60.0*60.0);
		m_isDST = (sysTime.tm_isdst != 0);
		//	Correct correction :) for the timezone.
		if (m_isDST)
			m_GMTOffset -= 1.0;
#else	//	SN_TARGET_PS3
		m_GMTOffset = -((double)currentTime.timezone) / 60.0;
		m_isDST = (sysTime.tm_isdst != 0);
#endif	//	SN_TARGET_PS3
	}
}

double LocalTime::getJ200Centuries( bool bAtomic ) const
{
	// Calculate GMT time, centuries
	double hours = (m_localSeconds/60.0f+m_localMinutes)/60.0f + m_localHours;

	//	TODO: Igor: we need to apply a half-day correction for some reason.
	hours += 12.f;

	//	Apply time correction
	if (m_isDST)	
		hours -= 1;

	hours -= m_GMTOffset;

	double year;
	double month;
	double dayTime;
	double res;
	dayTime = (m_localDay-FirstDay) + (hours / 24.0);

	//	This is used to handle leap years
	year = m_localYear;
	month = m_localMonth;

	if (m_localMonth < 3)
	{
		year -= 1;
		month += 12;
	}

	//	Calculate Julian day ???
	//	Since dayTime is zero-base day, add extra 1 to a standard constant
	res = (1720996.5 + 1.0)
		//	Consider leap years
		- floor(year/100.0) + floor(year/400.0) + floor(year*365.25)
		+ floor(30.6001*(month+1));

	//	Convert to J2000 Epoch before adding day and time information
	//	to preserve precision
	res -= 2451545.0;

	res += dayTime;

	if (bAtomic)
		res += 65.0 / 60.0 / 60.0 / 24.0;

	//	Convert to centuries
	res /= 36525.0;

	return res;
}
}

#endif
