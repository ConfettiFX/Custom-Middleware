/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#include "Terrain.h"

#include "../../../../The-Forge/Common_3/Renderer/IResourceLoader.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../../../../The-Forge/Common_3/OS/Interfaces/IMemory.h"

char TextureTileFilePaths[5][255] = {
  "Terrain/Tiles/grass_DM",
  "Terrain/Tiles/cliff_DM",
  "Terrain/Tiles/snow_DM",
  "Terrain/Tiles/grassDark_DM",
  "Terrain/Tiles/gravel_DM" };

char TextureNormalTileFilePaths[5][255] = {
  "Terrain/Tiles/grass_NM",
  "Terrain/Tiles/cliff_NM",
  "Terrain/Tiles/Snow_NM",
  "Terrain/Tiles/grass_NM",
  "Terrain/Tiles/gravel_NM" };

// Pre Stage
Shader*                   pGenTerrainNormalShader = NULL;
Pipeline*                 pGenTerrainNormalPipeline = NULL;
RootSignature*            pGenTerrainNormalRootSignature = NULL;
DescriptorSet*            pGenTerrainNormalDescriptorSet = NULL;

// Draw Stage
Shader*                   pTerrainShader = NULL;
Pipeline*                 pTerrainPipeline = NULL;

Shader*                   pRenderTerrainShader = NULL;
Pipeline*                 pRenderTerrainPipeline = NULL;

Shader*                   pLightingTerrainShader = NULL;
Pipeline*                 pLightingTerrainPipeline = NULL;

RootSignature*            pTerrainRootSignature = NULL;
DescriptorSet*            pTerrainDescriptorSet[2] = { NULL };

static vec4	 g_StandardPosition;

#define TERRAIN_NEAR 50.0f
#define TERRAIN_FAR 100000000.0f
#define EARTH_RADIUS 6360000.0f
#define HEIGHT_SCALE 2.0f

struct RenderTerrainUniformBuffer
{
	mat4 projView;
	float4 TerrainInfo;
	float4 CameraInfo;
};

struct LightingTerrainUniformBuffer
{
	mat4	InvViewProjMat;
	mat4	ShadowViewProjMat;
	float4	ShadowSpheres;
	float4	LightDirection;
	float4	SunColor;
	float4	LightColor;
};

static bool boxIntersects(TerrainFrustum &frustum, TerrainBoundingBox &box)
{
	Plane* planes = frustum.CPlanes.planes;
	Plane *currPlane;
	Vector3 *currNormal;
	Vector3 maxPoint;

	for (int planeIdx = 0; planeIdx < 6; planeIdx++)
	{
		currPlane = planes + planeIdx;
		currNormal = &currPlane->normal;

		maxPoint.setX((currNormal->getX() > 0) ? box.max.x : box.min.x);
		maxPoint.setY((currNormal->getY() > 0) ? box.max.y : box.min.y);
		maxPoint.setZ((currNormal->getZ() > 0) ? box.max.z : box.min.z);

		float dMax = dot(maxPoint, *currNormal) + currPlane->distance;

		if (dMax < 0)
			return false;
	}
	return true;
}

static void getFrustumFromMatrix(const mat4 &matrix, TerrainFrustum &frustum)
{
	// Left clipping plane 
	frustum.iPlanes.left.normal.setX(matrix[0][3] + matrix[0][0]);
	frustum.iPlanes.left.normal.setY(matrix[1][3] + matrix[1][0]);
	frustum.iPlanes.left.normal.setZ(matrix[2][3] + matrix[2][0]);
	frustum.iPlanes.left.distance = matrix[3][3] + matrix[3][0];

	// Right clipping plane 
	frustum.iPlanes.right.normal.setX(matrix[0][3] - matrix[0][0]);
	frustum.iPlanes.right.normal.setY(matrix[1][3] - matrix[1][0]);
	frustum.iPlanes.right.normal.setZ(matrix[2][3] - matrix[2][0]);
	frustum.iPlanes.right.distance = matrix[3][3] - matrix[3][0];

	// Top clipping plane 
	frustum.iPlanes.top.normal.setX(matrix[0][3] - matrix[0][1]);
	frustum.iPlanes.top.normal.setY(matrix[1][3] - matrix[1][1]);
	frustum.iPlanes.top.normal.setZ(matrix[2][3] - matrix[2][1]);
	frustum.iPlanes.top.distance = matrix[3][3] - matrix[3][1];

	// Bottom clipping plane 
	frustum.iPlanes.bottom.normal.setX(matrix[0][3] + matrix[0][1]);
	frustum.iPlanes.bottom.normal.setY(matrix[1][3] + matrix[1][1]);
	frustum.iPlanes.bottom.normal.setZ(matrix[2][3] + matrix[2][1]);
	frustum.iPlanes.bottom.distance = matrix[3][3] + matrix[3][1];

	// Near clipping plane 
	frustum.iPlanes.front.normal.setX(matrix[0][2]);
	frustum.iPlanes.front.normal.setY(matrix[1][2]);
	frustum.iPlanes.front.normal.setZ(matrix[2][2]);
	frustum.iPlanes.front.distance = matrix[3][2];

	// Far clipping plane 
	frustum.iPlanes.back.normal.setX(matrix[0][3] - matrix[0][2]);
	frustum.iPlanes.back.normal.setY(matrix[1][3] - matrix[1][2]);
	frustum.iPlanes.back.normal.setZ(matrix[2][3] - matrix[2][2]);
	frustum.iPlanes.back.distance = matrix[3][3] - matrix[3][2];
}

bool Terrain::Init(Renderer* renderer, PipelineCache* pCache)
{
	pRenderer = renderer;
	pPipelineCache = pCache;

	g_StandardPosition = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	//////////////////////////////////// Samplers ///////////////////////////////////////////////////
	SamplerDesc samplerClampDesc = {
	FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
	ADDRESS_MODE_MIRROR, ADDRESS_MODE_MIRROR, ADDRESS_MODE_MIRROR
	};
	addSampler(pRenderer, &samplerClampDesc, &pLinearMirrorSampler);

	samplerClampDesc = {
		FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
		ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT
	};
	addSampler(pRenderer, &samplerClampDesc, &pLinearWrapSampler);

	samplerClampDesc = {
	FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
	ADDRESS_MODE_CLAMP_TO_BORDER, ADDRESS_MODE_CLAMP_TO_BORDER, ADDRESS_MODE_CLAMP_TO_BORDER
	};
	addSampler(pRenderer, &samplerClampDesc, &pLinearBorderSampler);

	const char* pStaticSamplerNames[] = { "g_LinearMirror", "g_LinearWrap" };
	Sampler* pStaticSamplers[] = { pLinearMirrorSampler, pLinearWrapSampler };

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc terrainNormalShader = {};
	terrainNormalShader.mStages[0] = { "GenNormalMap.vert", NULL, 0, NULL };
	terrainNormalShader.mStages[1] = { "GenNormalMap.frag", NULL, 0, NULL };
	addShader(pRenderer, &terrainNormalShader, &pGenTerrainNormalShader);

	RootSignatureDesc rootDesc = {};
	rootDesc.mShaderCount = 1;
	rootDesc.ppShaders = &pGenTerrainNormalShader;
	rootDesc.mStaticSamplerCount = 2;
	rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
	rootDesc.ppStaticSamplers = pStaticSamplers;
	addRootSignature(pRenderer, &rootDesc, &pGenTerrainNormalRootSignature);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc terrainShader = {};
	terrainShader.mStages[0] = { "Terrain.vert", NULL, 0, NULL};
	terrainShader.mStages[1] = { "Terrain.frag", NULL, 0, NULL};
	addShader(pRenderer, &terrainShader, &pTerrainShader);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc terrainRenderShader = {};
	terrainRenderShader.mStages[0] = { "RenderTerrain.vert", NULL, 0, NULL };
	terrainRenderShader.mStages[1] = { "RenderTerrain.frag", NULL, 0, NULL };
	addShader(pRenderer, &terrainRenderShader, &pRenderTerrainShader);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc terrainlightingShader = {};
	terrainlightingShader.mStages[0] = { "LightingTerrain.vert", NULL, 0, NULL };
	terrainlightingShader.mStages[1] = { "LightingTerrain.frag", NULL, 0, NULL };
	addShader(pRenderer, &terrainlightingShader, &pLightingTerrainShader);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Shader*           shaders[] = { pTerrainShader, pRenderTerrainShader, pLightingTerrainShader };
	rootDesc = {};
	rootDesc.mShaderCount = 3;
	rootDesc.ppShaders = shaders;
	rootDesc.mStaticSamplerCount = 2;
	rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
	rootDesc.ppStaticSamplers = pStaticSamplers;
	rootDesc.mMaxBindlessTextures = 5;
	addRootSignature(pRenderer, &rootDesc, &pTerrainRootSignature);

	SyncToken token = {};

#if USE_PROCEDUAL_TERRAIN
	for (int j = 0; j < GRID_SIZE - 1; j++)
	{
		int base = j * GRID_SIZE;

		for (int i = 0; i < GRID_SIZE - 1; i++)
		{
			int localbase = base + i;

			// Two triangles for one quad
			// First triangle
			indexBuffer.push_back((uint32_t)localbase);
			indexBuffer.push_back((uint32_t)(localbase + GRID_SIZE + 1));
			indexBuffer.push_back((uint32_t)(localbase + 1));

			// Second triangle
			indexBuffer.push_back((uint32_t)localbase);
			indexBuffer.push_back((uint32_t)(localbase + GRID_SIZE));
			indexBuffer.push_back((uint32_t)(localbase + GRID_SIZE + 1));
		}
	}

	BufferLoadDesc zoneIbDesc = {};
	zoneIbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
	zoneIbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	zoneIbDesc.mDesc.mSize = (uint64_t)(indexBuffer.size() * sizeof(float));
	zoneIbDesc.pData = indexBuffer.data();
	zoneIbDesc.ppBuffer = &pGlobalZoneIndexBuffer;
	addResource(&zoneIbDesc, &token);
#endif

	float screenQuadPoints[] = {
				-1.0f,  3.0f, 0.5f, 0.0f, -1.0f,
				-1.0f, -1.0f, 0.5f, 0.0f, 1.0f,
				3.0f, -1.0f, 0.5f, 2.0f, 1.0f,
	};

	BufferLoadDesc TriangularVbDesc = {};
	TriangularVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	TriangularVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	TriangularVbDesc.mDesc.mSize = (uint64_t)(sizeof(float) * 5 * 3);
	TriangularVbDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	TriangularVbDesc.pData = screenQuadPoints;
	TriangularVbDesc.ppBuffer = &pGlobalTriangularVertexBuffer;
	addResource(&TriangularVbDesc, &token);

	BufferLoadDesc LightingTerrainUnifromBufferDesc = {};
	LightingTerrainUnifromBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	LightingTerrainUnifromBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	LightingTerrainUnifromBufferDesc.mDesc.mSize = sizeof(LightingTerrainUniformBuffer);
	LightingTerrainUnifromBufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	LightingTerrainUnifromBufferDesc.pData = NULL;

	for (uint i = 0; i < gImageCount; i++)
	{
		LightingTerrainUnifromBufferDesc.ppBuffer = &pLightingTerrainUniformBuffer[i];
		addResource(&LightingTerrainUnifromBufferDesc, &token);
	}

	BufferLoadDesc RenderTerrainUnifromBufferDesc = {};
	RenderTerrainUnifromBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	RenderTerrainUnifromBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	RenderTerrainUnifromBufferDesc.mDesc.mSize = sizeof(RenderTerrainUniformBuffer);
	RenderTerrainUnifromBufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	RenderTerrainUnifromBufferDesc.pData = NULL;

	for (uint i = 0; i < gImageCount; i++)
	{
		RenderTerrainUnifromBufferDesc.ppBuffer = &pRenderTerrainUniformBuffer[i];
		addResource(&RenderTerrainUnifromBufferDesc, &token);
	}

	BufferLoadDesc VolumetricCloudsShadowUnifromBufferDesc = {};
	VolumetricCloudsShadowUnifromBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	VolumetricCloudsShadowUnifromBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	VolumetricCloudsShadowUnifromBufferDesc.mDesc.mSize = sizeof(VolumetricCloudsShadowCB);
	VolumetricCloudsShadowUnifromBufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	VolumetricCloudsShadowUnifromBufferDesc.pData = NULL;
	VolumetricCloudsShadowUnifromBufferDesc.ppBuffer = &pVolumetricCloudsShadowBuffer;
	addResource(&VolumetricCloudsShadowUnifromBufferDesc, &token);

	////////////////////////////////////////////////////////////////////////////////////////

	waitForToken(&token);

	GenerateTerrainFromHeightmap(HEIGHT_SCALE, EARTH_RADIUS);

	DescriptorSetDesc setDesc = { pGenTerrainNormalRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
	addDescriptorSet(pRenderer, &setDesc, &pGenTerrainNormalDescriptorSet);
	setDesc = { pTerrainRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 2 };
	addDescriptorSet(pRenderer, &setDesc, &pTerrainDescriptorSet[0]);
	setDesc = { pTerrainRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
	addDescriptorSet(pRenderer, &setDesc, &pTerrainDescriptorSet[1]);

	return true;
}


void Terrain::Exit()
{
	removeDescriptorSet(pRenderer, pTerrainDescriptorSet[0]);
	removeDescriptorSet(pRenderer, pTerrainDescriptorSet[1]);
	removeDescriptorSet(pRenderer, pGenTerrainNormalDescriptorSet);
	
	removeSampler(pRenderer, pLinearMirrorSampler);
	removeSampler(pRenderer, pLinearWrapSampler);
	removeSampler(pRenderer, pLinearBorderSampler);

	removeRootSignature(pRenderer, pTerrainRootSignature);
	removeRootSignature(pRenderer, pGenTerrainNormalRootSignature);

	removeShader(pRenderer, pGenTerrainNormalShader);
	removeShader(pRenderer, pTerrainShader);
	removeShader(pRenderer, pRenderTerrainShader);
	removeShader(pRenderer, pLightingTerrainShader);

#if	USE_PROCEDUAL_TERRAIN
	removeResource(pGlobalZoneIndexBuffer);

	for (ZoneMap::iterator iter = gZoneMap.begin(); iter != gZoneMap.end(); ++iter)
	{
		Zone* zone = iter->second;
		removeResource(zone->pZoneVertexBuffer);
		tf_free(zone);
	}

	gZoneMap.clear();
#endif

	removeResource(pGlobalVertexBuffer);
	removeResource(pGlobalTriangularVertexBuffer);

	for (uint i = 0; i < gImageCount; ++i)
	{
		removeResource(pLightingTerrainUniformBuffer[i]);
		removeResource(pRenderTerrainUniformBuffer[i]);
	}

	removeResource(pVolumetricCloudsShadowBuffer);

	for (size_t i = 0; i < meshSegments.size(); i++)
	{
		MeshSegment mesh = meshSegments[i];
		removeResource(mesh.indexBuffer);
	}

	removeResource(pTerrainNormalTexture);
	removeResource(pTerrainMaskTexture);

	for (uint32_t i = 0; i < gTerrainTextureCount; ++i)
	{
		removeResource(gTerrainTiledColorTexturesStorage[i]);
		removeResource(gTerrainTiledNormalTexturesStorage[i]);
	}
	
	tf_free(gTerrainTiledColorTexturesStorage);
	tf_free(gTerrainTiledNormalTexturesStorage);

	removeResource(pTerrainHeightMap);
	removeRenderTarget(pRenderer, pNormalMapFromHeightmapRT);

	meshSegments.set_capacity(0);
	meshSegments.clear();
}

void Terrain::GenerateTerrainFromHeightmap(float height, float radius)
{
	//float terrainConstructionTime = getCurrentTime();

	eastl::vector<TerrainVertex> vertices;
	meshSegments.clear();
	HeightData dataSource("Terrain/HeightMap.r32", NULL, height);
	HemisphereBuilder hemisphereBuilder;
#if _DEBUG
	hemisphereBuilder.build(pRenderer, &dataSource, vertices, meshSegments, radius * 10.0f - 720000.0f, 0.15f, 64, 15, 33);
#else
	hemisphereBuilder.build(pRenderer, &dataSource, vertices, meshSegments, radius * 10.0f - 720000.0f, 0.15f, 64, 15, 513);
#endif

	SyncToken token = {};

	//vertexBufferPositions = renderer->addVertexBuffer((uint32_t)vertices.size() * sizeof(Vertex), STATIC, &vertices.front());
	TerrainPathVertexCount = (uint32_t)vertices.size();
	BufferLoadDesc zoneVbDesc = {};
	zoneVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	zoneVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	zoneVbDesc.mDesc.mSize = (uint64_t)(sizeof(TerrainVertex) * vertices.size());
	zoneVbDesc.pData = vertices.data();
	zoneVbDesc.ppBuffer = &pGlobalVertexBuffer;
	addResource(&zoneVbDesc, &token);

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// normalMapTexture = renderer->addTexture(subPathTexture, true);

	TextureLoadDesc TerrainNormalTextureDesc = {};
	TerrainNormalTextureDesc.pFileName = "Terrain/Normalmap";
	TerrainNormalTextureDesc.ppTexture = &pTerrainNormalTexture;
	addResource(&TerrainNormalTextureDesc, &token);

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// maskTexture = renderer->addTexture(maskTextureFilePath, true);

	TextureLoadDesc TerrainMaskTextureDesc = {};
	TerrainMaskTextureDesc.pFileName = "Terrain/Mask";
	TerrainMaskTextureDesc.ppTexture = &pTerrainMaskTexture;
	addResource(&TerrainMaskTextureDesc, &token);

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	
	gTerrainTiledColorTexturesStorage = (Texture**)tf_malloc(sizeof(Texture*) * gTerrainTextureCount);
	gTerrainTiledNormalTexturesStorage = (Texture**)tf_malloc(sizeof(Texture*) * gTerrainTextureCount);

	for (uint32_t i = 0; i < gTerrainTextureCount; ++i)
	{
		TextureLoadDesc TerrainTiledColorTextureDesc = {};
		TerrainTiledColorTextureDesc.pFileName = TextureTileFilePaths[i];
		TerrainTiledColorTextureDesc.ppTexture = &gTerrainTiledColorTexturesStorage[i];
		addResource(&TerrainTiledColorTextureDesc, &token);

		TextureLoadDesc TerrainTiledNormalTextureDesc = {};
		TerrainTiledNormalTextureDesc.pFileName = TextureNormalTileFilePaths[i];
		TerrainTiledNormalTextureDesc.ppTexture = &gTerrainTiledNormalTexturesStorage[i];
		addResource(&TerrainTiledNormalTextureDesc, &token);
	}

	TextureLoadDesc TerrainHeightMapDesc = {};
	TerrainHeightMapDesc.pFileName = "Terrain/HeightMap_normgen";
	TerrainHeightMapDesc.ppTexture = &pTerrainHeightMap;
	addResource(&TerrainHeightMapDesc, &token);
	waitForToken(&token);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	RenderTargetDesc NormalMapFromHeightmapRenderTarget = {};
	NormalMapFromHeightmapRenderTarget.mArraySize = 1;
	NormalMapFromHeightmapRenderTarget.mDepth = 1;
	NormalMapFromHeightmapRenderTarget.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	NormalMapFromHeightmapRenderTarget.mFormat = TinyImageFormat_R16G16_SFLOAT;
	NormalMapFromHeightmapRenderTarget.mStartState = RESOURCE_STATE_RENDER_TARGET;
	NormalMapFromHeightmapRenderTarget.mSampleCount = SAMPLE_COUNT_1;
	NormalMapFromHeightmapRenderTarget.mSampleQuality = 0;
	//NormalMapFromHeightmapRenderTarget.mSrgb = false;
	NormalMapFromHeightmapRenderTarget.mWidth = pTerrainHeightMap->mWidth;
	NormalMapFromHeightmapRenderTarget.mHeight = pTerrainHeightMap->mHeight;
	NormalMapFromHeightmapRenderTarget.pName = "NormalMapFromHeightmap RenderTarget";
	addRenderTarget(pRenderer, &NormalMapFromHeightmapRenderTarget, &pNormalMapFromHeightmapRT);
	///////////////////////////////////////////////////////////////////////////////////////////////////////
}

bool Terrain::GenerateNormalMap(Cmd* cmd)
{
	cmdBindRenderTargets(cmd, 1, &pNormalMapFromHeightmapRT, NULL, NULL, NULL, NULL, -1, -1);
	cmdSetViewport(cmd, 0.0f, 0.0f, (float)pNormalMapFromHeightmapRT->mWidth, (float)pNormalMapFromHeightmapRT->mHeight, 0.0f, 1.0f);
	cmdSetScissor(cmd, 0, 0, pNormalMapFromHeightmapRT->mWidth, pNormalMapFromHeightmapRT->mHeight);
	cmdBindPipeline(cmd, pGenTerrainNormalPipeline);

	struct
	{
		float	heightScale;
	} cbTempRootConstantStruct;
	cbTempRootConstantStruct.heightScale = HEIGHT_SCALE;
	cmdBindPushConstants(cmd, pGenTerrainNormalRootSignature, "cbRootConstant", &cbTempRootConstantStruct);
	cmdBindDescriptorSet(cmd, 0, pGenTerrainNormalDescriptorSet);
	cmdDraw(cmd, 3, 0);

	cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);

	return true;
}

bool Terrain::Load(RenderTarget** rts, uint32_t count)
{
	return false;
}

bool Terrain::Load(int32_t width, int32_t height)
{
	mWidth = width;
	mHeight = height;

	float aspect = (float)mWidth / (float)mHeight;
	float aspectInverse = 1.0f / aspect;
	float horizontal_fov = PI / 3.0f;
	//float vertical_fov = 2.0f * atan(tan(horizontal_fov*0.5f) * aspectInverse);

	TerrainProjectionMatrix = mat4::perspective(horizontal_fov, aspectInverse, TERRAIN_NEAR, TERRAIN_FAR);

	RenderTargetDesc SceneRT = {};
	SceneRT.mArraySize = 1;
	SceneRT.mDepth = 1;
	SceneRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	SceneRT.mFormat = TinyImageFormat_R8G8B8A8_UNORM;
	SceneRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	SceneRT.mSampleCount = SAMPLE_COUNT_1;
	SceneRT.mSampleQuality = 0;

	SceneRT.mWidth = mWidth;
	SceneRT.mHeight = mHeight;

	addRenderTarget(pRenderer, &SceneRT, &pTerrainRT);

	RenderTargetDesc BasicColorRT = {};
	BasicColorRT.mArraySize = 1;
	BasicColorRT.mDepth = 1;
	BasicColorRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	BasicColorRT.mFormat = TinyImageFormat_R8G8B8A8_UNORM;
	BasicColorRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	BasicColorRT.mSampleCount = SAMPLE_COUNT_1;
	BasicColorRT.mSampleQuality = 0;
	BasicColorRT.mWidth = mWidth;
	BasicColorRT.mHeight = mHeight;

	addRenderTarget(pRenderer, &BasicColorRT, &pGBuffer_BasicRT);

	RenderTargetDesc NormalRT = {};
	NormalRT.mArraySize = 1;
	NormalRT.mDepth = 1;
	NormalRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	NormalRT.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
	NormalRT.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
	NormalRT.mSampleCount = SAMPLE_COUNT_1;
	NormalRT.mSampleQuality = 0;
	NormalRT.mWidth = mWidth;
	NormalRT.mHeight = mHeight;

	addRenderTarget(pRenderer, &NormalRT, &pGBuffer_NormalRT);

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

	DepthStateDesc depthStateDesc = {};
	depthStateDesc.mDepthTest = true;
	depthStateDesc.mDepthWrite = true;
	depthStateDesc.mDepthFunc = CMP_LEQUAL;

	RasterizerStateDesc rasterizerStateDesc = {};
	rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

	RasterizerStateDesc rasterizerStateFrontDesc = {};
	rasterizerStateFrontDesc.mCullMode = CULL_MODE_FRONT;

	RasterizerStateDesc rasterizerStateBackDesc = {};
	rasterizerStateBackDesc.mCullMode = CULL_MODE_BACK;

	PipelineDesc pipelineDescTerrain = {};
	pipelineDescTerrain.pCache = pPipelineCache;
	{
		pipelineDescTerrain.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescTerrain.mGraphicsDesc;

		pipelineSettings = {};
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = &depthStateDesc;
		pipelineSettings.pColorFormats = &pTerrainRT->mFormat;
		//pipelineSettings.pSrgbValues = &pTerrainRT->mSrgb;
		pipelineSettings.mSampleCount = pTerrainRT->mSampleCount;
		pipelineSettings.mSampleQuality = pTerrainRT->mSampleQuality;
		pipelineSettings.mDepthStencilFormat = pDepthBuffer->mFormat;
		pipelineSettings.pRootSignature = pTerrainRootSignature;
		pipelineSettings.pShaderProgram = pTerrainShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;

		addPipeline(pRenderer, &pipelineDescTerrain, &pTerrainPipeline);
	}

	TinyImageFormat colorFormats[2] = {};
	colorFormats[0] = pGBuffer_BasicRT->mFormat;
	colorFormats[1] = pGBuffer_NormalRT->mFormat;

	//bool sRGBs[2] = {};
	//sRGBs[0] = pGBuffer_BasicRT->mSrgb;
	//sRGBs[1] = pGBuffer_NormalRT->mSrgb;


	PipelineDesc pipelineDescRenderTerrain = {};
	pipelineDescRenderTerrain.pCache = pPipelineCache;
	{
		pipelineDescRenderTerrain.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescRenderTerrain.mGraphicsDesc;

		pipelineSettings = {};
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_STRIP;
		pipelineSettings.mRenderTargetCount = 2;
		pipelineSettings.pDepthState = &depthStateDesc;
		pipelineSettings.pColorFormats = colorFormats;
		//pipelineSettings.pSrgbValues = sRGBs;
		pipelineSettings.mSampleCount = pGBuffer_BasicRT->mSampleCount;
		pipelineSettings.mSampleQuality = pGBuffer_BasicRT->mSampleQuality;
		pipelineSettings.mDepthStencilFormat = pDepthBuffer->mFormat;
		pipelineSettings.pRootSignature = pTerrainRootSignature;
		pipelineSettings.pShaderProgram = pRenderTerrainShader;
		pipelineSettings.pVertexLayout = &vertexLayout;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;
		addPipeline(pRenderer, &pipelineDescRenderTerrain, &pRenderTerrainPipeline);
	}

	PipelineDesc pipelineDescGenTerrainNormal = {};
	pipelineDescGenTerrainNormal.pCache = pPipelineCache;
	{
		pipelineDescGenTerrainNormal.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescGenTerrainNormal.mGraphicsDesc;

		pipelineSettings = {};
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &pNormalMapFromHeightmapRT->mFormat;
		//pipelineSettings.pSrgbValues = &pNormalMapFromHeightmapRT->mSrgb;
		pipelineSettings.mSampleCount = pNormalMapFromHeightmapRT->mSampleCount;
		pipelineSettings.mSampleQuality = pNormalMapFromHeightmapRT->mSampleQuality;
		pipelineSettings.pRootSignature = pGenTerrainNormalRootSignature;
		pipelineSettings.pShaderProgram = pGenTerrainNormalShader;
		pipelineSettings.pRasterizerState = &rasterizerStateDesc;
		addPipeline(pRenderer, &pipelineDescGenTerrainNormal, &pGenTerrainNormalPipeline);;
	}

	PipelineDesc pipelineDescLightingTerrain = {};
	pipelineDescLightingTerrain.pCache = pPipelineCache;
	{
		pipelineDescLightingTerrain.mType = PIPELINE_TYPE_GRAPHICS;
		GraphicsPipelineDesc &pipelineSettings = pipelineDescLightingTerrain.mGraphicsDesc;

		pipelineSettings = {};
		pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
		pipelineSettings.mRenderTargetCount = 1;
		pipelineSettings.pDepthState = NULL;
		pipelineSettings.pColorFormats = &pTerrainRT->mFormat;
		//pipelineSettings.pSrgbValues = &pTerrainRT->mSrgb;
		pipelineSettings.mSampleCount = pTerrainRT->mSampleCount;
		pipelineSettings.mSampleQuality = pTerrainRT->mSampleQuality;

		//pipelineSettings.pRootSignature = pLightingTerrainRootSignature;
		pipelineSettings.pRootSignature = pTerrainRootSignature;
		pipelineSettings.pShaderProgram = pLightingTerrainShader;

		pipelineSettings.pRasterizerState = &rasterizerStateDesc;
		//addPipeline(pRenderer, &pipelineSettings, &pLightingTerrainPipeline);
		addPipeline(pRenderer, &pipelineDescLightingTerrain, &pLightingTerrainPipeline);
	}

	DescriptorData LtParams[1] = {};
	LtParams[0].pName = "Heightmap";
	LtParams[0].ppTextures = &pTerrainHeightMap;
	updateDescriptorSet(pRenderer, 0, pGenTerrainNormalDescriptorSet, 1, LtParams);

	// Terrain Render and Lighting
	{
		DescriptorData ScParams[4] = {};
		ScParams[0].pName = "tileTextures";
		ScParams[0].mCount = gTerrainTextureCount;
		ScParams[0].ppTextures = gTerrainTiledColorTexturesStorage;
		ScParams[1].pName = "tileTexturesNrm";
		ScParams[1].mCount = gTerrainTextureCount;
		ScParams[1].ppTextures = gTerrainTiledNormalTexturesStorage;
		ScParams[2].pName = "NormalMap";
		ScParams[2].ppTextures = &pTerrainNormalTexture;
		ScParams[3].pName = "MaskMap";
		ScParams[3].ppTextures = &pTerrainMaskTexture;
		updateDescriptorSet(pRenderer, 0, pTerrainDescriptorSet[0], 4, ScParams);

		DescriptorData LtParams[4] = {};
		LtParams[0].pName = "BasicTexture";
		LtParams[0].ppTextures = &pGBuffer_BasicRT->pTexture;
		LtParams[1].pName = "NormalTexture";
		LtParams[1].ppTextures = &pGBuffer_NormalRT->pTexture;
		LtParams[2].pName = "weatherTexture";
		LtParams[2].ppTextures = &pWeatherMap;
		LtParams[3].pName = "depthTexture";
		LtParams[3].ppTextures = &pDepthBuffer->pTexture;
		updateDescriptorSet(pRenderer, 1, pTerrainDescriptorSet[0], 4, LtParams);

		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			DescriptorData ScParams[3] = {};
			ScParams[0].pName = "RenderTerrainUniformBuffer";
			ScParams[0].ppBuffers = &pRenderTerrainUniformBuffer[i];
			ScParams[1].pName = "LightingTerrainUniformBuffer";
			ScParams[1].ppBuffers = &pLightingTerrainUniformBuffer[i];
			ScParams[2].pName = "VolumetricCloudsShadowCB";
			ScParams[2].ppBuffers = &pVolumetricCloudsShadowBuffer;
			updateDescriptorSet(pRenderer, i, pTerrainDescriptorSet[1], 3, ScParams);
		}
	}

	return true;
}

void Terrain::Unload()
{
	removePipeline(pRenderer, pTerrainPipeline);
	removePipeline(pRenderer, pGenTerrainNormalPipeline);
	removePipeline(pRenderer, pRenderTerrainPipeline);
	removePipeline(pRenderer, pLightingTerrainPipeline);

	removeRenderTarget(pRenderer, pTerrainRT);
	removeRenderTarget(pRenderer, pGBuffer_BasicRT);
	removeRenderTarget(pRenderer, pGBuffer_NormalRT);
}

void Terrain::Draw(Cmd* cmd)
{
	if (bFirstDraw)
	{
		GenerateNormalMap(cmd);
		bFirstDraw = false;
	}

	cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Terrain");

	{
		cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Geometry");

		RenderTarget* pRenderTarget = pTerrainRT;

#if USE_PROCEDUAL_TERRAIN
		RenderTargetBarrier barriers20[] = {
			{ pTerrainRT, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
			{ pDepthBuffer, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_DEPTH_WRITE }
		};
		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, barriers20);

		LoadActionsDesc loadActions = {};
		loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
		loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
		loadActions.mClearColorValues[0].r = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].b = 0.0f;
		loadActions.mClearColorValues[0].a = 0.0f;
		loadActions.mClearDepth = pDepthBuffer->mClearValue;

		cmdBindRenderTargets(cmd, 1, &pRenderTarget, pDepthBuffer, &loadActions, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

		cmdBindPipeline(cmd, pTerrainPipeline);

		struct UniformData
		{
			mat4	viewProjMat;
		}cbRootConstantStruct;

		const float aspect = (float)mWidth / (float)mHeight;
		const float aspectInverse = 1.0f / aspect;
		const float horizontal_fov = PI / 3.0f;
		const float vertical_fov = 2.0f * atan(tan(horizontal_fov*0.5f) * aspectInverse);
		mat4 projMat = mat4::perspective(horizontal_fov, aspectInverse, 0.1f, 10000.0f);

		cbRootConstantStruct.viewProjMat = projMat * pCameraController->getViewMatrix();

		BufferUpdateDesc updateDesc = { pRenderTerrainUniformBuffer[gFrameIndex] };
		beginUpdateResource(&updateDesc);
		*(UniformData*)updateDesc.pMappedData = cbRootConstantStruct;
		endUpdateResource(&updateDesc, NULL);

		cmdBindDescriptorSet(cmd, gFrameIndex, pTerrainDescriptorSet[1]);

		for (ZoneMap::iterator iter = gZoneMap.begin(); iter != gZoneMap.end(); ++iter)
		{
			Zone* zone = iter->second;

			if (zone->visible)
			{
				const uint32_t stride = sizeof(float) * 5;
				cmdBindVertexBuffer(cmd, 1, &zone->pZoneVertexBuffer, &stride, NULL);
				cmdBindIndexBuffer(cmd, zone->pZoneIndexBuffer, INDEX_TYPE_UINT32, 0);
				cmdDrawIndexed(cmd, (uint32_t)indexBuffer.size(), 0, 0);
			}
		}

		barriers20[0] = { pDepthBuffer, RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_SHADER_RESOURCE };
		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers20);
#else

		/*
		if (!shadowPass)
		{
			TextureID rts[2] = { baseRT, normalRT };
			renderer->changeRenderTargets(rts, 2, depthRT);
			renderer->changeViewport(0, 0, width, height);
			renderer->clear(true, true);
		}
		*/

		LoadActionsDesc loadActions = {};
		loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
		loadActions.mLoadActionsColor[1] = LOAD_ACTION_CLEAR;

		loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;

		loadActions.mClearColorValues[0].r = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].b = 0.0f;
		loadActions.mClearColorValues[0].a = 0.0f;

		loadActions.mClearColorValues[1].r = 0.0f;
		loadActions.mClearColorValues[1].g = 0.0f;
		loadActions.mClearColorValues[1].b = 0.0f;
		loadActions.mClearColorValues[1].a = 0.0f;
		loadActions.mClearDepth = pDepthBuffer->mClearValue;

		getFrustumFromMatrix(TerrainProjectionMatrix * pCameraController->getViewMatrix(), terrainFrustum);

		RenderTargetBarrier barriersTerrainLighting[] = {
			{ pGBuffer_BasicRT, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
			{ pGBuffer_NormalRT, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
			{ pDepthBuffer, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_DEPTH_WRITE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 3, barriersTerrainLighting);

		RenderTarget* RTs[2] = { pGBuffer_BasicRT, pGBuffer_NormalRT };

		cmdBindRenderTargets(cmd, sizeof(RTs) / sizeof(RTs[0]), RTs, pDepthBuffer, &loadActions, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pGBuffer_BasicRT->mWidth, (float)pGBuffer_BasicRT->mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pGBuffer_BasicRT->mWidth, pGBuffer_BasicRT->mHeight);

		cmdBindPipeline(cmd, pRenderTerrainPipeline);

		{
			BufferUpdateDesc BufferUpdateDescDesc = { pRenderTerrainUniformBuffer[gFrameIndex] };
			beginUpdateResource(&BufferUpdateDescDesc);
			RenderTerrainUniformBuffer& _RenderTerrainUniformBuffer = *(RenderTerrainUniformBuffer*)BufferUpdateDescDesc.pMappedData;
			_RenderTerrainUniformBuffer.projView = TerrainProjectionMatrix * pCameraController->getViewMatrix();
			_RenderTerrainUniformBuffer.TerrainInfo = float4(EARTH_RADIUS, 1.0f, HEIGHT_SCALE, 0.0f);
			_RenderTerrainUniformBuffer.CameraInfo = float4(TERRAIN_NEAR, TERRAIN_FAR, TERRAIN_NEAR, TERRAIN_FAR);
			endUpdateResource(&BufferUpdateDescDesc, NULL);
		}

		const uint32_t terrainStride = sizeof(TerrainVertex);
		cmdBindVertexBuffer(cmd, 1, &pGlobalVertexBuffer, &terrainStride, NULL);
		cmdBindDescriptorSet(cmd, 0, pTerrainDescriptorSet[0]);
		cmdBindDescriptorSet(cmd, gFrameIndex, pTerrainDescriptorSet[1]);

		// render depth image
		for (auto& mesh : meshSegments)
		{
			if (!boxIntersects(terrainFrustum, mesh.boundingBox) /*&& !shadowPass*/)
				continue;

			cmdBindIndexBuffer(cmd, mesh.indexBuffer, INDEX_TYPE_UINT32, 0);


			cmdDrawIndexed(cmd, (uint32_t)mesh.indexCount, 0, 0);

		}

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

		cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Apply Lighting");

		RenderTargetBarrier barriersLighting[] = {
			{ pTerrainRT, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
			{ pGBuffer_BasicRT, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE },
			{ pGBuffer_NormalRT, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE },
			{ pDepthBuffer, RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_SHADER_RESOURCE },
		};

		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 4, barriersLighting);

		loadActions.mLoadActionDepth = LOAD_ACTION_DONTCARE;

		cmdBindRenderTargets(cmd, 1, &pTerrainRT, NULL, &loadActions, NULL, NULL, -1, -1);
		cmdSetViewport(cmd, 0.0f, 0.0f, (float)pTerrainRT->mWidth, (float)pTerrainRT->mHeight, 0.0f, 1.0f);
		cmdSetScissor(cmd, 0, 0, pTerrainRT->mWidth, pTerrainRT->mHeight);

		cmdBindPipeline(cmd, pLightingTerrainPipeline);

		{
			BufferUpdateDesc BufferUniformSettingDesc = { pLightingTerrainUniformBuffer[gFrameIndex] };
			beginUpdateResource(&BufferUniformSettingDesc);
			LightingTerrainUniformBuffer& cbTempRootConstantStruct = *(LightingTerrainUniformBuffer*)BufferUniformSettingDesc.pMappedData;

			cbTempRootConstantStruct.InvViewProjMat = inverse(pCameraController->getViewMatrix()) * inverse(TerrainProjectionMatrix);
			cbTempRootConstantStruct.ShadowViewProjMat = mat4::identity();
			cbTempRootConstantStruct.ShadowSpheres = float4(1.0f, 1.0f, 1.0f, 1.0f);
			cbTempRootConstantStruct.SunColor = float4(SunColor, 1.0f);
			cbTempRootConstantStruct.LightColor = LightColorAndIntensity;

			vec4 lightDir = vec4(f3Tov3(LightDirection));

			cbTempRootConstantStruct.LightDirection = v4ToF4(lightDir);
			endUpdateResource(&BufferUniformSettingDesc, NULL);

			BufferUpdateDesc ShadowBufferUniformSettingDesc = { pVolumetricCloudsShadowBuffer };
			beginUpdateResource(&ShadowBufferUniformSettingDesc);
			*(VolumetricCloudsShadowCB*)ShadowBufferUniformSettingDesc.pMappedData = volumetricCloudsShadowCB;
			endUpdateResource(&ShadowBufferUniformSettingDesc, NULL);
		}

		// Deferred ambient pass
		cmdBindDescriptorSet(cmd, 1, pTerrainDescriptorSet[0]);
		cmdBindDescriptorSet(cmd, gFrameIndex, pTerrainDescriptorSet[1]);
		cmdDraw(cmd, 3, 0);

		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

#endif
		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);

		RenderTargetBarrier barriers21[] = {
			{ pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers21);
	}

	cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
}

void Terrain::Update(float deltaTime)
{
	if (volumetricCloudsShadowCB.ShadowInfo.getZ() != 0.0f)
	{
		vec4 WindInfo = volumetricCloudsShadowCB.StandardPosition;

		volumetricCloudsShadowCB.SettingInfo00 = vec4(volumetricCloudsShadowCB.SettingInfo00.getX(), volumetricCloudsShadowCB.SettingInfo00.getY(), volumetricCloudsShadowCB.SettingInfo00.getZ() / volumetricCloudsShadowCB.ShadowInfo.getZ(), 0.0f);
		float DistanceWithCurrentSpeed = WindInfo.getW() * deltaTime * 100.0f;

		g_StandardPosition += vec4(WindInfo.getX(), 0.0, WindInfo.getZ(), 0.0) * (DistanceWithCurrentSpeed * volumetricCloudsShadowCB.ShadowInfo.getY() / volumetricCloudsShadowCB.ShadowInfo.getZ());
		g_StandardPosition.setW(volumetricCloudsShadowCB.ShadowInfo.getX());
		volumetricCloudsShadowCB.StandardPosition = g_StandardPosition;
	}


#if USE_PROCEDUAL_TERRAIN
	for (ZoneMap::iterator iter = gZoneMap.begin(); iter != gZoneMap.end(); ++iter)
	{
		Zone* zone = iter->second;
		zone->visible = false;
	}

	vec3 cameraPos = pCameraController->getViewPosition();

	uint32_t MinX = (uint32_t)(floor((cameraPos.getX() / (GRID_SIZE * OVERALLSCALE)) - 0.5f) + TILE_CENTER);
	uint32_t MaxX = (uint32_t)(floor((cameraPos.getX() / (GRID_SIZE * OVERALLSCALE)) + 0.5f) + TILE_CENTER);

	uint32_t MinZ = (uint32_t)(floor((cameraPos.getZ() / (GRID_SIZE * OVERALLSCALE)) - 0.5f) + TILE_CENTER);
	uint32_t MaxZ = (uint32_t)(floor((cameraPos.getZ() / (GRID_SIZE * OVERALLSCALE)) + 0.5f) + TILE_CENTER);

	for (uint32_t Z = MinZ; Z <= MaxZ; Z++)
	{
		for (uint32_t X = MinX; X <= MaxX; X++)
		{

			uint32_t z = Z;
			uint32_t x = X;
			while (z > 0)
			{
				z /= 10;
				x *= 10;
			}

			eastl::unordered_map<uint32_t, Zone*>::iterator iterator = gZoneMap.find(x + Z);

			// if there is no zone
			if (iterator == gZoneMap.end())
			{
				// create it
				Zone* newZone = tf_placement_new<Zone>(tf_calloc(1, sizeof(Zone)));

				int32_t XInit = (X - TILE_CENTER) * GRID_SIZE;
				int32_t ZInit = (Z - TILE_CENTER) * GRID_SIZE;

				float gap = (float)GRID_SIZE / (float)(GRID_SIZE - 1);

				float i = (float)XInit;

				for (int X_index = 0; X_index < GRID_SIZE; X_index++)
				{
					float j = (float)ZInit;

					for (int Z_index = 0; Z_index < GRID_SIZE; Z_index++)
					{
						newZone->PosAndUVs.push_back((float)i * OVERALLSCALE);
						newZone->PosAndUVs.push_back(noiseGenerator.perlinNoise2D((float)(i + (float)TILE_CENTER) * 0.125f, (float)(j + (float)TILE_CENTER) * 0.125f) * OVERALLSCALE);
						newZone->PosAndUVs.push_back((float)j * OVERALLSCALE);

						newZone->PosAndUVs.push_back((float)(j - ZInit) / GRID_SIZE);
						newZone->PosAndUVs.push_back((float)(i - XInit) / GRID_SIZE);

						j += gap;
					}

					i += gap;
				}

				newZone->X = X;
				newZone->Z = Z;
				newZone->visible = true;

				uint64_t zoneDataSize = GRID_SIZE * GRID_SIZE * sizeof(float) * 5;
				BufferLoadDesc zoneVbDesc = {};
				zoneVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
				zoneVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
				zoneVbDesc.mDesc.mSize = zoneDataSize;
				zoneVbDesc.pData = newZone->PosAndUVs.data();
				zoneVbDesc.ppBuffer = &newZone->pZoneVertexBuffer;
				addResource(&zoneVbDesc, NULL);

				newZone->pZoneIndexBuffer = pGlobalZoneIndexBuffer;

				gZoneMap.insert(eastl::pair<uint32_t, Zone*>(newZone->GetKey(), newZone));
			}
			else
			{
				iterator->second->visible = true;
			}
		}
	}
#endif
}

void Terrain::InitializeWithLoad(RenderTarget* InDepthRenderTarget)
{
	pDepthBuffer = InDepthRenderTarget;
}

void Terrain::Initialize(uint InImageCount,
	ICameraController* InCameraController, Queue*	InGraphicsQueue, CmdPool* InTransCmdPool,
	Cmd** InTransCmds, Fence* InTransitionCompleteFences, ProfileToken InGraphicsGpuProfiler)
{
	gImageCount = InImageCount;

	pCameraController = InCameraController;
	pGraphicsQueue = InGraphicsQueue;
	pTransCmdPool = InTransCmdPool;
	ppTransCmds = InTransCmds;
	pTransitionCompleteFences = InTransitionCompleteFences;
	gGpuProfileToken = InGraphicsGpuProfiler;
}
