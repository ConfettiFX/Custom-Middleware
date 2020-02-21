/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This is a part of Ephemeris.
* This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge.
* You may not use the material for commercial purposes.
*
*/

#pragma once

//asimp importer
#include "../../../../The-Forge/Common_3/Renderer/IResourceLoader.h"

#include "HeightData.h"
#include "Visibility.h"

inline float3 operator - (const float3& lhs, const float3& rhs) { return float3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z); }
inline float3 operator + (const float3& lhs, const float3& rhs) { return float3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z); }
inline float3 operator / (const float3& lhs, const float rhs) { return float3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs); }
inline float3 operator * (const float3& lhs, const float rhs) { return float3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs); }

struct TerrainVertex
{
	float3 wsPos;
	float2 maskUV;
	TerrainVertex() : wsPos(0, 0, 0), maskUV(0, 0) {}
};

struct MeshSegment
{
	Buffer* indexBuffer;
	uint32 indexCount;
	TerrainBoundingBox boundingBox;
	MeshSegment() : indexCount(0) {}
};

enum TriangulationOrder
{
	ORDER_UNDEFINED = 0,
	ORDER_00_TO_11,
	ORDER_01_TO_10
};

class HemisphereBuilder
{
public:
	void build(
		Renderer* a_renderer,
		HeightData *a_heightMap,
    eastl::vector<TerrainVertex> &vertices,
    eastl::vector<MeshSegment> &meshSegments,
		const float a_planetRadius,
		float a_sampleScale,
		float a_samplingStep,
		const int a_ringCount,
		int a_gridDimension)
	{
		// Init variables
		renderer = a_renderer;
		heightmap = a_heightMap;
		samplingStep = a_samplingStep;
		sampleScale = a_sampleScale;
		planetRadius = a_planetRadius;
		gridDimension = a_gridDimension;

		int startRing = 0;
		vertices.resize(a_ringCount * gridDimension * gridDimension);
		
		// Build mesh rings
		for (int currRing = startRing; currRing < a_ringCount; ++currRing)
		{
			int currGridStart = (int)vertices.size();
			vertices.resize(vertices.size() + gridDimension * gridDimension);
			float gridScale = 1.f / (float)(1 << (a_ringCount - 1 - currRing));

			// Configure vertices

			// Fill TerrainVertex buffer
			for (int32 row = 0; row < gridDimension; ++row)
			{
				for (int32 col = 0; col < gridDimension; ++col)
				{
					vertices[currGridStart + col + row*gridDimension] = createTerrainVertex(col, row, gridScale);
				}
			}
			
			// Aligns vertices on the outer boundary
			if (currRing < a_ringCount - 1)
			{
				for (int32 i = 1; i < gridDimension - 1; i += 2)
				{
					// Top & bottom boundaries
					for (int32 row = 0; row < gridDimension; row += gridDimension - 1)
					{
						float3& v0 = vertices[currGridStart + i - 1 + row*gridDimension].wsPos;
						float3& v1 = vertices[currGridStart + i + row*gridDimension].wsPos;
						float3& v2 = vertices[currGridStart + i + 1 + row*gridDimension].wsPos;
						v1 = v3ToF3( (f3Tov3(v0) + f3Tov3(v2)) * 0.5f );//    (v0 + v2) * 0.5f;
					}

					// Left & right boundaries
					for (int32 col = 0; col < gridDimension; col += gridDimension - 1)
					{
						float3& v0 = vertices[currGridStart + col + (i - 1) * gridDimension].wsPos;
						float3& v1 = vertices[currGridStart + col + i * gridDimension].wsPos;
						float3& v2 = vertices[currGridStart + col + (i + 1) * gridDimension].wsPos;
						v1 = v3ToF3((f3Tov3(v0) + f3Tov3(v2)) * 0.5f);//    (v0 + v2) * 0.5f;
					}
				}
			}

			// Configure indices

			const int gridMiddle = (gridDimension - 1) / 2;
			const int gridQuarter = (gridDimension - 1) / 4;

			gridPitch = gridDimension;
			gridStart = currGridStart;
			// Generate indices for the current ring
			if (currRing == 0)
			{
				meshSegments.push_back(buildMeshSegment(0, 0, gridMiddle + 1, gridMiddle + 1, ORDER_00_TO_11, vertices));
				meshSegments.push_back(buildMeshSegment(gridMiddle, 0, gridMiddle + 1, gridMiddle + 1, ORDER_01_TO_10, vertices));
				meshSegments.push_back(buildMeshSegment(0, gridMiddle, gridMiddle + 1, gridMiddle + 1, ORDER_01_TO_10, vertices));
				meshSegments.push_back(buildMeshSegment(gridMiddle, gridMiddle, gridMiddle + 1, gridMiddle + 1, ORDER_00_TO_11, vertices));
			}
			else
			{
				meshSegments.push_back(buildMeshSegment(0, 0, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices));
				meshSegments.push_back(buildMeshSegment(gridQuarter, 0, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices));

				meshSegments.push_back(buildMeshSegment(gridMiddle, 0, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices));
				meshSegments.push_back(buildMeshSegment(gridQuarter * 3, 0, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices));

				meshSegments.push_back(buildMeshSegment(0, gridQuarter, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices));
				meshSegments.push_back(buildMeshSegment(0, gridMiddle, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices));

				meshSegments.push_back(buildMeshSegment(gridQuarter * 3, gridQuarter, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices));
				meshSegments.push_back(buildMeshSegment(gridQuarter * 3, gridMiddle, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices));

				meshSegments.push_back(buildMeshSegment(0, gridQuarter * 3, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices));
				meshSegments.push_back(buildMeshSegment(gridQuarter, gridQuarter * 3, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices));

				meshSegments.push_back(buildMeshSegment(gridMiddle, gridQuarter * 3, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices));
				meshSegments.push_back(buildMeshSegment(gridQuarter * 3, gridQuarter * 3, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices));
			}
		}
	}

private:
	TriangulationOrder triangulationOrder = ORDER_UNDEFINED;
	uint32 gridPitch = 0;
	uint32 gridStart = 0;
	Renderer* renderer = nullptr;
	HeightData* heightmap = nullptr;
	float sampleScale, samplingStep, planetRadius;
	int gridDimension;

	void buildTriangleStrip(int startIndex, int colStart,
		int rowStart, int colCount, int rowCount, TriangulationOrder triangleType,
		uint32 pitch, eastl::vector<uint32>& indices)
	{
		//ASSERT(triangleType == ORDER_00_TO_11 || triangleType == ORDER_01_TO_10);
		int iFirstTerrainVertex = startIndex + colStart + (rowStart + (triangleType == ORDER_00_TO_11 ? 1 : 0)) * pitch;
		if (triangulationOrder != ORDER_UNDEFINED)
		{
			// To move from one strip to another, we have to generate two degenerate triangles
			// by duplicating the last TerrainVertex in previous strip and the first TerrainVertex in new strip
			indices.push_back(indices.back());
			indices.push_back(iFirstTerrainVertex);
		}

		if (((triangulationOrder != ORDER_UNDEFINED) && (triangulationOrder != triangleType)) ||
			((triangulationOrder == ORDER_UNDEFINED) && (triangleType == ORDER_01_TO_10)))
		{
			// If triangulation orientation changes, or if start strip orientation is 01 to 10, 
			// we also have to add one additional TerrainVertex to preserve winding order
			indices.push_back(iFirstTerrainVertex);
		}
		triangulationOrder = triangleType;

		for (int iRow = 0; iRow < rowCount - 1; ++iRow)
		{
			for (int iCol = 0; iCol < colCount; ++iCol)
			{
				int iV00 = startIndex + (colStart + iCol) + (rowStart + iRow) * pitch;
				int iV01 = startIndex + (colStart + iCol) + (rowStart + iRow + 1) * pitch;
				if (triangulationOrder == ORDER_01_TO_10)
				{
					if (iCol == 0 && iRow == 0)
					{
						//ASSERT(iFirstTerrainVertex == iV00);
					}

					indices.push_back(iV00);
					indices.push_back(iV01);
				}
				else if (triangulationOrder == ORDER_00_TO_11)
				{
					if (iCol == 0 && iRow == 0)
					{
						//ASSERT(iFirstTerrainVertex == iV01);
					}

					indices.push_back(iV01);
					indices.push_back(iV00);
				}
				else
				{
					//ASSERT(false);
				}
			}

			if (iRow < rowCount - 2)
			{
				indices.push_back(indices.back());
				indices.push_back(startIndex + colStart + (rowStart + iRow + 1 + (triangleType == ORDER_00_TO_11 ? 1 : 0)) * pitch);
			}
		}
	}

	MeshSegment buildMeshSegment(int colStart, int rowStart,	int colCount,	int rowCount, 
	  enum TriangulationOrder quadTriangType, eastl::vector<TerrainVertex>& vertices)
	{
		MeshSegment mesh;

    eastl::vector<uint32> indices;
		triangulationOrder = ORDER_UNDEFINED;
		buildTriangleStrip(gridStart, colStart, rowStart, colCount, rowCount, quadTriangType, gridPitch, indices);

		mesh.indexCount = (uint32)indices.size();
		
		SyncToken token = {};
		//mesh.indexBuffer = renderer->addIndexBuffer(mesh.indexCount, sizeof(uint32), STATIC, &indices.front());
		BufferLoadDesc zoneIbDesc = {};
		zoneIbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
		zoneIbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
		zoneIbDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_OWN_MEMORY_BIT;
		zoneIbDesc.mDesc.mSize = (uint64_t)(mesh.indexCount * sizeof(uint32));
		zoneIbDesc.mDesc.mIndexType = INDEX_TYPE_UINT32;
		zoneIbDesc.pData = indices.data();
		zoneIbDesc.ppBuffer = &mesh.indexBuffer;
		addResource(&zoneIbDesc, &token, LOAD_PRIORITY_HIGH);
		waitForToken(&token);

		auto &bounds = mesh.boundingBox;
		bounds.max = float3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		bounds.min = float3(FLT_MAX, FLT_MAX, FLT_MAX);

		for (auto idx : indices)
		{
			const auto &vert = vertices[idx].wsPos;
			bounds.min.x = min(bounds.min.x, vert.x);
			bounds.min.y = min(bounds.min.y, vert.y);
			bounds.min.z = min(bounds.min.z, vert.z);

			bounds.max.x = max(bounds.max.x, vert.x);
			bounds.max.y = max(bounds.max.y, vert.y);
			bounds.max.z = max(bounds.max.z, vert.z);
		}

		indices.clear();

		return mesh;
	}

	void computeTerrainVertexHeight(TerrainVertex &TerrainVertex)
	{
		float3 &posWs = TerrainVertex.wsPos;

		float col = posWs.x / samplingStep;
		float row = posWs.z / samplingStep;
		float displacement = heightmap->getInterpolatedHeight(col, row);
		TerrainVertex.maskUV.x = (col + (float)heightmap->colOffset + 0.5f) / (float)heightmap->colCount;
		TerrainVertex.maskUV.y = (row + (float)heightmap->rowOffset + 0.5f) / (float)heightmap->rowCount;

		float3 sphereNormal;
		sphereNormal = v3ToF3(normalize(f3Tov3(posWs)));
		posWs = v3ToF3(f3Tov3(posWs) + f3Tov3(sphereNormal) * displacement * sampleScale);
	}

	TerrainVertex createTerrainVertex(uint32 col, uint32 row, float gridScale)
	{
		TerrainVertex currVert;
		auto &pos = currVert.wsPos;
		pos.x = static_cast<float>(col) / static_cast<float>(gridDimension - 1);
		pos.z = static_cast<float>(row) / static_cast<float>(gridDimension - 1);
		pos.x = pos.x * 2 - 1;
		pos.z = pos.z * 2 - 1;
		pos.y = 0;
		float fDirectionScale = 1;
		if (pos.x != 0 || pos.z != 0)
		{
			float fDX = fabsf(pos.x);
			float fDZ = fabsf(pos.z);
			float fMaxD = max(fDX, fDZ);
			float fMinD = min(fDX, fDZ);
			float fTan = fMinD / fMaxD;
			fDirectionScale = 1 / sqrtf(1 + fTan*fTan);
		}

		pos.x *= fDirectionScale*gridScale;
		pos.z *= fDirectionScale*gridScale;
		pos.y = sqrtf(max(0.0f, 1 - (pos.x * pos.x + pos.z*pos.z)));

		pos = v3ToF3(f3Tov3(pos) * planetRadius);

		computeTerrainVertexHeight(currVert);

		pos.y -= planetRadius;

		return currVert;
	}
};
