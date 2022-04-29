/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#ifndef __LOCATION_H_090AE35F_46AB_4F9D_B162_71B03AAF5060_INCLUDED__
#define __LOCATION_H_090AE35F_46AB_4F9D_B162_71B03AAF5060_INCLUDED__

namespace confetti
{
class Location
{
public:
	Location(double radLatitude=0,double radLongiture=0)
		:m_radLatitude(radLatitude), m_radLongitude(radLongiture){}

	//	Igor: all calculations are in radians
	double	getLatitude() const { return m_radLatitude;}
	double	getLongitude() const { return m_radLongitude; }
	
	void	setLatitude( double radLatitude) { m_radLatitude = radLatitude; }
	void	setLongitude( double radLongiture) { m_radLongitude = radLongiture; }
	

private:
	double	m_radLatitude;		//	-PI/2..PI/2
	double	m_radLongitude;	//	0..2*PI
};
}

#endif //__LOCATION_H_090AE35F_46AB_4F9D_B162_71B03AAF5060_INCLUDED__