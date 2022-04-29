/*
* Copyright (c) 2017-2022 The Forge Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You can not use this code for commercial purposes.
*
*/

#include "SpaceObjects.h"

#include "../../../../The-Forge/Common_3/Renderer/IResourceLoader.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"

#define SKY_NEAR 50.0f
#define SKY_FAR 100000000.0f

#define SPACE_NEAR 100000.0f
#define SPACE_FAR 2000000000.0f

#define EARTH_RADIUS 6360000.0f

#define USING_MILKYWAY	0
#define USING_AURORA	0

//Precomputed Atmospheric Sacttering
Shader*           pSunShader = NULL;
Pipeline*         pSunPipeline = NULL;
DescriptorSet*    pSunDescriptorSet[2] = { NULL };
RootSignature*    pSunRootSignature = NULL;

Shader*           pStarShader = NULL;
Pipeline*         pStarPipeline = NULL;
DescriptorSet*    pStarDescriptorSet[2] = { NULL };
RootSignature*    pStarRootSignature = NULL;

//Shader*           pMilkyWayShader = NULL;
//Pipeline*         pMilkyWayPipeline = NULL;
//
//Shader*           pAuroraShader = NULL;
//Pipeline*         pAuroraPipeline = NULL;
//
//Shader*           pAuroraComputeShader = NULL;
//Pipeline*         pAuroraComputePipeline = NULL;

//DescriptorSet*    pSpaceObjectsDescriptorSet[2] = { NULL };
//RootSignature*    pSpaceObjectsRootSignature = NULL;
//RootSignature*    pSpaceObjectsRootSignatureCompute = NULL;

#if USING_MILKYWAY
Buffer*           pMilkyWayVertexBuffer = NULL;
Buffer*           pMilkyWayIndexBuffer = NULL;
uint32_t          MilkyWayIndexCount = 0;
#endif

Buffer*           pStarUniformBuffer[3] = { NULL };
Buffer*           pSunUniformBuffer[3] = { NULL };

static float g_ElapsedTime = 0.0f;

//Aurora                gAurora;

static float      SunSize = 20000000.0f;

static float      SpaceScale = EARTH_RADIUS * 100.0f;
//static float      NebulaScale = 9.453f;
//static float      StarIntensity = 1.5f;
//static float      StarDensity = 10.0f;
//static float      StarDistribution = 20000000.0f;
//static float ParticleScale = 100.0f;
//static float ParticleSize = 1000000.0f;

static mat4 rotMat = mat4::identity();
static mat4 rotMatStarField = mat4::identity();

struct SunUniformBuffer
{
	mat4 ViewMat;
	mat4 ViewProjMat;
	float4 LightDirection;
	float4 Dx;
	float4 Dy;
};

struct AuroraParticle
{
	float4 PrevPosition;			// PrePosition and movable flag
	float4 Position;				// Position and mass
	float4 Acceleration;
};

struct AuroraConstraint
{
	uint IndexP0;
	uint IndexP1;
	float RestDistance;
	float Padding00;
	float Padding01;
	float Padding02;
	float Padding03;
	float Padding04;
};

struct AuroraUniformStruct
{
	uint        maxVertex;
	float       heightOffset;
	float       height;
	float       deltaTime;

	mat4        ViewProjMat;
};

AuroraUniformStruct gAuroraUniformStruct;

//Buffer*               pAuroraVertexBuffer;
//Buffer*               pAuroraParticle;
//Buffer*               pAuroraConstraint;
//Buffer*               pAuroraUniformBuffer;

const float AuroraWidth = 100000.0f;
//const float AuroraHeight = 4000.0f;
const uint32_t AuroraParticleNum = 64;

void SpaceObjects::GenerateRing(eastl::vector<float> &vertices, eastl::vector<uint32_t> &indices, uint32_t WidthDividor, uint32_t HeightDividor, float radius, float height)
{

	float DegreeToRadian = PI / 180.0f;

	float angle = 360.0f / (float)WidthDividor * DegreeToRadian;

	float currentAngle;

	float heightGap = height / (float)HeightDividor;
	float y = height * 0.5f;

	for (uint32_t i = 0; i < HeightDividor; i++)
	{
		currentAngle = 0.0f;
		for (uint32_t w = 0; w <= WidthDividor; w++)
		{
			// POS
			vertices.push_back(cos(currentAngle) * radius);
			vertices.push_back(y);
			vertices.push_back(sin(currentAngle) * radius);

			vec3 normailzedVertex = normalize(vec3(cos(currentAngle), 0.0f, sin(currentAngle)));

			// NOR
			vertices.push_back(normailzedVertex.getX());
			vertices.push_back(0.0f);
			vertices.push_back(normailzedVertex.getZ());

			// Info
			vertices.push_back(0.0f);
			vertices.push_back(0.0f);
			vertices.push_back(0.0f);

			currentAngle += angle;
		}
		y -= heightGap;
	}

	uint WidthRow = (WidthDividor + 1);

	for (uint h = 0; h < HeightDividor - 1; h++)
	{
		for (uint w = 0; w < WidthDividor; w++)
		{
			indices.push_back(w + (h * WidthRow));
			indices.push_back(w + (WidthRow)+(h * WidthRow));
			indices.push_back(w + 1 + (h * WidthRow));

			indices.push_back(w + 1 + (h * WidthRow));
			indices.push_back(w + (WidthRow)+(h * WidthRow));
			indices.push_back(w + (WidthRow + 1) + (h * WidthRow));
		}
	}
}

bool SpaceObjects::Init(Renderer* const renderer, PipelineCache* pCache)
{
	/*
	Timer t;

	B_Spline test;
	eastl::vector<float> times;


	uint numberOfPts = 64;

	for (uint i = 0; i < numberOfPts; i++)
	{
	  times.push_back((float)i / (float)numberOfPts);
	}

	times.push_back(0.0f);
	times.push_back(0.1f);
	times.push_back(0.2f);
	times.push_back(0.3f);
	times.push_back(0.4f);
	times.push_back(0.5f);
	times.push_back(0.6f);
	times.push_back(0.7f);
	times.push_back(0.8f);
	times.push_back(0.9f);
	times.push_back(1.0f);

	eastl::vector<vec3> points;
	points.push_back(vec3(-1.0f, 0.0, 0.0f));
	points.push_back(vec3(-0.5f, 0.0, 0.5f));
	points.push_back(vec3(0.5f, 0.0, -0.5f));
	points.push_back(vec3(1.0f, 0.0, 0.0f));

	eastl::vector<float> knots;
	knots.push_back(0.0f);
	knots.push_back(0.0f);
	knots.push_back(0.0f);
	knots.push_back(1.0f);
	knots.push_back(2.0f);
	knots.push_back(2.0f);
	knots.push_back(2.0f);
	eastl::vector<float> weights;
	eastl::vector<vec3> results;
	test.interpolate(times, 2, points, knots, weights, results);

	LOGF(LogLevel::eINFO, "B_Spline CPU: %f", t.GetMSec(false) * 0.001f);
	*/


	//gAurora.Init(100000.0f,4000.0f,64, 1);



	eastl::vector< AuroraParticle > initialAuroraData;

	for (uint32_t x = 0; x < AuroraParticleNum; x++)
	{
		AuroraParticle tempParticle;
		tempParticle.Position = float4(AuroraWidth * ((float)x / (float)AuroraParticleNum), 0.0f, 0.0f, 1.0f);
		tempParticle.PrevPosition = float4(tempParticle.Position.getX(), tempParticle.Position.getY(), tempParticle.Position.getZ(), 1.0);
		tempParticle.Acceleration = float4(0.0f, 0.0f, 0.0f, 0.0f);

		initialAuroraData.push_back(tempParticle);
	}

	eastl::vector< AuroraConstraint > initialAuroraConstraintData;

	for (uint32_t x = 0; x < AuroraParticleNum - 1; x++)
	{
		AuroraConstraint tempParticle;
		tempParticle.IndexP0 = x;
		tempParticle.IndexP1 = x + 1;

		float3 p0 = initialAuroraData[x].Position.getXYZ();
		float3 p1 = initialAuroraData[x + 1].Position.getXYZ();

		float x2 = p0.getX() - p1.getX();
		x2 *= x2;

		float y2 = p0.getY() - p1.getY();
		y2 *= y2;

		float z2 = p0.getZ() - p1.getZ();
		z2 *= z2;

		tempParticle.RestDistance = sqrt(x2 + y2 + z2);

		tempParticle.Padding00 = 0.0f;
		tempParticle.Padding01 = 0.0f;
		tempParticle.Padding02 = 0.0f;
		tempParticle.Padding03 = 0.0f;
		tempParticle.Padding04 = 0.0f;

		initialAuroraConstraintData.push_back(tempParticle);
	}

	pRenderer = renderer;
	pPipelineCache = pCache;
	//////////////////////////////////// Samplers ///////////////////////////////////////////////////

	SamplerDesc samplerClampDesc = {
	FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
	ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE, ADDRESS_MODE_CLAMP_TO_EDGE
	};
	addSampler(pRenderer, &samplerClampDesc, &pLinearClampSampler);

	//////////////////////////////////// Samplers ///////////////////////////////////////////////////

	samplerClampDesc = {
	FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
	ADDRESS_MODE_CLAMP_TO_BORDER, ADDRESS_MODE_CLAMP_TO_BORDER, ADDRESS_MODE_CLAMP_TO_BORDER
	};

	addSampler(pRenderer, &samplerClampDesc, &pLinearBorderSampler);

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc sunShader = {};
	sunShader.mStages[0] = { "Sun.vert" , NULL, 0, NULL };
	sunShader.mStages[1] = { "Sun.frag", NULL, 0, NULL };
	addShader(pRenderer, &sunShader, &pSunShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc starShader = {};
	starShader.mStages[0] = { "Star.vert", NULL, 0, NULL };
	starShader.mStages[1] = { "Star.frag", NULL, 0, NULL };
	addShader(pRenderer, &starShader, &pStarShader);

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*ShaderLoadDesc MilkyWayShader = {};
	MilkyWayShader.mStages[0] = { "SpaceObjects/MilkyWay.vert", NULL, 0, NULL };
	MilkyWayShader.mStages[1] = { "SpaceObjects/MilkyWay.frag", NULL, 0, NULL };
	addShader(pRenderer, &MilkyWayShader, &pMilkyWayShader);*/

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*ShaderLoadDesc AuroraShader = {};
#if !defined(METAL)
	AuroraShader.mStages[0] = { "SpaceObjects/Aurora.vert", NULL, 0, NULL };
	AuroraShader.mStages[1] = { "SpaceObjects/Aurora.geom", NULL, 0, NULL };
	AuroraShader.mStages[2] = { "SpaceObjects/Aurora.frag", NULL, 0, NULL };
#else
	AuroraShader.mStages[0] = { "SpaceObjects/Aurora.vert", NULL, 0, NULL };
	AuroraShader.mStages[1] = { "SpaceObjects/Aurora.frag", NULL, 0, NULL };
#endif
	addShader(pRenderer, &AuroraShader, &pAuroraShader);*/

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*ShaderLoadDesc AuroraComputeShader = {};
	AuroraComputeShader.mStages[0] = { "SpaceObjects/Aurora.comp", NULL, 0, NULL };
	addShader(pRenderer, &AuroraComputeShader, &pAuroraComputeShader);*/

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	const char* pSkySamplerNames[] = { "g_LinearClamp", "g_LinearBorder" };
	Sampler* pSkySamplers[] = { pLinearClampSampler, pLinearBorderSampler };

	RootSignatureDesc sunRootDesc = {};
	sunRootDesc.mShaderCount = 1;
	sunRootDesc.ppShaders = &pSunShader;
	sunRootDesc.mStaticSamplerCount = 2;
	sunRootDesc.ppStaticSamplerNames = pSkySamplerNames;
	sunRootDesc.ppStaticSamplers = pSkySamplers;
	addRootSignature(pRenderer, &sunRootDesc, &pSunRootSignature);

	RootSignatureDesc starRootDesc = {};
	starRootDesc.mShaderCount = 1;
	starRootDesc.ppShaders = &pStarShader;
	starRootDesc.mStaticSamplerCount = 2;
	starRootDesc.ppStaticSamplerNames = pSkySamplerNames;
	starRootDesc.ppStaticSamplers = pSkySamplers;
	addRootSignature(pRenderer, &starRootDesc, &pStarRootSignature);

	/*Shader*           shaderComputes[] = { pAuroraComputeShader };
	rootDesc = {};
	rootDesc.mShaderCount = 1;
	rootDesc.ppShaders = shaderComputes;

	addRootSignature(pRenderer, &rootDesc, &pSpaceObjectsRootSignatureCompute);*/

	DescriptorSetDesc setDesc = { pSunRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
	addDescriptorSet(pRenderer, &setDesc, &pSunDescriptorSet[0]);
	setDesc = { pSunRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
	addDescriptorSet(pRenderer, &setDesc, &pSunDescriptorSet[1]);

	setDesc = { pStarRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
	addDescriptorSet(pRenderer, &setDesc, &pStarDescriptorSet[0]);
	setDesc = { pStarRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
	addDescriptorSet(pRenderer, &setDesc, &pStarDescriptorSet[1]);

	SyncToken token = {};

	float screenQuadPoints[] = {
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 0.5f, 0.0f, 0.0f
	};

	BufferLoadDesc TriangularVbDesc = {};
	TriangularVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	TriangularVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	TriangularVbDesc.mDesc.mSize = (uint64_t)(sizeof(float) * 5 * 4);
	TriangularVbDesc.pData = screenQuadPoints;
	TriangularVbDesc.ppBuffer = &pGlobalTriangularVertexBuffer;
	addResource(&TriangularVbDesc, &token);

	TextureLoadDesc SkyMoonTextureDesc = {};
	SkyMoonTextureDesc.pFileName = "SpaceObjects/Moon";
	SkyMoonTextureDesc.ppTexture = &pMoonTexture;
	// Textures representing color should be stored in SRGB or HDR format
	SkyMoonTextureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
	addResource(&SkyMoonTextureDesc, &token);

#if USING_MILKYWAY

	eastl::vector<float> MilkyWayVertices;
	eastl::vector<uint32_t> MilkyWayIndices;

	GenerateRing(MilkyWayVertices, MilkyWayIndices, 128, 10, 1000000.0f, 100000.0f);

	BufferLoadDesc MilkyWayVbDesc = {};
	MilkyWayVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	MilkyWayVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	MilkyWayVbDesc.mDesc.mSize = (uint64_t)MilkyWayVertices.size() / 3 * sizeof(float3) * 3;
	MilkyWayVbDesc.pData = MilkyWayVertices.data();
	MilkyWayVbDesc.ppBuffer = &pMilkyWayVertexBuffer;
	addResource(&MilkyWayVbDesc, &token);

	BufferLoadDesc MilkyWayIbDesc = {};
	MilkyWayIbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
	MilkyWayIbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	MilkyWayIbDesc.mDesc.mSize = (uint64_t)MilkyWayIndices.size() * sizeof(uint32_t);
	MilkyWayIbDesc.mDesc.mIndexType = INDEX_TYPE_UINT32;
	MilkyWayIbDesc.pData = MilkyWayIndices.data();
	MilkyWayIbDesc.ppBuffer = &pMilkyWayIndexBuffer;
	addResource(&MilkyWayIbDesc, &token);

	MilkyWayIndexCount = (uint32_t)MilkyWayIndices.size();

#endif

	// Generate sphere vertex buffer
	eastl::vector<float> IcosahedronVertices;
	eastl::vector<uint32_t> IcosahedronIndices;

	/*BufferLoadDesc AuroraVbDesc = {};
	AuroraVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	AuroraVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	AuroraVbDesc.mDesc.mSize = (uint64_t)AuroraParticleNum * sizeof(float3) * 3;
	AuroraVbDesc.ppBuffer = &pAuroraVertexBuffer;
	addResource(&AuroraVbDesc, &token);*/

	BufferLoadDesc AuroraParticleDesc = {};
	AuroraParticleDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER;
	AuroraParticleDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	AuroraParticleDesc.mDesc.mElementCount = (uint64_t)AuroraParticleNum;
	AuroraParticleDesc.mDesc.mStructStride = sizeof(AuroraParticle);
	AuroraParticleDesc.mDesc.mSize = AuroraParticleDesc.mDesc.mElementCount * AuroraParticleDesc.mDesc.mStructStride;
	AuroraParticleDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	AuroraParticleDesc.pData = initialAuroraData.data();

	// for(unsigned int i=0; i<gImageCount; i++)
	// {
	/*AuroraParticleDesc.ppBuffer = &pAuroraParticle;
	addResource(&AuroraParticleDesc, &token);*/
	// }

	/*BufferLoadDesc AuroraConstraintDesc = {};
	AuroraConstraintDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER;
	AuroraConstraintDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	AuroraConstraintDesc.mDesc.mElementCount = (uint64_t)AuroraParticleNum - 1;
	AuroraConstraintDesc.mDesc.mStructStride = sizeof(AuroraConstraint);
	AuroraConstraintDesc.mDesc.mSize = AuroraConstraintDesc.mDesc.mElementCount * AuroraConstraintDesc.mDesc.mStructStride;
	AuroraConstraintDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	AuroraConstraintDesc.pData = initialAuroraConstraintData.data();*/

	// for (unsigned int i = 0; i < gImageCount; i++)
	// {
	/*AuroraConstraintDesc.ppBuffer = &pAuroraConstraint;
	addResource(&AuroraConstraintDesc, &token);*/
	// }


	/*BufferLoadDesc AuroraUniformDesc = {};
	AuroraUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	AuroraUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	AuroraUniformDesc.mDesc.mElementCount = 1;
	AuroraUniformDesc.mDesc.mStructStride = sizeof(AuroraUniformStruct);
	AuroraUniformDesc.mDesc.mSize = AuroraUniformDesc.mDesc.mElementCount * AuroraUniformDesc.mDesc.mStructStride;
	AuroraUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;*/

	// for (unsigned int i = 0; i < gImageCount; i++)
	// {
	/*AuroraUniformDesc.ppBuffer = &pAuroraUniformBuffer;
	addResource(&AuroraUniformDesc, &token);*/
	// }

	BufferLoadDesc sunUniformDesc = {};
	sunUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sunUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	sunUniformDesc.mDesc.mSize = sizeof(SunUniformBuffer);
	sunUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	sunUniformDesc.pData = NULL;

	for (uint i = 0; i < gImageCount; i++)
	{
		sunUniformDesc.ppBuffer = &pSunUniformBuffer[i];
		addResource(&sunUniformDesc, &token);

		sunUniformDesc.ppBuffer = &pStarUniformBuffer[i];
		addResource(&sunUniformDesc, &token);
	}

	///////////////////////////////////////////////////////////////////
	// UI
	///////////////////////////////////////////////////////////////////

	UIComponentDesc UIComponentDesc = {};
	float dpiScale[2];
	getDpiScale(dpiScale);
	UIComponentDesc.mStartPosition = vec2(1280.0f / dpiScale[0], 700.0f / dpiScale[1]);
	UIComponentDesc.mStartSize = vec2(300.0f / dpiScale[0], 250.0f / dpiScale[1]);
	uiCreateComponent("Space Objects", &UIComponentDesc, &pGuiWindow);
	
	waitForToken(&token);

	return true;
}

void SpaceObjects::Exit()
{
	removeDescriptorSet(pRenderer, pSunDescriptorSet[0]);
	removeDescriptorSet(pRenderer, pSunDescriptorSet[1]);
	removeDescriptorSet(pRenderer, pStarDescriptorSet[0]);
	removeDescriptorSet(pRenderer, pStarDescriptorSet[1]);
	
	/*removeShader(pRenderer, pMilkyWayShader);
	removeShader(pRenderer, pAuroraShader);
	removeShader(pRenderer, pAuroraComputeShader);*/
	removeShader(pRenderer, pSunShader);
	removeShader(pRenderer, pStarShader);

	removeRootSignature(pRenderer, pSunRootSignature);
	removeRootSignature(pRenderer, pStarRootSignature);
	//removeRootSignature(pRenderer, pSpaceObjectsRootSignatureCompute);

	removeSampler(pRenderer, pLinearClampSampler);
	removeSampler(pRenderer, pLinearBorderSampler);

	//removeResource(pAuroraVertexBuffer);

	/*
  for (uint i = 0; i < gImageCount; i++)
  {
	removeResource(pAuroraParticle[i]);
	removeResource(pAuroraConstraint[i]);
	removeResource(pAuroraUniformBuffer[i]);
  }
  */

	/*removeResource(pAuroraParticle);
	removeResource(pAuroraConstraint);
	removeResource(pAuroraUniformBuffer);*/

#if USING_MILKYWAY
	removeResource(pMilkyWayVertexBuffer);
	removeResource(pMilkyWayIndexBuffer);
#endif

	for (uint i = 0; i < gImageCount; i++)
	{
		removeResource(pSunUniformBuffer[i]);
		removeResource(pStarUniformBuffer[i]);
	}
	removeResource(pMoonTexture);

	removeResource(pGlobalTriangularVertexBuffer);
}

bool SpaceObjects::Load(RenderTarget** rts, uint32_t count)
{
	pPreStageRenderTarget = rts[0];

	mWidth = pPreStageRenderTarget->mWidth;
	mHeight = pPreStageRenderTarget->mHeight;

	float aspect = (float)mWidth / (float)mHeight;
	float aspectInverse = 1.0f / aspect;
	float horizontal_fov = PI / 3.0f;

	SpaceProjectionMatrix = mat4::perspective(horizontal_fov, aspectInverse, SPACE_NEAR, SPACE_FAR);

	//layout and pipeline for ScreenQuad
	VertexLayout vertexLayout = {};
	vertexLayout.mAttribCount = 2;
	vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
	vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
	vertexLayout.mAttribs[0].mBinding = 0;
	vertexLayout.mAttribs[0].mLocation = 0;
	vertexLayout.mAttribs[0].mOffset = 0;

	vertexLayout.mAttribs[1].mSemantic = SEMANTIC_TEXCOORD0;
	vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32_SFLOAT;
	vertexLayout.mAttribs[1].mBinding = 0;
	vertexLayout.mAttribs[1].mLocation = 1;
	vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);

	vertexLayout = {};
	vertexLayout.mAttribCount = 3;
	vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
	vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
	vertexLayout.mAttribs[0].mBinding = 0;
	vertexLayout.mAttribs[0].mLocation = 0;
	vertexLayout.mAttribs[0].mOffset = 0;
	vertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
	vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
	vertexLayout.mAttribs[1].mBinding = 0;
	vertexLayout.mAttribs[1].mLocation = 1;
	vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);
	vertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD0;
	vertexLayout.mAttribs[2].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
	vertexLayout.mAttribs[2].mBinding = 0;
	vertexLayout.mAttribs[2].mLocation = 2;
	vertexLayout.mAttribs[2].mOffset = 6 * sizeof(float);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	RasterizerStateDesc rasterizerStateDesc = {};
	rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

	BlendStateDesc blendStateSunDesc = {};
	blendStateSunDesc.mBlendModes[0] = BM_ADD;
	blendStateSunDesc.mBlendAlphaModes[0] = BM_ADD;

	blendStateSunDesc.mSrcFactors[0] = BC_SRC_ALPHA;
	blendStateSunDesc.mDstFactors[0] = BC_ONE;

	blendStateSunDesc.mSrcAlphaFactors[0] = BC_ONE;
	blendStateSunDesc.mDstAlphaFactors[0] = BC_ZERO;

	blendStateSunDesc.mMasks[0] = ALL;
	blendStateSunDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	BlendStateDesc blendStateAdditiveDesc = {};
	blendStateAdditiveDesc.mBlendModes[0] = BM_ADD;
	blendStateAdditiveDesc.mBlendAlphaModes[0] = BM_ADD;

	blendStateAdditiveDesc.mSrcFactors[0] = BC_SRC_ALPHA;
	blendStateAdditiveDesc.mDstFactors[0] = BC_ONE;

	blendStateAdditiveDesc.mSrcAlphaFactors[0] = BC_SRC_ALPHA;
	blendStateAdditiveDesc.mDstAlphaFactors[0] = BC_ONE;

	blendStateAdditiveDesc.mMasks[0] = ALL;
	blendStateAdditiveDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//PipelineDesc pipelineDescMilkyWay = {};
	//pipelineDescMilkyWay.pCache = pPipelineCache;
	//{
	//	pipelineDescMilkyWay.mType = PIPELINE_TYPE_GRAPHICS;
	//	GraphicsPipelineDesc &pipelineSettings = pipelineDescMilkyWay.mGraphicsDesc;

	//	pipelineSettings = { 0 };

	//	pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
	//	pipelineSettings.mRenderTargetCount = 1;

	//	pipelineSettings.pColorFormats = &pPreStageRenderTarget->mFormat;
	//	pipelineSettings.mSampleCount = pPreStageRenderTarget->mSampleCount;
	//	pipelineSettings.mSampleQuality = pPreStageRenderTarget->mSampleQuality;

	//	pipelineSettings.pRootSignature = pSpaceObjectsRootSignature;
	//	pipelineSettings.pShaderProgram = pMilkyWayShader;
	//	pipelineSettings.pVertexLayout = &vertexLayout;
	//	pipelineSettings.pRasterizerState = &rasterizerStateDesc;
	//	//pipelineSettings.pBlendState = pBlendStateSpace;
	//	pipelineSettings.pBlendState = &blendStateSunDesc;

	//	addPipeline(pRenderer, &pipelineDescMilkyWay, &pMilkyWayPipeline);
	//}

	//PipelineDesc pipelineDescAurora = {};
	//pipelineDescAurora.pCache = pPipelineCache;
	//{
	//	pipelineDescAurora.mType = PIPELINE_TYPE_GRAPHICS;
	//	GraphicsPipelineDesc &pipelineSettings = pipelineDescAurora.mGraphicsDesc;

	//	pipelineSettings = { 0 };

	//	pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_POINT_LIST;
	//	pipelineSettings.mRenderTargetCount = 1;

	//	pipelineSettings.pColorFormats = &pPreStageRenderTarget->mFormat;
	//	pipelineSettings.mSampleCount = pPreStageRenderTarget->mSampleCount;
	//	pipelineSettings.mSampleQuality = pPreStageRenderTarget->mSampleQuality;

	//	pipelineSettings.pRootSignature = pSpaceObjectsRootSignature;
	//	pipelineSettings.pShaderProgram = pAuroraShader;
	//	pipelineSettings.pRasterizerState = &rasterizerStateDesc;
	//	pipelineSettings.pBlendState = &blendStateSunDesc;

	//	addPipeline(pRenderer, &pipelineDescAurora, &pAuroraPipeline);
	//}

	//PipelineDesc pipelineDescAuroraCompute = {};
	//pipelineDescAuroraCompute.pCache = pPipelineCache;
	//{
	//	pipelineDescAuroraCompute.mType = PIPELINE_TYPE_COMPUTE;
	//	ComputePipelineDesc &pipelineSettings = pipelineDescAuroraCompute.mComputeDesc;

	//	pipelineSettings = { 0 };

	//	pipelineSettings.pRootSignature = pSpaceObjectsRootSignatureCompute;
	//	pipelineSettings.pShaderProgram = pAuroraComputeShader;

	//	addPipeline(pRenderer, &pipelineDescAuroraCompute, &pAuroraComputePipeline);
	//}


	PipelineDesc pipelineDesSun = {};
	pipelineDesSun.pCache = pPipelineCache;
	{
		pipelineDesSun.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDesSun.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_STRIP;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pColorFormats = &pPreStageRenderTarget->mFormat;
		pipelineSettings.mSampleCount = pPreStageRenderTarget->mSampleCount;
		pipelineSettings.mSampleQuality = pPreStageRenderTarget->mSampleQuality;
		pipelineSettings.pRootSignature = pSunRootSignature;
		pipelineSettings.pShaderProgram = pSunShader;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;
		pipelineSettings.pBlendState = &blendStateSunDesc;

		addPipeline(pRenderer, &pipelineDesSun, &pSunPipeline);
	}

	vertexLayout = {};
	vertexLayout.mAttribCount = 3;

	vertexLayout.mAttribs[0].mSemantic = SEMANTIC_TEXCOORD0;
	vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
	vertexLayout.mAttribs[0].mBinding = 0;
	vertexLayout.mAttribs[0].mLocation = 0;
	vertexLayout.mAttribs[0].mOffset = 0;
	vertexLayout.mAttribs[0].mRate = VERTEX_ATTRIB_RATE_INSTANCE;

	vertexLayout.mAttribs[1].mSemantic = SEMANTIC_TEXCOORD1;
	vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
	vertexLayout.mAttribs[1].mBinding = 0;
	vertexLayout.mAttribs[1].mLocation = 1;
	vertexLayout.mAttribs[1].mOffset = 4 * sizeof(float);
	vertexLayout.mAttribs[1].mRate = VERTEX_ATTRIB_RATE_INSTANCE;

	vertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD2;
	vertexLayout.mAttribs[2].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
	vertexLayout.mAttribs[2].mBinding = 0;
	vertexLayout.mAttribs[2].mLocation = 2;
	vertexLayout.mAttribs[2].mOffset = 8 * sizeof(float);
	vertexLayout.mAttribs[2].mRate = VERTEX_ATTRIB_RATE_INSTANCE;

	PipelineDesc pipelineDescStar = {};
	pipelineDescStar.pCache = pPipelineCache;
	{
		pipelineDescStar.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescStar.mGraphicsDesc;

		pipelineSettings = { 0 };
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_STRIP;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pColorFormats = &pPreStageRenderTarget->mFormat;
		pipelineSettings.mSampleCount = pPreStageRenderTarget->mSampleCount;
		pipelineSettings.mSampleQuality = pPreStageRenderTarget->mSampleQuality;

		pipelineSettings.pRootSignature = pStarRootSignature;
		pipelineSettings.pShaderProgram = pStarShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;
		pipelineSettings.pBlendState = &blendStateAdditiveDesc;

		addPipeline(pRenderer, &pipelineDescStar, &pStarPipeline);
	}

	// Prepare descriptor sets
	// Sky
	{
		DescriptorData ScParams[8] = {};	
		ScParams[0].pName = "depthTexture";
		ScParams[0].ppTextures = &pDepthBuffer->pTexture;
		ScParams[1].pName = "volumetricCloudsTexture";
		ScParams[1].ppTextures = &pSavePrevTexture;
#if defined(ORBIS)
		ScParams[2].pName = "starInstanceBuffer";
		ScParams[2].ppBuffers = &pParticleInstanceBuffer;
		updateDescriptorSet(pRenderer, 0, pStarDescriptorSet[0], 3, ScParams);
#else
		updateDescriptorSet(pRenderer, 0, pStarDescriptorSet[0], 2, ScParams);
#endif
		ScParams[2] = {};
		ScParams[2].pName = "moonAtlas";
		ScParams[2].ppTextures = &pMoonTexture;
		updateDescriptorSet(pRenderer, 0, pSunDescriptorSet[0], 3, ScParams);

		ScParams[0] = {};
		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			ScParams[0].pName = "StarUniform";
			ScParams[0].ppBuffers = &pStarUniformBuffer[i];
			updateDescriptorSet(pRenderer, i, pStarDescriptorSet[1], 1, ScParams);

			ScParams[0].pName = "SunUniform";
			ScParams[0].ppBuffers = &pSunUniformBuffer[i];
			updateDescriptorSet(pRenderer, i, pSunDescriptorSet[1], 1, ScParams);
		}
	}

	return false;
}

void SpaceObjects::Unload()
{	
	/*removePipeline(pRenderer, pMilkyWayPipeline);
	removePipeline(pRenderer, pAuroraPipeline);
	removePipeline(pRenderer, pAuroraComputePipeline);*/
	removePipeline(pRenderer, pSunPipeline);
	removePipeline(pRenderer, pStarPipeline);
}

void SpaceObjects::Update(float deltaTime)
{
	g_ElapsedTime += deltaTime;

	/*
  gAurora.addForce(normalize(vec3(1.0f, 0.0f, 1.0f)));
  gAurora.update(deltaTime);

  eastl::vector<float3> tempPos;

  for(uint i = 0; i < (uint)gAurora.particles.size(); i++)
  {
	tempPos.push_back( v3ToF3(gAurora.particles[i].position) );
  }

  BufferUpdateDesc BufferAuroraInfoSettingDesc = { pAuroraInfoBuffer[gFrameIndex], tempPos.data() };
  updateResource(&BufferAuroraInfoSettingDesc);
  

	gAuroraUniformStruct.maxVertex = (uint32_t)AuroraParticleNum;
	gAuroraUniformStruct.heightOffset = 4000.0f;
	gAuroraUniformStruct.height = 4000.0f;
	gAuroraUniformStruct.deltaTime = deltaTime;
	gAuroraUniformStruct.ViewProjMat = SkyProjectionMatrix * pCameraController->getViewMatrix();

	//BufferUpdateDesc BufferAuroraUniformSettingDesc = { pAuroraUniformBuffer[gFrameIndex], &gAuroraUniformStruct };
	BufferUpdateDesc BufferAuroraUniformSettingDesc = { pAuroraUniformBuffer, &gAuroraUniformStruct };
	updateResource(&BufferAuroraUniformSettingDesc);
	*/

	rotMat = mat4::translation(vec3(0.0f, -EARTH_RADIUS * 10.0f, 0.0f)) * (mat4::rotationY(-Azimuth) * mat4::rotationZ(Elevation)) * mat4::translation(vec3(0.0f, EARTH_RADIUS* 10.0f, 0.0f));
	rotMatStarField = (mat4::rotationY(-Azimuth) * mat4::rotationZ(Elevation));
}


void SpaceObjects::Draw(Cmd* cmd)
{
	cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Space Objects");

	vec4 lightDir = vec4(f3Tov3(LightDirection));

	
	/////////////////////////////////////////////////////////////////////////////////////

	if (lightDir.getY() < 0.2f)
	{
#if USING_MILKYWAY
		{
			cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw MilkyWay");

			TextureBarrier barriersSky[] =
			{
			  { pPreStageRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET }
			};

			cmdResourceBarrier(cmd, 0, NULL, 1, barriersSky, false);

			cmdBindRenderTargets(cmd, 1, &pPreStageRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
			cmdSetViewport(cmd, 0.0f, 0.0f, (float)pPreStageRenderTarget->mWidth, (float)pPreStageRenderTarget->mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pPreStageRenderTarget->mWidth, pPreStageRenderTarget->mHeight);

			DescriptorData MilkyWayParams[2] = {};

			cmdBindPipeline(cmd, pMilkyWayPipeline);

			MilkyWayParams[0].pName = "SpaceUniform";
			MilkyWayParams[0].ppBuffers = &pSpaceUniformBuffer[gFrameIndex];
			MilkyWayParams[1].pName = "depthTexture";
			MilkyWayParams[1].ppTextures = &pDepthBuffer->pTexture;

			cmdBindDescriptors(cmd, pSkyDescriptorBinder, pSkyRootSignature, 2, MilkyWayParams);

			cmdBindVertexBuffer(cmd, 1, &pMilkyWayVertexBuffer, NULL);
			cmdBindIndexBuffer(cmd, pMilkyWayIndexBuffer, 0);
			cmdDrawIndexed(cmd, (uint32_t)MilkyWayIndexCount, 0, 0);

			cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

			cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
		}
#endif
	}

	////////////////////////////////////////////////////////////////////////

	{
		cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Starfield");

		RenderTargetBarrier barriersSky2[] =
		{
			{ pPreStageRenderTarget, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET }
		};

		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersSky2);

		cmdBindRenderTargets(cmd, 1, &pPreStageRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pPreStageRenderTarget->mWidth, (float)pPreStageRenderTarget->mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pPreStageRenderTarget->mWidth, pPreStageRenderTarget->mHeight);

		struct StarData
		{
			mat4 RotMat;
			mat4 ViewProjMat;
			float4 LightDirection;
			float4 Dx;
			float4 Dy;
		} starData;

		const mat4 viewMatrix = pCameraController->getViewMatrix();

		starData.RotMat = rotMat;
		starData.ViewProjMat = SpaceProjectionMatrix * viewMatrix;
		starData.LightDirection = float4(LightDirection, g_ElapsedTime * 2.0f);

		starData.Dx = v4ToF4(viewMatrix.getRow(0));
		starData.Dy = v4ToF4(viewMatrix.getRow(1));

		BufferUpdateDesc BufferUniformSettingDesc2 = { pStarUniformBuffer[gFrameIndex] };
		beginUpdateResource(&BufferUniformSettingDesc2);
		*(StarData*)BufferUniformSettingDesc2.pMappedData = starData;
		endUpdateResource(&BufferUniformSettingDesc2, NULL);

		cmdBindPipeline(cmd, pStarPipeline);
		cmdBindDescriptorSet(cmd, 0, pStarDescriptorSet[0]);
		cmdBindDescriptorSet(cmd, gFrameIndex, pStarDescriptorSet[1]);

#if !defined(ORBIS)
		const uint32_t strides[] = { ParticleInstanceStride };
		Buffer* particleVertexBuffers[] = { pParticleInstanceBuffer };
		cmdBindVertexBuffer(cmd, 1, particleVertexBuffers, strides, NULL);
#endif

		cmdDrawInstanced(cmd, 4, 0, ParticleCount, 0);

		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	}

	{
		cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Sun and Moon");

		cmdBindRenderTargets(cmd, 1, &pPreStageRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pPreStageRenderTarget->mWidth, (float)pPreStageRenderTarget->mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pPreStageRenderTarget->mWidth, pPreStageRenderTarget->mHeight);

		struct Data
		{
			mat4 ViewMat;
			mat4 ViewProjMat;
			float4 LightDirection;
			float4 Dx;
			float4 Dy;
		} data;

		data.ViewMat = pCameraController->getViewMatrix();
		data.ViewProjMat = SpaceProjectionMatrix * pCameraController->getViewMatrix();
		data.LightDirection = float4(LightDirection * SpaceScale, 0.0f);
		data.Dx = v4ToF4(pCameraController->getViewMatrix().getRow(0) * SunSize);
		data.Dy = v4ToF4(pCameraController->getViewMatrix().getRow(1) * SunSize);

		BufferUpdateDesc BufferUniformSettingDesc = { pSunUniformBuffer[gFrameIndex] };
		beginUpdateResource(&BufferUniformSettingDesc);
		*(Data*)BufferUniformSettingDesc.pMappedData = data;
		endUpdateResource(&BufferUniformSettingDesc, NULL);

		cmdBindPipeline(cmd, pSunPipeline);
		cmdBindDescriptorSet(cmd, 0, pSunDescriptorSet[0]);
		cmdBindDescriptorSet(cmd, gFrameIndex, pSunDescriptorSet[1]);

		cmdDraw(cmd, 4, 0);

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		RenderTargetBarrier barriersSkyEnd[] = {
			{ pPreStageRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriersSkyEnd);

		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	}
	////////////////////////////////////////////////////////////////////////
  /*
  #if !defined(METAL)
	{
	  TextureBarrier barriersSky[] =
	  {
		{ pSkyRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET }
	  };

	  BufferBarrier barrierAurora[] =
	  {
		{ pAuroraParticle, RESOURCE_STATE_UNORDERED_ACCESS },
		{ pAuroraConstraint, RESOURCE_STATE_UNORDERED_ACCESS },
		{ pAuroraUniformBuffer, RESOURCE_STATE_SHADER_RESOURCE }
	  };

	  cmdResourceBarrier(cmd, 2, barrierAurora, 1, barriersSky, false);

	  cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Update Aurora", true);

	  cmdBindPipeline(cmd, pAuroraComputePipeline);

	  DescriptorData AuroraComputeParams[3] = {};
	  AuroraComputeParams[0].pName = "AuroraParticleBuffer";
	  AuroraComputeParams[0].ppBuffers = &pAuroraParticle;

	  AuroraComputeParams[1].pName = "AuroraConstraintBuffer";
	  AuroraComputeParams[1].ppBuffers = &pAuroraConstraint;

	  AuroraComputeParams[2].pName = "AuroraUniformBuffer";
	  AuroraComputeParams[2].ppBuffers = &pAuroraUniformBuffer;

	  cmdBindDescriptors(cmd, pSkyDescriptorBinder, pSkyRootSignatureCompute, 3, AuroraComputeParams);
	  uint32_t* pThreadGroupSize = pAuroraComputeShader->mReflection.mStageReflections[0].mNumThreadsPerGroup;
	  cmdDispatch(cmd, (uint32_t)ceil((float)AuroraParticleNum / (float)pThreadGroupSize[0]), 1, 1);

	  cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

	  cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Aurora", true);

	  cmdBindRenderTargets(cmd, 1, &pSkyRenderTarget, NULL, NULL, NULL, NULL, -1, -1);
	  cmdSetViewport(cmd, 0.0f, 0.0f, (float)pSkyRenderTarget->mWidth, (float)pSkyRenderTarget->mHeight, 0.0f, 1.0f);
	  cmdSetScissor(cmd, 0, 0, pSkyRenderTarget->mWidth, pSkyRenderTarget->mHeight);

	  DescriptorData AuroraParams[2] = {};

	  cmdBindPipeline(cmd, pAuroraPipeline);

	  AuroraParams[0].pName = "AuroraParticleBuffer";
	  AuroraParams[0].ppBuffers = &pAuroraParticle;
	  AuroraParams[1].pName = "AuroraUniformBuffer";
	  AuroraParams[1].ppBuffers = &pAuroraUniformBuffer;



	  cmdBindDescriptors(cmd, pSkyDescriptorBinder, pSkyRootSignature, 2, AuroraParams);

	  cmdBindVertexBuffer(cmd, 1, &pAuroraVertexBuffer, NULL);
	  cmdDraw(cmd, (uint32_t)AuroraParticleNum, 0);
	  cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

	  cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	}
  #else
	{
	  cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Aurora", true);

	  cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	}
  #endif
  */

	cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
}

void SpaceObjects::Initialize(uint InImageCount,
	ICameraController* InCameraController, Queue*	InGraphicsQueue, CmdPool* InTransCmdPool,
	Cmd** InTransCmds, Fence* InTransitionCompleteFences, ProfileToken InGraphicsGpuProfiler, Buffer*	InTransmittanceBuffer)
{
	gImageCount = InImageCount;

	pCameraController = InCameraController;
	pGraphicsQueue = InGraphicsQueue;
	pTransCmdPool = InTransCmdPool;
	ppTransCmds = InTransCmds;
	pTransitionCompleteFences = InTransitionCompleteFences;
	gGpuProfileToken = InGraphicsGpuProfiler;
}

void SpaceObjects::InitializeWithLoad(RenderTarget* InDepthRenderTarget, RenderTarget* InLinearDepthRenderTarget,
	Texture* SavePrevTexture,
	Buffer* ParticleVertexBuffer, Buffer* ParticleInstanceBuffer, uint32_t ParticleCountParam,
	uint32_t ParticleVertexStrideParam, uint32_t ParticleInstanceStrideParam)
{
	pDepthBuffer = InDepthRenderTarget;
	pLinearDepthBuffer = InLinearDepthRenderTarget;

	pSavePrevTexture = SavePrevTexture;
	pParticleVertexBuffer = ParticleVertexBuffer;
	pParticleInstanceBuffer = ParticleInstanceBuffer;
	ParticleCount = ParticleCountParam;
	ParticleVertexStride = ParticleVertexStrideParam;
	ParticleInstanceStride = ParticleInstanceStrideParam;
}
