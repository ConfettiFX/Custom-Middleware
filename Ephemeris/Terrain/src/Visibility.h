/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#pragma once

#include "../../../../The-Forge/Common_3/Utilities/Math/MathTypes.h"

struct TerrainBoundingBox { float3 min, max; };

struct IndivisualPlane
{
	Plane left;
	Plane right;
	Plane bottom;
	Plane top;
	Plane front;
	Plane back;
};

struct Planes
{
	Plane planes[6];
};


struct TerrainFrustum
{
	TerrainFrustum() {}
	~TerrainFrustum() {}
	union {
		IndivisualPlane iPlanes;
		Planes CPlanes;
	};
};
