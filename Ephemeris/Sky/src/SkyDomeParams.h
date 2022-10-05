/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#pragma once

#include "../../../../The-Forge/Common_3/Utilities/ThirdParty/OpenSource/EASTL/vector.h"
#include "../../../../The-Forge/Common_3/Graphics/Interfaces/IGraphics.h"
#include "../../../../The-Forge/Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"

/*
#include "../../../../The-Forge/Common_3/Renderer/ResourceLoader.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/ILog.h"
#include "../../../../The-Forge/Common_3/Utilities/Interfaces/IMemory.h"
*/

#if !defined(XBOX)
#define	USE_CLOUDS_DEPTH_RECONSTRUCTION
#endif

#define	STABLISE_PARTICLE_ROTATION

struct SkyDomeParams
{
	vec3		vLocationOrigin;	//	coordinates of Location point in client units
	float		fSunPosition;
	float		fExposure;
	float		fInscatterIntencity;
	float		fInscatterContrast;
	float		fSunSpeed;
	float		fUnitsToM;	//	client units to m conversion multiplier (m/units ratio)
	bool		bMoveSun;
};

struct CloudsParams
{
	float		fStepSize;
	float		fAttenuation;
	float		fAlphaSaturation;
	float		fCumulusAlphaSaturation;
	float		fCloudSpeed;
	float		fCumulusExistanceR;
	float		fCumulusFadeStart;
	bool		bUpdateEveryFrame;
	bool		bDepthTestClouds;
	bool		bMoveClouds;

	float		fCumulusSunIntensity;
	float		fCumulusAmbientIntensity;
	float		fStratusSunIntensity;
	float		fStratusAmbientIntensity;

	float		fCloudsSaturation;
	float		fCloudsContrast;
};

struct SunShaftParams
{
	float		fShaftsLength;
	float		fShaftsAngularFactor;
};

struct SpaceObjectsParams
{
	float		fSunSizeDeg;	//	Sun impostor size in degrees
	float		fMoonSizeDeg;	//	Moon impostor size in degrees

	float		fSunIntensity;	//	Default value is 100. This is physically-balanced against sky intensity
	float		fMoonIntensity;	//	Default value is 1
	float		fStarsIntensity;//	Default value is 100. This is physically-balanced against sky intensity

	float fNebulaIntensity; // Default value is 2
};

struct Params
{
	SkyDomeParams		SDParams;
	CloudsParams		CParams;
	SunShaftParams		SSHParams;
	SpaceObjectsParams	SOParams;
};
