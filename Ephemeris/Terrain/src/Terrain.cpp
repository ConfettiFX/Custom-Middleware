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

#if defined(XBOX) || defined(ORBIS) || defined(PROSPERO)
char TextureTileFilePaths[5][255] = {
  "Textures/Tiles/grass_DM",
  "Textures/Tiles/cliff_DM",
  "Textures/Tiles/snow_DM",
  "Textures/Tiles/grassDark_DM",
  "Textures/Tiles/gravel_DM" };

char TextureNormalTileFilePaths[5][255] = {
  "Textures/Tiles/grass_NM",
  "Textures/Tiles/cliff_NM",
  "Textures/Tiles/Snow_NM",
  "Textures/Tiles/grass_NM",
  "Textures/Tiles/gravel_NM" };
#else
char TextureTileFilePaths[5][255] = {
  "../../../Ephemeris/Terrain/resources/Textures/Tiles/grass_DM",
  "../../../Ephemeris/Terrain/resources/Textures/Tiles/cliff_DM",
  "../../../Ephemeris/Terrain/resources/Textures/Tiles/snow_DM",
  "../../../Ephemeris/Terrain/resources/Textures/Tiles/grassDark_DM",
  "../../../Ephemeris/Terrain/resources/Textures/Tiles/gravel_DM" };

char TextureNormalTileFilePaths[5][255] = {
  "../../../Ephemeris/Terrain/resources/Textures/Tiles/grass_NM",
  "../../../Ephemeris/Terrain/resources/Textures/Tiles/cliff_NM",
  "../../../Ephemeris/Terrain/resources/Textures/Tiles/Snow_NM",
  "../../../Ephemeris/Terrain/resources/Textures/Tiles/grass_NM",
  "../../../Ephemeris/Terrain/resources/Textures/Tiles/gravel_NM" };
#endif

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
	frustum.iPlanes.left.distance = matrix[3][3] + matrix[3][0];

	// Right clipping plane 
	frustum.iPlanes.right.normal.x = matrix[0][3] - matrix[0][0];
	frustum.iPlanes.right.normal.y = matrix[1][3] - matrix[1][0];
	frustum.iPlanes.right.normal.z = matrix[2][3] - matrix[2][0];
	frustum.iPlanes.right.distance = matrix[3][3] - matrix[3][0];

	// Top clipping plane 
	frustum.iPlanes.top.normal.x = matrix[0][3] - matrix[0][1];
	frustum.iPlanes.top.normal.y = matrix[1][3] - matrix[1][1];
	frustum.iPlanes.top.normal.z = matrix[2][3] - matrix[2][1];
	frustum.iPlanes.top.distance = matrix[3][3] - matrix[3][1];

	// Bottom clipping plane 
	frustum.iPlanes.bottom.normal.x = matrix[0][3] + matrix[0][1];
	frustum.iPlanes.bottom.normal.y = matrix[1][3] + matrix[1][1];
	frustum.iPlanes.bottom.normal.z = matrix[2][3] + matrix[2][1];
	frustum.iPlanes.bottom.distance = matrix[3][3] + matrix[3][1];

	// Near clipping plane 
	frustum.iPlanes.front.normal.x = matrix[0][2];
	frustum.iPlanes.front.normal.y = matrix[1][2];
	frustum.iPlanes.front.normal.z = matrix[2][2];
	frustum.iPlanes.front.distance = matrix[3][2];

	// Far clipping plane 
	frustum.iPlanes.back.normal.x = matrix[0][3] - matrix[0][2];
	frustum.iPlanes.back.normal.y = matrix[1][3] - matrix[1][2];
	frustum.iPlanes.back.normal.z = matrix[2][3] - matrix[2][2];
	frustum.iPlanes.back.distance = matrix[3][3] - matrix[3][2];
}

#if defined(VULKAN)
static void TransitionRenderTargets(RenderTarget *pRT, ResourceState state, Renderer* renderer, CmdPool* cmdPool, Cmd* cmd, Queue* queue, Fence* fence)
{
	// Transition render targets to desired state
	beginCmd(cmd);

	RenderTargetBarrier barrier[] = {
		   { pRT, state }
	};

	cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barrier);
	endCmd(cmd);

	QueueSubmitDesc submitDesc = {};
	submitDesc.mCmdCount = 1;
	submitDesc.ppCmds = &cmd;
	submitDesc.pSignalFence = fence;
	queueSubmit(queue, &submitDesc);
	waitForFences(renderer, 1, &fence);

	resetCmdPool(renderer, cmdPool);
}
#endif

static void ShaderPath(const eastl::string &shaderPath, char* pShaderName, eastl::string &result)
{
	result.resize(0);
	eastl::string shaderName(pShaderName);
	result = shaderPath + shaderName;
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

#if defined(XBOX) || defined(ORBIS) || defined(PROSPERO)
	eastl::string shaderPath("");
#elif defined(DIRECT3D12)
	eastl::string shaderPath("../../../../../Ephemeris/Terrain/resources/Shaders/D3D12/");
#elif defined(VULKAN)
	eastl::string shaderPath("../../../../../Ephemeris/Terrain/resources/Shaders/Vulkan/");
#elif defined(METAL)
	eastl::string shaderPath("../../../../../Ephemeris/Terrain/resources/Shaders/Metal/");
#endif
	const char* pStaticSamplerNames[] = { "g_LinearMirror", "g_LinearWrap" };
	Sampler* pStaticSamplers[] = { pLinearMirrorSampler, pLinearWrapSampler };

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc terrainNormalShader = {};
	eastl::string terrainNormalShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"GenNormalMap.vert", terrainNormalShaderFullPath[0]);
	terrainNormalShader.mStages[0] = { terrainNormalShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"GenNormalMap.frag", terrainNormalShaderFullPath[1]);
	terrainNormalShader.mStages[1] = { terrainNormalShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };
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
	eastl::string terrainShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"terrain.vert", terrainShaderFullPath[0]);
	terrainShader.mStages[0] = { terrainShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"terrain.frag", terrainShaderFullPath[1]);
	terrainShader.mStages[1] = { terrainShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &terrainShader, &pTerrainShader);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc terrainRenderShader = {};
	eastl::string terrainRenderShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"RenderTerrain.vert", terrainRenderShaderFullPath[0]);
	terrainRenderShader.mStages[0] = { terrainRenderShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"RenderTerrain.frag", terrainRenderShaderFullPath[1]);
	terrainRenderShader.mStages[1] = { terrainRenderShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

	addShader(pRenderer, &terrainRenderShader, &pRenderTerrainShader);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	ShaderLoadDesc terrainlightingShader = {};
	eastl::string terrainlightingShaderFullPath[2];
	ShaderPath(shaderPath, (char*)"LightingTerrain.vert", terrainlightingShaderFullPath[0]);
	terrainlightingShader.mStages[0] = { terrainlightingShaderFullPath[0].c_str(), NULL, 0, RD_SHADER_SOURCES };
	ShaderPath(shaderPath, (char*)"LightingTerrain.frag", terrainlightingShaderFullPath[1]);
	terrainlightingShader.mStages[1] = { terrainlightingShaderFullPath[1].c_str(), NULL, 0, RD_SHADER_SOURCES };

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
	zoneIbDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_OWN_MEMORY_BIT | BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	zoneIbDesc.mDesc.mSize = (uint64_t)(indexBuffer.size() * sizeof(float));
	zoneIbDesc.mDesc.mIndexType = INDEX_TYPE_UINT32;
	zoneIbDesc.pData = indexBuffer.data();
	zoneIbDesc.ppBuffer = &pGlobalZoneIndexBuffer;
	addResource(&zoneIbDesc, &token, LOAD_PRIORITY_NORMAL);
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
	TriangularVbDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_OWN_MEMORY_BIT | BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	TriangularVbDesc.pData = screenQuadPoints;
	TriangularVbDesc.ppBuffer = &pGlobalTriangularVertexBuffer;
	addResource(&TriangularVbDesc, &token, LOAD_PRIORITY_NORMAL);

	BufferLoadDesc LightingTerrainUnifromBufferDesc = {};
	LightingTerrainUnifromBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	LightingTerrainUnifromBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	LightingTerrainUnifromBufferDesc.mDesc.mSize = sizeof(LightingTerrainUniformBuffer);
	LightingTerrainUnifromBufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	LightingTerrainUnifromBufferDesc.pData = NULL;

	for (uint i = 0; i < gImageCount; i++)
	{
		LightingTerrainUnifromBufferDesc.ppBuffer = &pLightingTerrainUniformBuffer[i];
		addResource(&LightingTerrainUnifromBufferDesc, &token, LOAD_PRIORITY_NORMAL);
	}

	BufferLoadDesc RenderTerrainUnifromBufferDesc = {};
	RenderTerrainUnifromBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	RenderTerrainUnifromBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	RenderTerrainUnifromBufferDesc.mDesc.mSize = sizeof(RenderTerrainUniformBuffer);
	RenderTerrainUnifromBufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	RenderTerrainUnifromBufferDesc.pData = NULL;

	for (uint i = 0; i < gImageCount; i++)
	{
		RenderTerrainUnifromBufferDesc.ppBuffer = &pRenderTerrainUniformBuffer[i];
		addResource(&RenderTerrainUnifromBufferDesc, &token, LOAD_PRIORITY_NORMAL);
	}

	BufferLoadDesc VolumetricCloudsShadowUnifromBufferDesc = {};
	VolumetricCloudsShadowUnifromBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	VolumetricCloudsShadowUnifromBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	VolumetricCloudsShadowUnifromBufferDesc.mDesc.mSize = sizeof(VolumetricCloudsShadowCB);
	VolumetricCloudsShadowUnifromBufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT | BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
	VolumetricCloudsShadowUnifromBufferDesc.pData = NULL;
	VolumetricCloudsShadowUnifromBufferDesc.ppBuffer = &pVolumetricCloudsShadowBuffer;
	addResource(&VolumetricCloudsShadowUnifromBufferDesc, &token, LOAD_PRIORITY_NORMAL);

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
	removeSampler(pRenderer, pLinearMirrorSampler);
	removeSampler(pRenderer, pLinearWrapSampler);
	removeSampler(pRenderer, pLinearBorderSampler);

	removeShader(pRenderer, pTerrainShader);
	removeRootSignature(pRenderer, pTerrainRootSignature);
	removeDescriptorSet(pRenderer, pTerrainDescriptorSet[0]);
	removeDescriptorSet(pRenderer, pTerrainDescriptorSet[1]);
	removeDescriptorSet(pRenderer, pGenTerrainNormalDescriptorSet);

	removeShader(pRenderer, pGenTerrainNormalShader);
	removeRootSignature(pRenderer, pGenTerrainNormalRootSignature);
	removeShader(pRenderer, pRenderTerrainShader);
	removeShader(pRenderer, pLightingTerrainShader);

#if	USE_PROCEDUAL_TERRAIN
	removeResource(pGlobalZoneIndexBuffer);

	for (ZoneMap::iterator iter = gZoneMap.begin(); iter != gZoneMap.end(); ++iter)
	{
		Zone* zone = iter->second;
		removeResource(zone->pZoneVertexBuffer);
		conf_free(zone);
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

	conf_free(gTerrainTiledColorTexturesStorage);
	conf_free(gTerrainTiledNormalTexturesStorage);

	for (int i = 0; i < 5; ++i)
	{
		removeResource(pTerrainTiledColorTextures[i]);
		removeResource(pTerrainTiledNormalTextures[i]);
	}

	removeResource(pTerrainHeightMap);
	removeRenderTarget(pRenderer, pNormalMapFromHeightmapRT);

	meshSegments.set_capacity(0);
	meshSegments.clear();

	pTerrainTiledColorTextures.set_capacity(0);
	pTerrainTiledColorTextures.clear();
	pTerrainTiledNormalTextures.set_capacity(0);
	pTerrainTiledNormalTextures.clear();

	gTerrainTiledColorTexturesPacked.set_capacity(0);
	gTerrainTiledColorTexturesPacked.clear();
	gTerrainTiledNormalTexturesPacked.set_capacity(0);
	gTerrainTiledNormalTexturesPacked.clear();
}

void Terrain::GenerateTerrainFromHeightmap(float height, float radius)
{
	//float terrainConstructionTime = getCurrentTime();

	eastl::vector<TerrainVertex> vertices;
	meshSegments.clear();
#if defined(XBOX) || defined(ORBIS) || defined(PROSPERO)
	HeightData dataSource("Textures/HeightMap.r32", height);
#else
	HeightData dataSource("../../../Ephemeris/Terrain/resources/Textures/HeightMap.r32", height);
#endif
	HemisphereBuilder hemisphereBuilder;
#if _DEBUG
	hemisphereBuilder.build(pRenderer, &dataSource, vertices, meshSegments, radius * 10.0f - 720000.0f, 0.15f, 64, 15, 33);
#else
	hemisphereBuilder.build(pRenderer, &dataSource, vertices, meshSegments, radius * 10.0f - 720000.0f, 0.15f, 64, 15, 513);
#endif

	SyncToken token = {};

	//vertexBufferPositions = renderer->addVertexBuffer((uint32)vertices.size() * sizeof(Vertex), STATIC, &vertices.front());
	TerrainPathVertexCount = (uint32_t)vertices.size();
	BufferLoadDesc zoneVbDesc = {};
	zoneVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	zoneVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	zoneVbDesc.mDesc.mSize = (uint64_t)(sizeof(TerrainVertex) * vertices.size());
	zoneVbDesc.pData = vertices.data();
	zoneVbDesc.ppBuffer = &pGlobalVertexBuffer;
	addResource(&zoneVbDesc, &token, LOAD_PRIORITY_NORMAL);

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// normalMapTexture = renderer->addTexture(subPathTexture, true);

	TextureLoadDesc TerrainNormalTextureDesc = {};
#if defined(XBOX) || defined(ORBIS) || defined(PROSPERO)
	PathHandle TerrainNormalTextureFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, "Textures/Normalmap");
#else
	PathHandle TerrainNormalTextureFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, "../../../Ephemeris/Terrain/resources/Textures/Normalmap");
#endif
	TerrainNormalTextureDesc.pFilePath = TerrainNormalTextureFilePath;
	TerrainNormalTextureDesc.ppTexture = &pTerrainNormalTexture;
	addResource(&TerrainNormalTextureDesc, &token, LOAD_PRIORITY_NORMAL);

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// maskTexture = renderer->addTexture(maskTextureFilePath, true);

	TextureLoadDesc TerrainMaskTextureDesc = {};
#if defined(XBOX) || defined(ORBIS) || defined(PROSPERO)
	PathHandle TerrainMaskTextureFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, "Textures/Mask");
#else
	PathHandle TerrainMaskTextureFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, "../../../Ephemeris/Terrain/resources/Textures/Mask");
#endif
	TerrainMaskTextureDesc.pFilePath = TerrainMaskTextureFilePath;
	TerrainMaskTextureDesc.ppTexture = &pTerrainMaskTexture;
	addResource(&TerrainMaskTextureDesc, &token, LOAD_PRIORITY_NORMAL);

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	pTerrainTiledColorTextures.resize(5);
	pTerrainTiledNormalTextures.resize(5);

	for (int i = 0; i < 5; ++i)
	{
		TextureLoadDesc TerrainTiledColorTextureDesc = {};

		PathHandle TerrainTiledColorTextureFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, TextureTileFilePaths[i]);

		TerrainTiledColorTextureDesc.pFilePath = TerrainTiledColorTextureFilePath;
		TerrainTiledColorTextureDesc.ppTexture = &pTerrainTiledColorTextures[i];
		addResource(&TerrainTiledColorTextureDesc, &token, LOAD_PRIORITY_NORMAL);

		TextureLoadDesc TerrainTiledNormalTextureDesc = {};
		
		PathHandle TerrainTiledNormalTextureFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, TextureNormalTileFilePaths[i]);

		TerrainTiledNormalTextureDesc.pFilePath = TerrainTiledNormalTextureFilePath;
		TerrainTiledNormalTextureDesc.ppTexture = &pTerrainTiledNormalTextures[i];
		addResource(&TerrainTiledNormalTextureDesc, &token, LOAD_PRIORITY_NORMAL);
	}

	TextureLoadDesc TerrainHeightMapDesc = {};
#if defined(XBOX) || defined(ORBIS) || defined(PROSPERO)
	PathHandle TerrainHeightMapFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, "Textures/HeightMap_normgen");
#else
	PathHandle TerrainHeightMapFilePath = fsGetPathInResourceDirEnum(RD_OTHER_FILES, "../../../Ephemeris/Terrain/resources/Textures/HeightMap_normgen");
#endif
	TerrainHeightMapDesc.pFilePath = TerrainHeightMapFilePath;
	TerrainHeightMapDesc.ppTexture = &pTerrainHeightMap;
	addResource(&TerrainHeightMapDesc, &token, LOAD_PRIORITY_HIGH);
	waitForToken(&token);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	RenderTargetDesc NormalMapFromHeightmapRenderTarget = {};
	NormalMapFromHeightmapRenderTarget.mArraySize = 1;
	NormalMapFromHeightmapRenderTarget.mDepth = 1;
	NormalMapFromHeightmapRenderTarget.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	NormalMapFromHeightmapRenderTarget.mFormat = TinyImageFormat_R16G16_SFLOAT;
	NormalMapFromHeightmapRenderTarget.mSampleCount = SAMPLE_COUNT_1;
	NormalMapFromHeightmapRenderTarget.mSampleQuality = 0;
	//NormalMapFromHeightmapRenderTarget.mSrgb = false;
	NormalMapFromHeightmapRenderTarget.mWidth = pTerrainHeightMap->mWidth;
	NormalMapFromHeightmapRenderTarget.mHeight = pTerrainHeightMap->mHeight;
	NormalMapFromHeightmapRenderTarget.pName = "NormalMapFromHeightmap RenderTarget";
	addRenderTarget(pRenderer, &NormalMapFromHeightmapRenderTarget, &pNormalMapFromHeightmapRT);

#if defined(VULKAN)
	TransitionRenderTargets(pNormalMapFromHeightmapRT, ResourceState::RESOURCE_STATE_RENDER_TARGET, pRenderer, pTransCmdPool, ppTransCmds[0], pGraphicsQueue, pTransitionCompleteFences);
#endif

	gTerrainTiledColorTexturesStorage = (Texture*)conf_malloc(sizeof(Texture) * pTerrainTiledColorTextures.size());
	gTerrainTiledNormalTexturesStorage = (Texture*)conf_malloc(sizeof(Texture) * pTerrainTiledNormalTextures.size());

	for (uint32_t i = 0; i < (uint32_t)pTerrainTiledColorTextures.size(); ++i)
	{
		memcpy(&gTerrainTiledColorTexturesStorage[i], pTerrainTiledColorTextures[i], sizeof(Texture));
		gTerrainTiledColorTexturesPacked.push_back(&gTerrainTiledColorTexturesStorage[i]);
	}

	for (uint32_t i = 0; i < (uint32_t)pTerrainTiledNormalTextures.size(); ++i)
	{
		memcpy(&gTerrainTiledNormalTexturesStorage[i], pTerrainTiledNormalTextures[i], sizeof(Texture));
		gTerrainTiledNormalTexturesPacked.push_back(&gTerrainTiledNormalTexturesStorage[i]);
	}

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
	SceneRT.mSampleCount = SAMPLE_COUNT_1;
	SceneRT.mSampleQuality = 0;

	SceneRT.mWidth = mWidth;
	SceneRT.mHeight = mHeight;

	addRenderTarget(pRenderer, &SceneRT, &pTerrainRT);

#if defined(VULKAN)
	TransitionRenderTargets(pTerrainRT, ResourceState::RESOURCE_STATE_RENDER_TARGET, pRenderer, pTransCmdPool, ppTransCmds[0], pGraphicsQueue, pTransitionCompleteFences);
#endif

	RenderTargetDesc BasicColorRT = {};
	BasicColorRT.mArraySize = 1;
	BasicColorRT.mDepth = 1;
	BasicColorRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	BasicColorRT.mFormat = TinyImageFormat_R8G8B8A8_UNORM;
	BasicColorRT.mSampleCount = SAMPLE_COUNT_1;
	BasicColorRT.mSampleQuality = 0;
	BasicColorRT.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
	BasicColorRT.mWidth = mWidth;
	BasicColorRT.mHeight = mHeight;

	addRenderTarget(pRenderer, &BasicColorRT, &pGBuffer_BasicRT);

#if defined(VULKAN)
	TransitionRenderTargets(pGBuffer_BasicRT, ResourceState::RESOURCE_STATE_RENDER_TARGET, pRenderer, pTransCmdPool, ppTransCmds[0], pGraphicsQueue, pTransitionCompleteFences);
#endif

	RenderTargetDesc NormalRT = {};
	NormalRT.mArraySize = 1;
	NormalRT.mDepth = 1;
	NormalRT.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	NormalRT.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
	NormalRT.mSampleCount = SAMPLE_COUNT_1;
	NormalRT.mSampleQuality = 0;
	NormalRT.mFlags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;
	NormalRT.mWidth = mWidth;
	NormalRT.mHeight = mHeight;

	addRenderTarget(pRenderer, &NormalRT, &pGBuffer_NormalRT);

#if defined(VULKAN)
	TransitionRenderTargets(pGBuffer_NormalRT, ResourceState::RESOURCE_STATE_RENDER_TARGET, pRenderer, pTransCmdPool, ppTransCmds[0], pGraphicsQueue, pTransitionCompleteFences);
#endif

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
		ScParams[0].mCount = (uint32_t)gTerrainTiledColorTexturesPacked.size();
		ScParams[0].ppTextures = gTerrainTiledColorTexturesPacked.data();
		ScParams[1].pName = "tileTexturesNrm";
		ScParams[1].mCount = (uint32_t)gTerrainTiledNormalTexturesPacked.size();
		ScParams[1].ppTextures = gTerrainTiledNormalTexturesPacked.data();
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

		TextureBarrier barriers20[] = {
			{ pTerrainRT->pTexture, RESOURCE_STATE_RENDER_TARGET },
			{ pDepthBuffer->pTexture, RESOURCE_STATE_DEPTH_WRITE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 2, barriers20);


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

		struct
		{
			mat4	viewProjMat;
		}cbRootConstantStruct;

		const float aspect = (float)mWidth / (float)mHeight;
		const float aspectInverse = 1.0f / aspect;
		const float horizontal_fov = PI / 3.0f;
		const float vertical_fov = 2.0f * atan(tan(horizontal_fov*0.5f) * aspectInverse);
		mat4 projMat = mat4::perspective(horizontal_fov, aspectInverse, 0.1f, 10000.0f);

		cbRootConstantStruct.viewProjMat = projMat * pCameraController->getViewMatrix();

		BufferUpdateDesc updateDesc = { pRenderTerrainUniformBuffer[gFrameIndex], &cbRootConstantStruct, 0, 0, sizeof(cbRootConstantStruct) };
		updateResource(&updateDesc);

		cmdBindDescriptorSet(cmd, gFrameIndex, pTerrainDescriptorSet[1]);

		for (ZoneMap::iterator iter = gZoneMap.begin(); iter != gZoneMap.end(); ++iter)
		{
			Zone* zone = iter->second;

			if (zone->visible)
			{
				cmdBindVertexBuffer(cmd, 1, &zone->pZoneVertexBuffer, NULL);
				cmdBindIndexBuffer(cmd, zone->pZoneIndexBuffer, NULL);
				cmdDrawIndexed(cmd, (uint32_t)indexBuffer.size(), 0, 0);
			}
		}

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
			{ pGBuffer_BasicRT, RESOURCE_STATE_RENDER_TARGET },
			{ pGBuffer_NormalRT, RESOURCE_STATE_RENDER_TARGET },
			{ pDepthBuffer, RESOURCE_STATE_DEPTH_WRITE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 3, barriersTerrainLighting);

		eastl::vector<RenderTarget*> RTs;

		RTs.push_back(pGBuffer_BasicRT);
		RTs.push_back(pGBuffer_NormalRT);

		cmdBindRenderTargets(cmd, (uint32_t)RTs.size(), RTs.data(), pDepthBuffer, &loadActions, NULL, NULL, -1, -1);
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
			{ pTerrainRT, RESOURCE_STATE_RENDER_TARGET },
			{ pGBuffer_BasicRT, RESOURCE_STATE_SHADER_RESOURCE },
			{ pGBuffer_NormalRT, RESOURCE_STATE_SHADER_RESOURCE },
			{ pDepthBuffer, RESOURCE_STATE_SHADER_RESOURCE },
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

		cmdBindRenderTargets(cmd, 0, NULL, 0, NULL, NULL, NULL, -1, -1);
		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

#endif

		RenderTargetBarrier barriers21[] = {
			{ pRenderTarget, RESOURCE_STATE_SHADER_RESOURCE }
		};

		cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers21);
	}

	cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
}

void Terrain::Update(float deltaTime)
{
	vec4 WindInfo = volumetricCloudsShadowCB.StandardPosition;

	if (volumetricCloudsShadowCB.ShadowInfo.getZ() != 0.0f)
	{

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
				Zone* newZone = conf_placement_new<Zone>(conf_calloc(1, sizeof(Zone)));

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
				zoneVbDesc.mDesc.mVertexStride = sizeof(float) * 5;
				zoneVbDesc.pData = newZone->PosAndUVs.data();
				zoneVbDesc.ppBuffer = &newZone->pZoneVertexBuffer;
				addResource(&zoneVbDesc);

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
	Cmd** InTransCmds, Fence* InTransitionCompleteFences, ProfileToken InGraphicsGpuProfiler, UIApp* InGAppUI)
{
	gImageCount = InImageCount;

	pCameraController = InCameraController;
	pGraphicsQueue = InGraphicsQueue;
	pTransCmdPool = InTransCmdPool;
	ppTransCmds = InTransCmds;
	pTransitionCompleteFences = InTransitionCompleteFences;
	gGpuProfileToken = InGraphicsGpuProfiler;
	pGAppUI = InGAppUI;
}
