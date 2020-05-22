//--------------------------------------------------------------------------------------------
//
// Coyright (C) 2009 - 2013 Confetti Special Effects Inc.
// All rights reserved.
//
// This source may not be distributed and/or modified without expressly written permission
// from Confetti Special Effects Inc.
//
//--------------------------------------------------------------------------------------------

#include "CloudImpostor.h"

#include "CumulusCloud.h"
//#include "Singletons.h"

#include <assert.h>

#define MaxParticles 100
#define QMaxParticles ((MaxParticles+3)/ 4)

struct ImposterUniformBuffer
{
  mat4 model;
  vec4	OffsetScale[MaxParticles];
  //int4	TexIDs[QMaxParticles];
  vec4	TexIDs[QMaxParticles];

  mat4 vp;
  mat4 v;
  vec3 dx, dy;
  float zNear;
};



CloudImpostor::CloudImpostor() :
	m_Radius(0),
	m_SortDepth(0),
	m_Valid(false)
{

	//m_TextureSize = 256;
	m_TextureSize = 512;
	//m_TextureSize = 1024;

	//m_tImpostor = Renderer.addRenderTarget(m_TextureSize, m_TextureSize, 1, 1, 1, ::FORMAT_RGBA8, 1, SS_NONE, 0);

#ifdef	USE_CLOUDS_DEPTH_RECONSTRUCTION
	//	TODO: Igor: check if can use simpler format
	//m_tImpostor = pRenderer->addRenderTarget(m_TextureSize, m_TextureSize, FORMAT_RGBA16F);

  RenderTargetDesc ImposterRenderTarget = {};
  ImposterRenderTarget.mArraySize = 1;
  ImposterRenderTarget.mDepth = 1;
  ImposterRenderTarget.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
  ImposterRenderTarget.mSampleCount = SAMPLE_COUNT_1;
  ImposterRenderTarget.mSampleQuality = 0;
  //ImposterRenderTarget.mSrgb = false;
  ImposterRenderTarget.mWidth = m_TextureSize;
  ImposterRenderTarget.mHeight = m_TextureSize;
  ImposterRenderTarget.pDebugName = L"Imposter RenderTarget";
  ImposterRenderTarget.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
  addRenderTarget(pRenderer, &ImposterRenderTarget, &m_tImpostor);
#else	//	USE_CLOUDS_DEPTH_RECONSTRUCTION
	//	TODO: Igor: check if can use simpler format

  RenderTargetDesc ImposterRenderTarget = {};
  ImposterRenderTarget.mArraySize = 1;
  ImposterRenderTarget.mDepth = 1;
  ImposterRenderTarget.mFormat = ImageFormat::RGBA8;
  ImposterRenderTarget.mSampleCount = SAMPLE_COUNT_1;
  ImposterRenderTarget.mSampleQuality = 0;
  ImposterRenderTarget.mSrgb = false;
  ImposterRenderTarget.mWidth = m_TextureSize;
  ImposterRenderTarget.mHeight = m_TextureSize;
  ImposterRenderTarget.pDebugName = L"Imposter RenderTarget";
  ImposterRenderTarget.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
  addRenderTarget(pRenderer, &ImposterRenderTarget, &m_tImpostor);

	//m_tImpostor = pRenderer->addRenderTarget(m_TextureSize, m_TextureSize, FORMAT_RGBA8);
#endif	//	USE_CLOUDS_DEPTH_RECONSTRUCTION

/*
  BufferLoadDesc imposterUniformDesc = {};
  imposterUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  imposterUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
  imposterUniformDesc.mDesc.mSize = sizeof(ImposterUniformBuffer);
  imposterUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
  imposterUniformDesc.pData = NULL;

  for (uint i = 0; i < gImageCount; i++)
  {
    imposterUniformDesc.ppBuffer = &pImposterUniformBuffer[i];
    addResource(&imposterUniformDesc);
  }
*/
	//assert( m_tImpostor != TEXTURE_NONE);
}

CloudImpostor::~CloudImpostor(void)
{
/*
  for (uint i = 0; i < gImageCount; i++)
  {
    removeResource(pImposterUniformBuffer[i]);
  }
*/
  removeRenderTarget(pRenderer, m_tImpostor);
}

#ifdef	CLAMP_IMPOSTOR_PROJ

float4	GetProjClampingRect(const float4x4 & proj, const float4x4 & view, float3 cameraCornerDirs[4],
							const float3 & n, float d)
{
	float minX = -1.0f;
	float maxX = 1.0f;
	float minY = -1.0f;
	float maxY = 1.0f;

	//return float4(minX, maxX, minY, maxY);

	//	Igor: multiplication order
	//	impostorProj*impostorView

	//	Vector-plane intersection, plane equation: dot(n,X) = d
	//	X = dir*lambda is the vector we want to find
	//	lambda = d / dot(n, dir)

	//	Find camera corners intersections in the world space
	float3 vIntersections[4];
	for (int i=0; i<4; ++i)
	{
		float denom = dot(n, cameraCornerDirs[i]);
		const float denomThreshold = 0.001f;

		if ( denom < denomThreshold )	
			return float4(minX, maxX, minY, maxY);

		vIntersections[i] = cameraCornerDirs[i] * d / denom;
	}

	const float4x4 VP = proj * view;

	//	Project intersections to the homogenous space
	float2 vIntersectProj[4];
	for (int i=0; i<4; ++i)
	{
		float4 tmp = VP*float4(vIntersections[i], 1.0f);
		vIntersectProj[i] = float2( tmp.x/tmp.w, tmp.y/tmp.w );
	}

	//	Find homogenous AABB for intersections
	float2 vIntersectProjMin = vIntersectProj[0];
	float2 vIntersectProjMax = vIntersectProj[0];
	for (int i=1; i<4; ++i)
	{
		vIntersectProjMin = min(vIntersectProj[i], vIntersectProjMin);
		vIntersectProjMax = max(vIntersectProj[i], vIntersectProjMax);
	}

	//	Igor: do the actual clamping
	minX = max(minX, vIntersectProjMin.x);
	minY = max(minY, vIntersectProjMin.y);
	maxX = min(maxX, vIntersectProjMax.x);
	maxY = min(maxY, vIntersectProjMax.y);

	//	Igor: need to invert y here???
	//	Do this just before return
	{
		float tmp = -minY;
		minY = -maxY;
		maxY = tmp;
	}

	return float4(minX, maxX, minY, maxY);
}

void	ClampImpostorProj(const float4 & clampWindow, float4x4 & proj)
{
	float minX = clampWindow.x;
	float maxX = clampWindow.y;
	float minY = clampWindow.z;
	float maxY = clampWindow.w;

	{
		float4x4 projPatch;
		//	Igor: zero the matrix.
		//	TODO: Igor: check if this is zero for all platforms. IEE standard?
		memset(&projPatch, 0, sizeof(projPatch));

		projPatch.rows[0][0]=1.0f;
		projPatch.rows[1][1]=1.0f;
		projPatch.rows[2][2]=1.0f;
		projPatch.rows[3][3]=1.0f;

		projPatch.rows[0][3] = -(maxX+minX)*0.5f;
		projPatch.rows[1][3] = (maxY+minY)*0.5f;

		projPatch.rows[0] /= (maxX-minX)*0.5f;
		projPatch.rows[1] /= (maxY-minY)*0.5f;

		proj = projPatch * proj;
	}
}
#endif	//	CLAMP_IMPOSTOR_PROJ	

void CloudImpostor::setupRenderer( Cmd *cmd, CumulusCloud *pCloud, const vec3 &camPos, float camNear, const mat4 &view,
 mat4 &v, mat4 &vp, vec3 &dx, vec3 &dy,
 ImpostorUpdateMode updateMode, bool &needUpdate
#ifdef	CLAMP_IMPOSTOR_PROJ
								  , vec3 cameraCornerDirs[4]
#endif	//	CLAMP_IMPOSTOR_PROJ		
#ifdef	USE_CLOUDS_DEPTH_RECONSTRUCTION
      ,  vec2 &packDepthParams
#endif
#ifdef STABLISE_PARTICLE_ROTATION
      , float &masterParticleRotation
#endif
)
{
	needUpdate = true;

	bool bWantProjClamping = false;

	/////////////////////////////////////////
	//	Calc projection
	const vec3 camUp = view.getRow(1).getXYZ();
	const mat4 &cT = pCloud->Transform();
  vec3 cloudPos = vec3(cT.getRow(0).getW(), cT.getRow(1).getW(), cT.getRow(2).getW());
  vec3 cloudDir = cloudPos-camPos;
	float distance = length(cloudDir);

	m_SortDepth = distance;

	//	Use main camera to render near cloud's impostor
	//	Lerp impostor camera and user camera
	//	Can be safely removed if alternative approach is used for near clouds rendering
	{
		const float lerpDist = 1000;
		const vec3 camDir = view.getRow(2).getXYZ();
		if ( distance<m_Radius+lerpDist )
		{
			float lerpParam = clamp((distance-m_Radius)/lerpDist, 0, 1);
			cloudPos = lerp(camPos + camDir*m_Radius, cloudPos, lerpParam);

			//	Update data
			cloudDir = cloudPos-camPos;
			distance = length(cloudDir);

			bWantProjClamping = true;
		}
		else 
		{
			//////////////////////////////////////////////////////////////////////
			//	Check here if need to update impostor (angular and distance check)
			if (m_Valid && (m_OldDistance>m_Radius+lerpDist))
			{
				//	Since old and current distances are more than R, normalize/rcp operations should be safe
				//	Looks bad for large particles: when our rotate camera and then move it, cloud
				//	changes significantly
				//	probably changing only a fraction number of clouds per frame could be better
				while(updateMode==IUM_PositionBased)
				{
					const float distanceRatioEPS = 0.1f;
					//	This should be pixel-angle size dependent
					//	Currently it's for 0.06 degree
					const float viewAngleCosEps = 0.00000045f;
					if (abs(m_OldDistance-distance)/m_OldDistance>distanceRatioEPS) break;

					vec3 nearDir = normalize(m_OldNearPos - camPos);
          vec3 farDir = normalize(m_OldFarPos - camPos);

					if (abs(dot(nearDir, farDir)-1)>viewAngleCosEps) break;

					//	TODO: C: implement angular update heuristics for the moving cloud!
					//	Probably, don't store old near/far pos, recalculate them each time for the nex cloud position/orientation

					//	Don't update the impostor
					needUpdate = false;
					return;
				}
			}
		}
	}

	//Renderer.changeRenderTarget(m_tImpostor);
	//pRenderer->changeRenderTarget(m_tImpostor);

  RenderTargetBarrier barriersImposter[] = {
      { m_tImpostor, RESOURCE_STATE_RENDER_TARGET }
};

  cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersImposter);

	//Renderer.clear(true, false, false);
	//	TODO: A: clear to neutral color
	//	x is used for depth.
#ifdef	USE_MULTIPLICATIVE_DENSITY_ACCUMULTAION
	float color[] = {0.0f, 0.0f, 0.0f, 1.0f};
	pRenderer->clear(true, false, false, color);
#else	//	USE_MULTIPLICATIVE_DENSITY_ACCUMULTAION
	//pRenderer->clear(true, false, false);

  LoadActionsDesc loadActions = {};
  loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
  //loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
  loadActions.mClearColorValues[0].r = 0.0f;
  loadActions.mClearColorValues[0].g = 0.0f;
  loadActions.mClearColorValues[0].b = 0.0f;
  loadActions.mClearColorValues[0].a = 0.0f;
  //loadActions.mClearDepth = pDepthBuffer->mClearValue;

  cmdBindRenderTargets(cmd, 1, &m_tImpostor, NULL, &loadActions, NULL, NULL, -1, -1);
  cmdSetViewport(cmd, 0.0f, 0.0f, (float)m_tImpostor->mWidth, (float)m_tImpostor->mHeight, 0.0f, 1.0f);
  cmdSetScissor(cmd, 0, 0, m_tImpostor->mWidth, m_tImpostor->mHeight);

#endif	//	USE_MULTIPLICATIVE_DENSITY_ACCUMULTAION
	

	//	Just to make sure. distance should always be big enough.
	if (distance>0.0001)
	{
		//const float particleSize = pCloud->ParticlesScale();
		vec3 cloudDirUnit = cloudDir / distance;

		/////////////////////////////////////////////////
		//	Update impostor old data here
		{
			m_OldDistance = distance;
			m_OldNearPos = cloudPos-cloudDirUnit*m_Radius;
			m_OldFarPos = cloudPos+cloudDirUnit*m_Radius;
			m_Valid = true;
		}

    vec3 vRight = normalize(cross(camUp, cloudDirUnit));
    vec3 vUp = cross(cloudDirUnit, vRight);
		mat4 impostorView = mat4::identity();
		impostorView.setRow(0, vec4(vRight,0));
		impostorView.setRow(1, vec4(vUp, 0));
		impostorView.setRow(2, vec4(cloudDirUnit, 0));
		m_Transform = impostorView;
		impostorView.translation(-camPos);// translate(-camPos);

		float zNear = distance-m_Radius*1.1f;
		float zFar  = distance+m_Radius*1.1f;
		//	Don't want Z-near to be negative, and even less than camera near
		//	Update Z-far accordingly
		zNear = max(zNear, camNear);
		zFar = max(zFar, zNear+1);
		float ratio = m_Radius/distance;
		//	Igor: just for safety. When we are in the bounding sphere, this won't work well
		ratio = min(ratio,0.75f);
		float fov = m_Radius/sqrt(1-ratio*ratio);
		//float w = distance/fov;
		//	TODO: Igor: A: play with maths in order to figure out if this clamping might make any issues for the very big clouds
		//	Igor: fov clamping may enhance impostor resolution.
// 		const float camFov = 1.5f;
// 		w = max( w, cosf(0.5f * camFov) / sinf(0.5f * camFov) );
// 		fov = distance/w;

/*
		mat4 impostorProj = mat4(
			w, 0.0f, 0.0f, 0.0f,
			0.0f, w, 0.0f, 0.0f,
			0.0f, 0.0f, zFar / (zFar - zNear), -(zFar * zNear) / (zFar - zNear),
			0.0f, 0.0f, 1.0f, 0.0f);
*/

//    mat4 impostorProj = mat4(
//vec4(w, 0.0f, 0.0f, 0.0f),
//vec4(0.0f, w, 0.0f, 0.0f),
//vec4(0.0f, 0.0f, zFar / (zFar - zNear), 1.0f),
//vec4(0.0f, 0.0f, -(zFar * zNear) / (zFar - zNear), 0.0f)
//);

#ifdef	CLAMP_IMPOSTOR_PROJ
		//bWantProjClamping = true;
		if (bWantProjClamping)
		{
			m_ClampWindow = GetProjClampingRect(impostorProj, m_Transform, cameraCornerDirs,
				cloudDirUnit, distance);
			ClampImpostorProj(m_ClampWindow, impostorProj);
		}
		else
		{
			m_ClampWindow = float4(-1.0f, 1.0f, -1.0f, 1.0f);
			//m_ClampWindow = float4(-1.0f, 0.0f, -1.0f, 1.0f);
			//ClampImpostorProj(m_ClampWindow, impostorProj);
		}
#endif	//	CLAMP_IMPOSTOR_PROJ	

// 		Renderer.setShaderConstant4x4f("v", impostorView);
// 		Renderer.setShaderConstant4x4f("vp", impostorProj*impostorView);
// 
// 		Renderer.setShaderConstant3f("dx", impostorView.rows[0].xyz() * particleSize);
// 		Renderer.setShaderConstant3f("dy", impostorView.rows[1].xyz() * particleSize);

		//pRenderer->setShaderConstant4x4f("v", impostorView);
		//pRenderer->setShaderConstant4x4f("vp", impostorProj*impostorView);

		//pRenderer->setShaderConstant3f("dx", impostorView.getRow(0).getXYZ() * particleSize);
		//pRenderer->setShaderConstant3f("dy", impostorView.getRow(1).getXYZ() * particleSize);

		/////////////////////////////////
		//	Calculate impostor transform
		m_Transform.setRow(1, -m_Transform.getRow(1));	//	texture y is inverted, so jut do it here.
		//	our shader extrudes geometry along z instead of y so exchange z and y rows
		vec4 vTemp = m_Transform.getRow(1);// rows[1];
		m_Transform.setRow(1, m_Transform.getRow(2));
		m_Transform.setRow(2, vTemp);

		m_Transform = transpose(m_Transform);

		m_Transform = m_Transform*fov;
/*
		m_Transform.rows[3].w = 1.0f;
		m_Transform.rows[0].w = cloudPos.x;
		m_Transform.rows[1].w = cloudPos.y;
		m_Transform.rows[2].w = cloudPos.z;
*/
    m_Transform[3][3] = 1.0f;
    m_Transform[3][0] = cloudPos.getX();
    m_Transform[3][1] = cloudPos.getY();
    m_Transform[3][2] = cloudPos.getZ();


#ifdef	USE_CLOUDS_DEPTH_RECONSTRUCTION
		//	Calculate depth unpack params, set depth params
		//	Pack depth into 0..1
		vec2 packDepthParams;
		m_UnpackDepthParams.y = zNear;
		m_UnpackDepthParams.x = zFar-zNear;
		packDepthParams[0] = 1.0f / m_UnpackDepthParams.x;
		//packDepthParams.y = -m_UnpackDepthParams.y / m_UnpackDepthParams.x;
		packDepthParams[1] = -m_UnpackDepthParams.y * packDepthParams[0];

		//	Additional normalization in order to allow to reconstruct dir easier.
		m_UnpackDepthParams.y /= distance;
		m_UnpackDepthParams.x /= distance;

		//pRenderer->setShaderConstant2f("packDepthParams", packDepthParams);
#endif	//	USE_CLOUDS_DEPTH_RECONSTRUCTION

#ifdef	STABLISE_PARTICLE_ROTATION
		//pRenderer->setShaderConstant1f("masterParticleRotation", m_ParticleStabilize.updateMasterParticleRotation(vUp, vRight, cloudDirUnit));
    masterParticleRotation = m_ParticleStabilize.updateMasterParticleRotation(vUp, vRight, cloudDirUnit);
#endif	//	STABLISE_PARTICLE_ROTATION
	}
}

bool CloudImpostor::resolve()
{
	//bool bRes = pRenderer->resolveRenderTarget(m_tImpostor);
	//pRenderer->unbindResource(m_tImpostor);
	//return bRes;
   return false;
}

void CloudImpostor::InitFromCloud( CumulusCloud *pCloud )
{
	assert(pCloud);
	m_Radius = pCloud->getRadius();
}

#ifdef	STABLISE_PARTICLE_ROTATION
CloudImpostor::ParticleStabilizeData::ParticleStabilizeData() : m_Valid(false)
{

}

float CloudImpostor::ParticleStabilizeData::updateMasterParticleRotation( const vec3 & vUp, const vec3 & vRight, const vec3 & vForward )
{
	if (!m_Valid)
	{
		m_OldRight = vRight;
		m_OldMasterParticleRotation = 0.0f;
		m_MasterParticleRotation = 0.0f;
		m_Valid = true;
		return m_MasterParticleRotation;
	}

	//m_MasterParticleRotation += 0.1;

	//return m_MasterParticleRotation;

	vec3 OldRightReproj = m_OldRight - vForward*dot(vForward, m_OldRight);
	float OldRightReprojLength = length(OldRightReproj);

	const float OldRightReprojLengthThreshold = 0.1f;
	if (OldRightReprojLength<OldRightReprojLengthThreshold)
	{
		m_OldMasterParticleRotation = m_MasterParticleRotation;
		m_OldRight = vRight;
	}
	else
	{
		OldRightReproj /= OldRightReprojLength;

		float DP = dot(OldRightReproj,vRight);
		DP = clamp(DP, -1.0f, 1.0f);

		float delta = acos(DP);
		
		if (dot(OldRightReproj, vUp)<0)
			delta = -delta;

		m_MasterParticleRotation = m_OldMasterParticleRotation-delta;

		//	Normilize rotation
		if (m_MasterParticleRotation>PI*2)
		{
			m_MasterParticleRotation -= PI*2;
		}
		else if (m_MasterParticleRotation<-PI*2)
		{
			m_MasterParticleRotation += PI*2;
		}

		//const float OldRightReprojLengthUpdateThreshold = 0.4;
		//	Stands for 30 degrees.
		const float OldRightReprojLengthUpdateThreshold = 0.86602540378443864676372317075294f;
		if (OldRightReprojLength<OldRightReprojLengthUpdateThreshold)
		{
			m_OldMasterParticleRotation = m_MasterParticleRotation;
			m_OldRight = vRight;
		}
	}

	return m_MasterParticleRotation;
}
#endif	//	STABLISE_PARTICLE_ROTATION
