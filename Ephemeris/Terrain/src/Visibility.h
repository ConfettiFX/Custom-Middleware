/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#pragma once

#include "../../../../The-Forge/Common_3/OS/Math/MathTypes.h"

struct TerrainBoundingBox { float3 min, max; };

struct Plane {	float3 normal; float distance; };

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

static bool boxIntersects(TerrainFrustum &frustum, TerrainBoundingBox &box)
{
	Plane* planes = frustum.CPlanes.planes;
	Plane *currPlane;
	float3 *currNormal;
	float3 maxPoint;
	
	for (int planeIdx = 0; planeIdx < 6; planeIdx++)
	{
		currPlane = planes + planeIdx;
		currNormal = &currPlane->normal;
		
		maxPoint.x = (currNormal->x > 0) ? box.max.x : box.min.x;
		maxPoint.y = (currNormal->y > 0) ? box.max.y : box.min.y;
		maxPoint.z = (currNormal->z > 0) ? box.max.z : box.min.z;

		float dMax = dot(f3Tov3(maxPoint), f3Tov3(*currNormal)) + currPlane->distance;

		if (dMax < 0)
			return false;
	}
	return true;
}

static void getFrustumFromMatrix(const mat4 &matrix, TerrainFrustum &frustum)
{
	// Left clipping plane 
	frustum.iPlanes.left.normal.x = matrix[0][3] + matrix[0][0];
	frustum.iPlanes.left.normal.y = matrix[1][3] + matrix[1][0];
	frustum.iPlanes.left.normal.z = matrix[2][3] + matrix[2][0];
	frustum.iPlanes.left.distance =  matrix[3][3] + matrix[3][0];

	// Right clipping plane 
	frustum.iPlanes.right.normal.x = matrix[0][3] - matrix[0][0];
	frustum.iPlanes.right.normal.y = matrix[1][3] - matrix[1][0];
	frustum.iPlanes.right.normal.z = matrix[2][3] - matrix[2][0];
	frustum.iPlanes.right.distance =  matrix[3][3] - matrix[3][0];

	// Top clipping plane 
	frustum.iPlanes.top.normal.x = matrix[0][3] - matrix[0][1];
	frustum.iPlanes.top.normal.y = matrix[1][3] - matrix[1][1];
	frustum.iPlanes.top.normal.z = matrix[2][3] - matrix[2][1];
	frustum.iPlanes.top.distance =  matrix[3][3] - matrix[3][1];

	// Bottom clipping plane 
	frustum.iPlanes.bottom.normal.x = matrix[0][3] + matrix[0][1];
	frustum.iPlanes.bottom.normal.y = matrix[1][3] + matrix[1][1];
	frustum.iPlanes.bottom.normal.z = matrix[2][3] + matrix[2][1];
	frustum.iPlanes.bottom.distance =  matrix[3][3] + matrix[3][1];

	// Near clipping plane 
	frustum.iPlanes.front.normal.x = matrix[0][2];
	frustum.iPlanes.front.normal.y = matrix[1][2];
	frustum.iPlanes.front.normal.z = matrix[2][2];
	frustum.iPlanes.front.distance =  matrix[3][2];

	// Far clipping plane 
	frustum.iPlanes.back.normal.x = matrix[0][3] - matrix[0][2];
	frustum.iPlanes.back.normal.y = matrix[1][3] - matrix[1][2];
	frustum.iPlanes.back.normal.z = matrix[2][3] - matrix[2][2];
	frustum.iPlanes.back.distance =  matrix[3][3] - matrix[3][2];
}

