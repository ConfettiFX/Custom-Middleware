/*
* Copyright (c) 2017-2024 The Forge Interactive Inc.
*
* This is a part of Aura.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#ifndef LPV_CS_LIGHT_PROPAGATE_FUNCTIONS_H
#define LPV_CS_LIGHT_PROPAGATE_FUNCTIONS_H

#include "lightPropagation.h"

// WorkGroupSize in lightPropagation.h (keep literal value for metal renderer)
#define THREAD_GROUP_X_DIM 4
#define THREAD_GROUP_Y_DIM 4
#define THREAD_GROUP_Z_DIM 4

//	TODO: Igor: this can be removed for release if 1
//	TODO: Igor: Those multiplications can be moved elsewhere to reduce the number of multiplications, e.g.
//	constants can be premultipled :)
PUSH_CONSTANT(PropagationSetupRootConstant, b0)
{
	DATA(float, fPropagationScale, None);
};

#include "lpvSHMaths.h"

GroupShared(SHSpectralCoeffs, SHGrid[(THREAD_GROUP_X_DIM + 2)][(THREAD_GROUP_Y_DIM + 2)][(THREAD_GROUP_Z_DIM + 2)]);

void FillLocalGrid(const uint3 groupThreadId, const uint3 dispatchThreadId, SamplerState pointBorder) 
{
	// all threads grab a (not its own) voxel from global to local memory 
	uint3 grabGlobalIndex = dispatchThreadId.xyz - u3(1);
	Get(SHGrid)[groupThreadId.x][groupThreadId.y][groupThreadId.z] = SHSampleGrid(pointBorder, float3(grabGlobalIndex) / GridRes, i3(0));

	// sure i'm being brain dead and missing the easy way to fill the border of this volume without lots of if's...
	if (groupThreadId.x == 0 || groupThreadId.x == 1) 
	{
		uint3 tmp = grabGlobalIndex + uint3(THREAD_GROUP_X_DIM, 0, 0);
		Get(SHGrid)[groupThreadId.x + THREAD_GROUP_X_DIM][groupThreadId.y][groupThreadId.z] = SHSampleGrid(pointBorder, float3(tmp) / GridRes, i3(0));
	}
	if (groupThreadId.y == 0 || groupThreadId.y == 1) 
	{
		uint3 tmp = grabGlobalIndex + uint3(0, THREAD_GROUP_Y_DIM, 0);
		Get(SHGrid)[groupThreadId.x][groupThreadId.y + THREAD_GROUP_Y_DIM][groupThreadId.z] = SHSampleGrid(pointBorder, float3(tmp) / GridRes, i3(0));
	}
	if (groupThreadId.z == 0 || groupThreadId.z == 1) 
	{
		uint3 tmp = grabGlobalIndex + uint3(0, 0, THREAD_GROUP_Z_DIM);
		Get(SHGrid)[groupThreadId.x][groupThreadId.y][groupThreadId.z + THREAD_GROUP_Z_DIM] = SHSampleGrid(pointBorder, float3(tmp) / GridRes, i3(0));
	}
	if ((groupThreadId.x == 0 || groupThreadId.x == 1) && (groupThreadId.y == 0 || groupThreadId.y == 1)) 
	{
		uint3 tmp = grabGlobalIndex + uint3(THREAD_GROUP_X_DIM, THREAD_GROUP_Y_DIM, 0);
		Get(SHGrid)[groupThreadId.x + THREAD_GROUP_X_DIM][groupThreadId.y + THREAD_GROUP_Y_DIM][groupThreadId.z] = SHSampleGrid(pointBorder, float3(tmp) / GridRes, i3(0));
	}
	if ((groupThreadId.x == 0 || groupThreadId.x == 1) && (groupThreadId.z == 0 || groupThreadId.z == 1)) 
	{
		uint3 tmp = grabGlobalIndex + uint3(THREAD_GROUP_X_DIM, 0, THREAD_GROUP_Z_DIM);
		Get(SHGrid)[groupThreadId.x + THREAD_GROUP_X_DIM][groupThreadId.y][groupThreadId.z + THREAD_GROUP_Z_DIM] = SHSampleGrid(pointBorder, float3(tmp) / GridRes, i3(0));
	}
	if ((groupThreadId.y == 0 || groupThreadId.y == 1) && (groupThreadId.z == 0 || groupThreadId.z == 1)) 
	{
		uint3 tmp = grabGlobalIndex + uint3(0, THREAD_GROUP_Y_DIM, THREAD_GROUP_Z_DIM);
		Get(SHGrid)[groupThreadId.x][groupThreadId.y + THREAD_GROUP_Y_DIM][groupThreadId.z + THREAD_GROUP_Z_DIM] = SHSampleGrid(pointBorder, float3(tmp) / GridRes, i3(0));
	}
	if ((groupThreadId.x == 0 || groupThreadId.x == 1) && (groupThreadId.y == 0 || groupThreadId.y == 1) && (groupThreadId.z == 0 || groupThreadId.z == 1)) 
	{
		uint3 tmp = grabGlobalIndex + uint3(THREAD_GROUP_X_DIM, THREAD_GROUP_Y_DIM, THREAD_GROUP_Z_DIM);
		Get(SHGrid)[groupThreadId.x + THREAD_GROUP_X_DIM][groupThreadId.y + THREAD_GROUP_Y_DIM][groupThreadId.z + THREAD_GROUP_Z_DIM] = SHSampleGrid(pointBorder, float3(tmp) / GridRes, i3(0));
	}

	// wait for all threads in the group to have filled local SH
	GroupMemoryBarrier();
}

///////////////////////////////////////////
//	Single virtual directions
void IVPropagateDir(inout(SHSpectralCoeffs) pixelCoeffs, const uint3 _index, const int3 nOffset)
{
	// get adjacent cell's SH coeffs 
	uint3 i = _index + uint3(nOffset);
	SHSpectralCoeffs sampleCoeffs = Get(SHGrid)[i.x][i.y][i.z];

	// generate function for incoming direction from adjacent cell 
	SHCoeffs shIncomingDirFunction = Cone90Degree(-float3(nOffset));

	// integrate incoming radiance with this function 
	float3 incidentLuminance = max(f3(0.0f), SHDot(sampleCoeffs, shIncomingDirFunction));

	// add it to the result 
	pixelCoeffs = SHAdd(pixelCoeffs, SHScale(shIncomingDirFunction, incidentLuminance));
}

float getSolidAngle(const int3 dir, const int3 faceDir)
{
	//	4 faces of this kind
	const float faceASolidAngle = 0.42343134f;
	//	1 face of this kind
	const float faceBSolidAngle = 0.40066966f;

	int faceType = int(dot(float3(dir), float3(faceDir)));

	float ret = 0.0f;
	//	Front face. Light enters cube through this face
	ret = (faceType < 0) ? 0.0f : ((faceType > 0) ? faceBSolidAngle : faceASolidAngle);

	return ret;
}

SHSpectralCoeffs IVPropagateVirtualDir(const int3 _index, const SHSpectralCoeffs sampleCoeffs, const int3 nOffset, const int3 virtDir)
{
	float propagationFactor = getSolidAngle(-nOffset, virtDir) * 0.5f;

	const float3 propDir = normalize(float3(-nOffset) + 0.5f * float3(virtDir));

	const float3 reprojLuminance = propagationFactor * max(SHEvaluateFunction(propDir, sampleCoeffs), f3(0.0f));

	SHCoeffs shIncomingDirFunction = SHProjectCone(float3(virtDir));

	return SHScale(shIncomingDirFunction, reprojLuminance);
}

void IVPropagateDirAdvanced(inout(SHSpectralCoeffs) pixelCoeffs, const uint3 _index, const int3 nOffset)
{
	const int3 virtualDir[] =
	{
		int3( 1,  0,  0),
		int3(-1,  0,  0),
		int3( 0,  1,  0),
		int3( 0, -1,  0),
		int3( 0,  0,  1),
		int3( 0,  0, -1)
	};

	// get adjacent cell's SH coeffs 
	uint3 id = _index + uint3(nOffset);
	SHSpectralCoeffs sampleCoeffs = Get(SHGrid)[id.x][id.y][id.z];

	//	Iterate over all virtual directions
	UNROLL
	for (int i = 0; i < 6; ++i)
	{
		SHSpectralCoeffs virtDirSpectral = IVPropagateVirtualDir(int3(_index), sampleCoeffs, nOffset, virtualDir[i]);
		pixelCoeffs = SHAdd(pixelCoeffs, virtDirSpectral);  // add virtual direction propagated light to the result 
	}
}

//#define USE_VIRTUAL_DIRECTIONS

//////////////////////////////////////////////////////////
//	Final gathering
SHSpectralCoeffs IVPropagate(const uint3 _index)
{
	SHSpectralCoeffs pixelCoeffs;
	pixelCoeffs.c[0] = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);
	pixelCoeffs.c[1] = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);
	pixelCoeffs.c[2] = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);
	
#ifdef USE_VIRTUAL_DIRECTIONS
	// 6-point axial gathering stencil "cross" 
	IVPropagateDirAdvanced(pixelCoeffs, _index, int3( 1,  0,  0));
	IVPropagateDirAdvanced(pixelCoeffs, _index, int3(-1,  0,  0));
	IVPropagateDirAdvanced(pixelCoeffs, _index, int3( 0,  1,  0));
	IVPropagateDirAdvanced(pixelCoeffs, _index, int3( 0, -1,  0));
	IVPropagateDirAdvanced(pixelCoeffs, _index, int3( 0,  0,  1));
	IVPropagateDirAdvanced(pixelCoeffs, _index, int3( 0,  0, -1));
#else			
	IVPropagateDir(pixelCoeffs, _index, int3( 1,  0,  0));
	IVPropagateDir(pixelCoeffs, _index, int3(-1,  0,  0));
	IVPropagateDir(pixelCoeffs, _index, int3( 0,  1,  0));
	IVPropagateDir(pixelCoeffs, _index, int3( 0, -1,  0));
	IVPropagateDir(pixelCoeffs, _index, int3( 0,  0,  1));
	IVPropagateDir(pixelCoeffs, _index, int3( 0,  0, -1));
#endif

	UNROLL
	for (uint i = 0; i < 3; ++i)
		pixelCoeffs.c[i] *= Get(fPropagationScale);

	return pixelCoeffs;
}

#endif // LPV_CS_LIGHT_PROPAGATE_FUNCTIONS_H
