/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Aura.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#ifndef LPV_LIGHT_PROPAGATE_FUNCTIONS_H
#define LPV_LIGHT_PROPAGATE_FUNCTIONS_H

//	TODO: Igor: this can be removed for release if 1
//	TODO: Igor: Those multiplications can be moved elsewhere to reduce the number of multiplications, e.g.
//	constants can be premultipled :)
PUSH_CONSTANT(PropagationSetupRootConstant, b0)
{
	DATA(float, fPropagationScale, None);
};

#include "lpvSHMaths.h"

///////////////////////////////////////////
//	Multiple virtual directions
float getSolidAngle(const float3 dir, const float3 faceDir)
{
	//	4 faces of this kind
	const float faceASolidAngle = 0.42343134f;
	//	1 face of this kind
	const float faceBSolidAngle = 0.40066966f;

	float faceType = dot(dir, faceDir);

	//	Front face. Light enters cube through this face
	if (faceType < -0.01f)
		return 0.0f;

	//	Back face
	if (faceType > 0.01f)
		return faceBSolidAngle;

	//	Side face
	return faceASolidAngle;
}

SHSpectralCoeffs IVPropagateVirtualDir(const float3 texCoord,
	const SHSpectralCoeffs sampleCoeffs,
	const float3 nOffset,
	const float3 virtDir)
{
	SHSpectralCoeffs res;
	res.c[0] = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);
	res.c[1] = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);
	res.c[2] = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);

	float solidAngle = getSolidAngle(-nOffset, virtDir);

	//	Compiler should optimize this out
	FLATTEN
		if (solidAngle <= 0.0f)
			return res;

	const float3 propDir = normalize(-nOffset + 0.5f * virtDir);

	const float  propagationFactor = solidAngle / 2.0f; //(4*PI);
	const float  reprojectionFactor = 1.0f; // /PI;
	const float3 reprojLuminance = propagationFactor * reprojectionFactor * max(SHEvaluateFunction(propDir, sampleCoeffs), f3(0.0f));
	SHCoeffs shIncomingDirFunction = SHProjectCone(virtDir);

	res = SHScale(shIncomingDirFunction, reprojLuminance);

	return res;
}

#ifdef VULKAN

///////////////////////////////////////////
//	Single virtual directions
#define IVPropagateDir(pixelCoeffs, texCoord, nOffset) \
{ \
	/* get adjacent cell's SH coeffs */ \
	SHSpectralCoeffs sampleCoeffs = SHSampleGrid(Get(pointBorder), texCoord, int3(nOffset)); \
\
	/* generate function for incoming direction from adjacent cell */ \
	SHCoeffs shIncomingDirFunction = Cone90Degree(-nOffset); \
\
	/* integrate incoming radiance with this function */ \
	float3 incidentLuminance = max(f3(0.0f), SHDot(sampleCoeffs, shIncomingDirFunction)); \
\
	/* add it to the result */ \
	pixelCoeffs = SHAdd(pixelCoeffs, SHScale(shIncomingDirFunction, incidentLuminance)); \
}

#define IVPropagateDirN(pixelCoeffs, texCoord, nOffset, useOcclusion, multipleReflections, selfSpectralCoeffs) \
{ \
	/* get adjacent cell's SH coeffs */ \
	SHSpectralCoeffs sampleCoeffs = SHSampleGrid(Get(pointBorder), texCoord, int3(nOffset)); \
\
	/* generate function for incoming direction from adjacent cell */ \
	SHCoeffs shIncomingDirFunction = Cone90Degree(-nOffset); \
\
	/* integrate incoming radiance with this function */ \
	float3 incidentLuminance = max(f3(0.0f), SHDot(sampleCoeffs, shIncomingDirFunction)); \
\
	FLATTEN \
	if (useOcclusion || multipleReflections) \
	{ \
		float occlusion  = 1.0f; \
		float hpCellSize = 0.5f / GridRes; \
		SHCoeffs occluderCoeffs = SHSampleOccluder(Get(pointBorder), LPVtoOccluders(texCoord) + nOffset * hpCellSize, i3(0)); \
\
		if (useOcclusion) \
		{ \
			occlusion = saturate(1.0f - SHEvaluateFunction(nOffset, occluderCoeffs)); \
			/* Igor: is used to allow surfel to block in either direction the same way. */ \
			occlusion = min(occlusion, saturate(1.0f - SHEvaluateFunction(-nOffset, occluderCoeffs))); \
		} \
\
		incidentLuminance *= occlusion; \
\
		FLATTEN \
		if (multipleReflections) \
		{ \
			SHCoeffs occluderReflectorCoeffs = SHSampleOccluder(Get(pointBorder), LPVtoOccluders(texCoord) + 1.5f * nOffset * hpCellSize, i3(0)); \
			float3 sourceLuminance = max(SHEvaluateFunction(nOffset, selfSpectralCoeffs), f3(0.0f)); \
			float  sourceOcclusion = saturate(1.0f - SHEvaluateFunction(-nOffset, occluderCoeffs)); \
			float  reflectedLuminance = max(SHEvaluateFunction(-nOffset, occluderReflectorCoeffs), 0.0f); \
			reflectedLuminance = (reflectedLuminance > 1.0f) ? 1.0f / reflectedLuminance : reflectedLuminance; \
\
			/* sourceLuminance *= sourceOcclusion * reflectedLuminance * occlusion * 5.0f; */ \
			/* sourceLuminance *= sourceOcclusion * reflectedLuminance * occlusion * 2.0f; */ \
			/* sourceLuminance *= sourceOcclusion * reflectedLuminance * occlusion * 0.6f; */ \
			sourceLuminance *= sourceOcclusion * reflectedLuminance * occlusion; \
\
			/* add it to the result */ \
			pixelCoeffs = SHAdd(pixelCoeffs, SHScale(occluderReflectorCoeffs, sourceLuminance)); \
		} \
	} \
\
	/* add it to the result */ \
	pixelCoeffs = SHAdd(pixelCoeffs, SHScale(shIncomingDirFunction, incidentLuminance)); \
}

#define IVPropagateDirAdvanced1(pixelCoeffs, texCoord, nOffset) \
{ \
	const float3 virtualDir[] = \
	{ \
		float3( 1,  0,  0),	\
		float3(-1,  0,  0),	\
		float3( 0,  1,  0),	\
		float3( 0, -1,  0),	\
		float3( 0,  0,  1),	\
		float3( 0,  0, -1)	\
	}; \
\
	/* get adjacent cell's SH coeffs */ \
	SHSpectralCoeffs sampleCoeffs = SHSampleGrid(Get(pointBorder), texCoord, int3(nOffset)); \
\
	/* Iterate over all virtual directions */ \
	UNROLL \
	for (int i = 0; i < 6; ++i) \
	{ \
		SHSpectralCoeffs virtDirSpectral = IVPropagateVirtualDir(texCoord, sampleCoeffs, nOffset, virtualDir[i]); \
		/* add virtual direction propagated light to the result */ \
		pixelCoeffs = SHAdd(pixelCoeffs, virtDirSpectral); \
	} \
}

#else

///////////////////////////////////////////
//	Single virtual directions
void IVPropagateDir(inout(SHSpectralCoeffs) pixelCoeffs, const float3 texCoord, const float3 nOffset)
{
	// get adjacent cell's SH coeffs 
	SHSpectralCoeffs sampleCoeffs = SHSampleGrid(Get(pointBorder), texCoord, int3(nOffset));

	// generate function for incoming direction from adjacent cell 
	SHCoeffs shIncomingDirFunction = Cone90Degree(-nOffset);

	// integrate incoming radiance with this function 
	float3 incidentLuminance = max(f3(0.0f), SHDot(sampleCoeffs, shIncomingDirFunction));

	// add it to the result 
	pixelCoeffs = SHAdd(pixelCoeffs, SHScale(shIncomingDirFunction, incidentLuminance));
}

void IVPropagateDirN(inout(SHSpectralCoeffs) pixelCoeffs, 
	const float3 texCoord, 
	const float3 nOffset,
	const bool useOcclusion,
	const bool multipleReflections,
	const SHSpectralCoeffs selfSpectralCoeffs)
{
	// get adjacent cell's SH coeffs 
	SHSpectralCoeffs sampleCoeffs = SHSampleGrid(Get(pointBorder), texCoord, int3(nOffset));

	// generate function for incoming direction from adjacent cell 
	SHCoeffs shIncomingDirFunction = Cone90Degree(-nOffset);

	// integrate incoming radiance with this function 
	float3 incidentLuminance = max(f3(0.0f), SHDot(sampleCoeffs, shIncomingDirFunction));

	FLATTEN
	if (useOcclusion || multipleReflections)
	{
		float occlusion  = 1.0f;
		float hpCellSize = 0.5f / GridRes;
		SHCoeffs occluderCoeffs = SHSampleOccluder(Get(pointBorder), LPVtoOccluders(texCoord) + nOffset * hpCellSize, i3(0));

		if (useOcclusion)
		{
			occlusion = saturate(1.0f - SHEvaluateFunction(nOffset, occluderCoeffs));
			//	Igor: is used to allow surfel to block in either direction the same way.
			occlusion = min(occlusion, saturate(1.0f - SHEvaluateFunction(-nOffset, occluderCoeffs)));
		}

		incidentLuminance *= occlusion;

		FLATTEN
		if (multipleReflections)
		{
			SHCoeffs occluderReflectorCoeffs = SHSampleOccluder(Get(pointBorder), LPVtoOccluders(texCoord) + 1.5f * nOffset * hpCellSize, i3(0));
			float3 sourceLuminance    = max(SHEvaluateFunction(nOffset, selfSpectralCoeffs), f3(0.0f));
			float  sourceOcclusion    = saturate(1.0f - SHEvaluateFunction(-nOffset, occluderCoeffs));
			float  reflectedLuminance = max(SHEvaluateFunction(-nOffset, occluderReflectorCoeffs), 0.0f);
			reflectedLuminance = (reflectedLuminance > 1.0f) ? 1.0f / reflectedLuminance : reflectedLuminance;

			//sourceLuminance *= sourceOcclusion * reflectedLuminance * occlusion * 5.0f;
			//sourceLuminance *= sourceOcclusion * reflectedLuminance * occlusion * 2.0f;
			//sourceLuminance *= sourceOcclusion * reflectedLuminance * occlusion * 0.6f;.
			sourceLuminance *= sourceOcclusion * reflectedLuminance * occlusion;

			// add it to the result 
			pixelCoeffs = SHAdd(pixelCoeffs, SHScale(occluderReflectorCoeffs, sourceLuminance));
		}
	}

	// add it to the result 
	pixelCoeffs = SHAdd(pixelCoeffs, SHScale(shIncomingDirFunction, incidentLuminance));
}

void IVPropagateDirAdvanced1(inout(SHSpectralCoeffs) pixelCoeffs, const float3 texCoord, const float3 nOffset)
{
	const float3 virtualDir[] =
	{
		float3( 1,  0,  0),
		float3(-1,  0,  0),
		float3( 0,  1,  0),
		float3( 0, -1,  0),
		float3( 0,  0,  1),
		float3( 0,  0, -1)
	};

	// get adjacent cell's SH coeffs 
	SHSpectralCoeffs sampleCoeffs = SHSampleGrid(Get(pointBorder), texCoord, int3(nOffset));

	// Iterate over all virtual directions
	UNROLL
	for (int i = 0; i < 6; ++i)
	{
		SHSpectralCoeffs virtDirSpectral = IVPropagateVirtualDir(texCoord, sampleCoeffs, nOffset, virtualDir[i]);
		// add virtual direction propagated light to the result 
		pixelCoeffs = SHAdd(pixelCoeffs, virtDirSpectral);
	}
}

#endif // VULKAN

//////////////////////////////////////////////////////////
//	Final gathering
SHSpectralCoeffs IVPropagate(const float3 texCoord, const bool firstIteration, bool reflections)
{
	SHSpectralCoeffs pixelCoeffs;
	pixelCoeffs.c[0] = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);
	pixelCoeffs.c[1] = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);
	pixelCoeffs.c[2] = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);

	//	There switches are use to set up the way
	//	propagation function behaves
	const bool advanced = true;

#ifdef ALLOW_OCCLUSION
	const bool useOcclusion = true && !firstIteration;
#else
	const bool useOcclusion = false;
#endif // ALLOW_OCCLUSION

	const bool multipleReflections = false;
	//const bool multipleReflections = reflections;

	if (advanced)
	{
		// 6-point axial gathering stencil "cross" 
		IVPropagateDirAdvanced1(pixelCoeffs, texCoord, float3( 1,  0,  0));
		IVPropagateDirAdvanced1(pixelCoeffs, texCoord, float3(-1,  0,  0));
		IVPropagateDirAdvanced1(pixelCoeffs, texCoord, float3( 0,  1,  0));
		IVPropagateDirAdvanced1(pixelCoeffs, texCoord, float3( 0, -1,  0));
		IVPropagateDirAdvanced1(pixelCoeffs, texCoord, float3( 0,  0,  1));
		IVPropagateDirAdvanced1(pixelCoeffs, texCoord, float3( 0,  0, -1));
	}
	else
	{
		SHSpectralCoeffs selfCoeffs = SHSampleGrid(Get(pointBorder), texCoord, i3(0));

		IVPropagateDirN(pixelCoeffs, texCoord, float3( 1,  0,  0), useOcclusion, multipleReflections, selfCoeffs);
		IVPropagateDirN(pixelCoeffs, texCoord, float3(-1,  0,  0), useOcclusion, multipleReflections, selfCoeffs);
		IVPropagateDirN(pixelCoeffs, texCoord, float3( 0,  1,  0), useOcclusion, multipleReflections, selfCoeffs);
		IVPropagateDirN(pixelCoeffs, texCoord, float3( 0, -1,  0), useOcclusion, multipleReflections, selfCoeffs);
		IVPropagateDirN(pixelCoeffs, texCoord, float3( 0,  0,  1), useOcclusion, multipleReflections, selfCoeffs);
		IVPropagateDirN(pixelCoeffs, texCoord, float3( 0,  0, -1), useOcclusion, multipleReflections, selfCoeffs);
	}

	for (int i = 0; i < 3; ++i)
		pixelCoeffs.c[i] *= Get(fPropagationScale);

	return pixelCoeffs;
}

#endif // LPV_LIGHT_PROPAGATE_FUNCTIONS_H
