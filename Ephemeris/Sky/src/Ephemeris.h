/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#ifndef __EPHEMERIS_H_3A42F2DB_4AA8_4FC9_9170_09A8F352F103_INCLUDED__
#define __EPHEMERIS_H_3A42F2DB_4AA8_4FC9_9170_09A8F352F103_INCLUDED__

#include "../../../../The-Forge/Common_3/Utilities/Math/MathTypes.h"

#include "LocalTime.h"
#include "Location.h"

namespace confetti
{
class Location;
class LocalTime;

class Ephemeris
{
public:
	void	Update(const Location& location, const LocalTime& localTime);

	const float3&	getSunDirection() const { return m_sunHorizon;}
	const float3&	getMoonDirection() const { return m_moonHorizon;}
	const float3&	getSunLocalToMoonDirection() const { return m_SunLocalToMoon;}

	const mat4&	getEquatorialToHorizon() const {return m_EquatorialToHorizon;}
	const mat4&	getHorizonToEquatorial() const {return m_HorizonToEquatorial;}
	
private:
	void	UpdateSunPosition();
	void	UpdateMoonPosition();
	float3	toCartesian(double r, double latitude, double longitude) const;
	float3	toEngine(const float3& v) const;
	void	toEngine(mat4 &m) const;

private:
	double	m_TimeAtomic;	//	Terrestrial, in centuries
	double	m_TimeGMT;	//	GMT, in centuries

	double	m_GSTM;	//	Greenwich Standard Time Meridian, radians
	double	m_LSTM;	//	Local Standard Time Meridian, radians

	//	Temporary
	mat4	m_Precession;

	//	Conversion
	mat4	m_EclipticToHorizon;
	mat4	m_EquatorialToHorizon;

	mat4	m_HorizonToEquatorial;


	float		m_sunEclipticLongitude;
	float3		m_sunHorizon;

	float3		m_moonHorizon;
	float3		m_SunLocalToMoon;

//	Location	m_location;
//	LocalTime	m_localTime;
};
}

#endif //__EPHEMERIS_H_3A42F2DB_4AA8_4FC9_9170_09A8F352F103_INCLUDED__