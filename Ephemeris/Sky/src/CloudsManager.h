/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#pragma once

#include "SkyDomeParams.h"

struct ParticleProps
{
  ParticleProps() : texID(0), angle(0), reserved0(1), reserved1(0) {}
  float	texID;
  float	angle;
  float	reserved0;
  float	reserved1;
};

typedef int CloudHandle;
static const int CLOUD_NONE = -1;

class ICloudsManager
{
public:
  virtual ~ICloudsManager(void) { ; }

  virtual CloudHandle createDistantCloud(const mat4 & transform, Texture* texID) = 0;
  virtual CloudHandle createCumulusCloud(const mat4 & transform, Texture* texID, float particleScale, vec4 * particleOffsetScale, ParticleProps * particleProps, uint32 particleCount, bool centerParticles = true) = 0;
  virtual	void		removeCloud(CloudHandle handle) = 0;

  virtual void		setCloudTramsform(CloudHandle handle, const mat4 & transform) = 0;
};

#include "IndexManager.h"

#include "DistantCloud.h"
#include "CumulusCloud.h"
#include "CloudImpostor.h"
//#include "../Include/SkyDomeParams.h"
//#include "Containers.h"

//	Igor: don't have intrusive prt, so don't bother with pointers, owners, etc.
//	Just use arrays. Array index identifies corresponding instances of different types
struct CloudSortData
{
	enum { CT_Distant, CT_Cumulus} type;
	size_t	index;
	float distanceSQR;
};


class CloudsManager : public ICloudsManager
{
public:
	CloudsManager(void);
	~CloudsManager(void);

	//void	init(Dialog *pDialog);
	//void	exit();

	bool load(int width, int height, const char* pszShaderDefines);
	void	unload();

	void prepareImpostors(Cmd *cmd, const vec3 & camPos, const mat4 & view, const mat4 &vp, float camNear );
	void	update(float frameTime);
	void	clipClouds(const vec3 & camPos);
	void drawFrame(Cmd *cmd, const mat4 &vp, const mat4 &view, const vec3 &camPos, const vec3 &camPosLocalKM, const vec4 &offsetScale, vec3 &sunDir,
  Texture* Transmittance, Texture* Irradiance, Texture* Inscatter, Texture* shaftsMask, float exposure, vec2 inscatterParams, vec4 QNnear, const Texture* rtDepth, bool bSoftClouds);

	void	drawDebug();

	void	setParams(const CloudsParams& params);
	void	getParams(CloudsParams& params);

	//	Igor: ICloudManager implementation
	virtual CloudHandle	createDistantCloud(const mat4 & transform, Texture* texID );
	virtual CloudHandle createCumulusCloud(const mat4 & transform, Texture* texID, float particleScale, vec4 * particleOffsetScale, ParticleProps * particleProps, uint32 particleCount, bool centerParticles=true);
	virtual	void		removeCloud(CloudHandle handle);
	virtual void		setCloudTramsform(CloudHandle handle, const mat4 & transform);
private:
	void	clipCumulusClouds(const vec3 & camPos);
	void	loadDistantClouds();
	void	loadCumulusClouds();
	void	generateCumulusCloudParticles( eastl::vector<vec4> & particlePosScale, eastl::vector<uint32> & particleTeIDs);
	void	generateCumulusCloudParticles( eastl::vector<vec4> & particlePosScale, eastl::vector<ParticleProps> & particleProps);
	//void	prepareSortData();

	void renderCumulusCloud(Cmd *cmd, Texture* Transmittance, Texture* Irradiance, Texture* Inscatter, Texture* shaftsMask, float exposure,
 const vec3 & localCamPosKM, const vec4 &offsetScaleToLocalKM, vec3 & sunDir, vec2 inscatterParams, size_t i, const mat4 & vp, float cloudOpacity, const vec3& camDir, vec4 QNnear, const Texture* rtDepth, bool bSoftClouds);
	void	renderDistantCloud(Cmd* cmd,
    Texture* Transmittance, Texture* Irradiance, Texture* Inscatter, Texture* shaftsMask, float exposure,
		const vec3 & localCamPosKM, const vec4 &offsetScaleToLocalKM, vec3 & sunDir, vec2 inscatterParams, size_t i, const mat4 & vp);
	void	sortClouds( const vec3 & camPos );

	char*	prepareIncludes();

  uint gFrameIndex;

  Renderer* pRenderer;

  uint gImageCount;
  uint mWidth;
  uint mHeight;

  RenderTarget* pSkyRenderTarget;

private:

  eastl::vector<DistantCloud>	m_DistantClouds;
	IndexManager			m_DistantCloudsHandles;

  eastl::vector<CumulusCloud>	m_CumulusClouds;
  eastl::vector<CloudImpostor>	m_Impostors;
	IndexManager			m_CumulusCloudsHandles;

  eastl::vector<CloudSortData>  m_SortedClouds;

	Sampler*		linearClamp;
  Sampler*		trilinearClamp;

	Shader*	m_shDistantCloud;
#ifdef	USE_CLOUDS_DEPTH_RECONSTRUCTION
  Shader*	m_shImpostorCloud;
#endif	//	USE_CLOUDS_DEPTH_RECONSTRUCTION
  Shader*	m_shCumulusCloud;

  Pipeline* pDistantCloudPipeline;
  Pipeline* pCumulusCloudPipeline;
  Pipeline* pImposterCloudPipeline;

	Texture*	m_tDistantCloud;
  Texture*	m_tCumulusCloud;

	CloudsParams	m_Params;

  Buffer*   pCumulusUniformBuffer[3] = { NULL };
  Buffer*   pDistantUniformBuffer[3] = { NULL };
  Buffer*   pImposterUniformBuffer[3] = { NULL };
  Buffer* pRenderSkyUniformBuffer = NULL;
};