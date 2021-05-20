/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "Ephemeris.h"

namespace confetti
{
	   
//	TODO: C: vheck all signs here

	mat4 rotateX(const float angle) {
		float cosA = cosf(angle), sinA = sinf(angle);

		return mat4(
			vec4(1.0f, 0.0f, 0.0f, 0.0f),
			vec4(0.0f, cosA, sinA, 0.0f),
			vec4(0.0f, -sinA, cosA, 0.0f),
			vec4(0.0f, 0.0f, 0.0f, 1.0f));
	}

	mat4 rotateY(const float angle) {
		float cosA = cosf(angle), sinA = sinf(angle);

		return mat4(
			vec4(cosA, 0.0f, sinA, 0.0f),
			vec4(0.0f, 1.0f, 0.0f, 0.0f),
			vec4(-sinA, 0.0f, cosA, 0.0f),
			vec4(0.0f, 0.0f, 0.0f, 1.0f));
	}

	mat4 rotateZ(const float angle) {
		float cosA = cosf(angle), sinA = sinf(angle);

		return mat4(
			vec4(cosA, sinA, 0.0f, 0.0f),
			vec4(-sinA, cosA, 0.0f, 0.0f),
			vec4(0.0f, 0.0f, 1.0f, 0.0f),
			vec4(0.0f, 0.0f, 0.0f, 1.0f));
	}

void Ephemeris::Update( const Location& location, const LocalTime& localTime )
{	
#if !defined(METAL) && !defined(__linux__)
	m_TimeAtomic = localTime.getJ200Centuries(true);
	m_TimeGMT = localTime.getJ200Centuries(false);

	m_GSTM = 4.894961 + 230121.675315 * m_TimeGMT;
	m_LSTM = m_GSTM + location.getLongitude();

	mat4 Rx, Ry, Rz;
	Ry = rotateY((float)(-0.00972 * m_TimeAtomic));
	Rz = rotateZ((float)(0.01118 * m_TimeAtomic));
	m_Precession = Rz * Ry * Rz;

	double e = 0.409093 - 0.000227 * m_TimeAtomic;

	Ry = rotateY(((float)location.getLatitude() - PI / 2.0f));
	//	TODO: B: check: Igor: changed this to make sun rotate correct side
	//Rz = rotateZ(LSTM);
	Rz = rotateZ((float)-m_LSTM);
	Rx = rotateX((float)e);
	m_EquatorialToHorizon = Ry * Rz * m_Precession;
	m_EclipticToHorizon = Ry * Rz * m_Precession * Rx;

	toEngine(m_EquatorialToHorizon);
	toEngine(m_EclipticToHorizon);

	//m_HorizonToEquatorial = !m_EquatorialToHorizon;

	m_HorizonToEquatorial = inverse(m_EquatorialToHorizon);

	UpdateSunPosition();
	UpdateMoonPosition();
#endif
}

void Ephemeris::UpdateSunPosition()
{
	double M = 6.24 + 628.302 * m_TimeAtomic;

	m_sunEclipticLongitude = (float)(4.895048 + 
		628.331951 * m_TimeAtomic + 
		(0.033417 - 0.000084 * m_TimeAtomic) * sin(M)
		+ 0.000351 * sin(2.0 * M));

	double latitude = 0;
	double geocentricDistance = 1.000140 - (0.016708 - 0.000042 * m_TimeAtomic) * cos(M) -
		0.000141 * cos(2.0 * M); // AU's

	
	float3 vEcliptic = toCartesian(geocentricDistance, latitude, m_sunEclipticLongitude);

	mat3 EclipticToHorizon;
	EclipticToHorizon[0] = vec3(m_EclipticToHorizon[0].getX(), m_EclipticToHorizon[0].getY(), m_EclipticToHorizon[0].getZ());
	EclipticToHorizon[1] = vec3(m_EclipticToHorizon[1].getX(), m_EclipticToHorizon[1].getY(), m_EclipticToHorizon[1].getZ());
	EclipticToHorizon[2] = vec3(m_EclipticToHorizon[2].getX(), m_EclipticToHorizon[2].getY(), m_EclipticToHorizon[2].getZ());

	m_sunHorizon = toEngine( v3ToF3(EclipticToHorizon * f3Tov3(vEcliptic)));
}

float3 Ephemeris::toCartesian( double r, double latitude, double longitude ) const
{
	//	TODO: Igor: check the whole maths here.
	latitude = -latitude;

	float3 v;
	v.x = (float)(r * cos(longitude) * cos(latitude)); // north
	v.y = (float)(r * sin(longitude) * cos(latitude)); // east
	v.z = (float)(r * sin(latitude)); // up

	return v;
}

float3 Ephemeris::toEngine( const float3& v ) const
{
	return v;

	// Oriented -x=n z=up y=east
	//float3 tmp;
	//tmp.x = v.x; // x is east
	//tmp.y = v.y; // y is up
	//tmp.z = -v.z; // z is north

	// Oriented -x=n z=up y=east
// 	float3 tmp;
// 	tmp.x = v.y; // x is east
// 	tmp.y = v.z; // y is up
// 	tmp.z = v.x; // -z is north

	//return tmp;
}

void Ephemeris::toEngine(mat4 &m ) const
{
	// TODO: this can be wrong from the original codes
	/*
	float4 tmp = m.rows[0];	//	Store x
	m.rows[0] = m.rows[1];	// x is east
	m.rows[1] = m.rows[2];	// y is up
	m.rows[2] = -tmp;		// z is north
	*/

	vec4 tmp = m.getRow(0);	//	Store x
	m.setRow(0, m.getRow(1));	// x is east
	m.setRow(1, m.getRow(2));	// y is up
	m.setRow(2, -tmp);		// z is north
}

void Ephemeris::UpdateMoonPosition()
{
	double lp = 3.8104 + 8399.7091 * m_TimeAtomic;
	double m = 6.2300 + 628.3019 * m_TimeAtomic;
	double f = 1.6280 + 8433.4663 * m_TimeAtomic;
	double mp = 2.3554 + 8328.6911 * m_TimeAtomic;
	double d = 5.1985 + 7771.3772 * m_TimeAtomic;

	double longitude =
		lp
		+ 0.1098 * sin(mp)
		+ 0.0222 * sin(2*d - mp)
		+ 0.0115 * sin(2*d)
		+ 0.0037 * sin(2*mp)
		- 0.0032 * sin(m)
		- 0.0020 * sin(2*f)
		+ 0.0010 * sin(2*d - 2*mp)
		+ 0.0010 * sin(2*d - m - mp)
		+ 0.0009 * sin(2*d + mp)
		+ 0.0008 * sin(2*d - m)
		+ 0.0007 * sin(mp - m)
		- 0.0006 * sin(d)
		- 0.0005 * sin(m + mp);

	double latitude =
		+ 0.0895 * sin(f)
		+ 0.0049 * sin(mp + f)
		+ 0.0048 * sin(mp - f)
		+ 0.0030 * sin(2*d  - f)
		+ 0.0010 * sin(2*d + f - mp)
		+ 0.0008 * sin(2*d - f - mp)
		+ 0.0006 * sin(2*d + f);

	double pip =
		+ 0.016593
		+ 0.000904 * cos(mp)
		+ 0.000166 * cos(2*d - mp)
		+ 0.000137 * cos(2*d)
		+ 0.000049 * cos(2*mp)
		+ 0.000015 * cos(2*d + mp)
		+ 0.000009 * cos(2*d - m);

	double dMoon = 1.0 / pip; // earth radii

	float3 vEcliptical = toCartesian(dMoon, latitude, longitude);


	mat3 EclipticToHorizon;
	EclipticToHorizon[0] = vec3(m_EclipticToHorizon[0].getX(), m_EclipticToHorizon[0].getY(), m_EclipticToHorizon[0].getZ());
	EclipticToHorizon[1] = vec3(m_EclipticToHorizon[1].getX(), m_EclipticToHorizon[1].getY(), m_EclipticToHorizon[1].getZ());
	EclipticToHorizon[2] = vec3(m_EclipticToHorizon[2].getX(), m_EclipticToHorizon[2].getY(), m_EclipticToHorizon[2].getZ());

	
	m_moonHorizon = toEngine(v3ToF3(EclipticToHorizon * f3Tov3(vEcliptical)));


	float moonPhaseAngle = (float)longitude - m_sunEclipticLongitude;
	m_SunLocalToMoon.y = 0;// sin(latitude);
	m_SunLocalToMoon.x = -cos(moonPhaseAngle);//*cos(latitude);
	m_SunLocalToMoon.z = sin(moonPhaseAngle);//*cos(latitude);

	/*
	InRange(longitude);
	InRange(sunEclipticLongitude);
	moonPhaseAngle = longitude - sunEclipticLongitude;
	InRange(moonPhaseAngle);

	moonPhase = 0.5 * (1.0 - cos(moonPhaseAngle));

	moonDistance = (moonHoriz - Vector3(0, 0, 1)).Length() * 6378.137;
	*/
}
}
