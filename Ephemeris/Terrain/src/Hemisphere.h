/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#pragma once

// asimp importer
#include "../../../../The-Forge/Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"

#include "HeightData.h"
#include "Visibility.h"

inline float3 operator-(const float3& lhs, const float3& rhs) { return float3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z); }
inline float3 operator+(const float3& lhs, const float3& rhs) { return float3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z); }
inline float3 operator/(const float3& lhs, const float rhs) { return float3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs); }
inline float3 operator*(const float3& lhs, const float rhs) { return float3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs); }

struct TerrainVertex
{
    float3 wsPos;
    float2 maskUV;
    TerrainVertex(): wsPos(0, 0, 0), maskUV(0, 0) {}
};

struct MeshSegment
{
    Buffer*            indexBuffer;
    uint32_t           indexCount;
    TerrainBoundingBox boundingBox;
    MeshSegment(): indexBuffer(NULL), indexCount(0) {}
};

enum TriangulationOrder
{
    ORDER_UNDEFINED = 0,
    ORDER_00_TO_11,
    ORDER_01_TO_10
};

static inline void buildTriangleStrip(TriangulationOrder triangulationOrder, uint32_t startIndex, uint32_t colStart, uint32_t rowStart,
                                      uint32_t colCount, uint32_t rowCount, TriangulationOrder triangleType, uint32_t pitch,
                                      uint32_t* outIndexCount, uint32_t** outIndices)
{
    uint32_t  maxPossibleIndexCount = 3 + 4 * (rowCount - 1) * colCount;
    uint32_t  indexCount = 0;
    uint32_t* indices = (uint32_t*)tf_malloc(4 * maxPossibleIndexCount);

    // ASSERT(triangleType == ORDER_00_TO_11 || triangleType == ORDER_01_TO_10);
    uint32_t iFirstTerrainVertex = startIndex + colStart + (rowStart + (triangleType == ORDER_00_TO_11 ? 1 : 0)) * pitch;
    if (!VERIFYMSG(triangulationOrder == ORDER_UNDEFINED, "Only ORDER_UNDEFINED triangulation order is supported"))
    {
        return;
    }

    if (triangleType == ORDER_01_TO_10)
    {
        // If triangulation orientation changes, or if start strip orientation is 01 to 10,
        // we also have to add one additional TerrainVertex to preserve winding order
        indices[indexCount++] = iFirstTerrainVertex;
    }
    triangulationOrder = triangleType;

    for (uint32_t iRow = 0; iRow < rowCount - 1; ++iRow)
    {
        for (uint32_t iCol = 0; iCol < colCount; ++iCol)
        {
            uint32_t iV00 = startIndex + (colStart + iCol) + (rowStart + iRow) * pitch;
            uint32_t iV01 = startIndex + (colStart + iCol) + (rowStart + iRow + 1) * pitch;
            if (triangulationOrder == ORDER_01_TO_10)
            {
                if (iCol == 0 && iRow == 0)
                {
                    // ASSERT(iFirstTerrainVertex == iV00);
                }

                indices[indexCount++] = iV00;
                indices[indexCount++] = iV01;
            }
            else if (triangulationOrder == ORDER_00_TO_11)
            {
                if (iCol == 0 && iRow == 0)
                {
                    // ASSERT(iFirstTerrainVertex == iV01);
                }

                indices[indexCount++] = iV01;
                indices[indexCount++] = iV00;
            }
            else
            {
                // ASSERT(false);
            }
        }

        if (iRow < rowCount - 2)
        {
            ASSERT(indexCount);
            uint32_t lastIndex = indices[indexCount - 1];
            indices[indexCount++] = lastIndex;
            indices[indexCount++] = startIndex + colStart + (rowStart + iRow + 1 + (triangleType == ORDER_00_TO_11 ? 1 : 0)) * pitch;
        }
    }

    *outIndexCount = indexCount;
    *outIndices = indices;
}

class HemisphereBuilder
{
public:
    void build(Renderer* a_renderer, HeightData* a_heightMap, const float a_planetRadius, float a_sampleScale, float a_samplingStep,
               uint32_t a_ringCount, uint32_t a_gridDimension, uint32_t* outVertexCount, TerrainVertex** outVertices,
               uint32_t* outMeshSegmentCount, MeshSegment** outMeshSegments)
    {
        ASSERT(a_ringCount);

        // Init variables
        renderer = a_renderer;
        heightmap = a_heightMap;
        samplingStep = a_samplingStep;
        sampleScale = a_sampleScale;
        planetRadius = a_planetRadius;
        gridDimension = a_gridDimension;

        uint32_t       vertexCount = a_ringCount * gridDimension * gridDimension + a_ringCount * gridDimension * gridDimension;
        TerrainVertex* vertices = (TerrainVertex*)tf_malloc(sizeof(TerrainVertex) * vertexCount);

        uint32_t     meshSegmentCount = 4 + (a_ringCount - 1) * 12;
        MeshSegment* meshSegments = (MeshSegment*)tf_malloc(sizeof(MeshSegment) * meshSegmentCount);

        MeshSegment* segment = meshSegments;

        // Build mesh rings
        uint32_t currGridStart = a_ringCount * gridDimension * gridDimension;
        for (uint32_t currRing = 0; currRing < a_ringCount; ++currRing)
        {
            float gridScale = 1.f / (float)(1 << (a_ringCount - 1 - currRing));

            // Configure vertices

            // Fill TerrainVertex buffer
            for (uint32_t row = 0; row < gridDimension; ++row)
            {
                for (uint32_t col = 0; col < gridDimension; ++col)
                {
                    vertices[currGridStart + col + row * gridDimension] = createTerrainVertex(col, row, gridScale);
                }
            }

            // Aligns vertices on the outer boundary
            if (currRing < a_ringCount - 1)
            {
                for (uint32_t i = 1; i < gridDimension - 1; i += 2)
                {
                    // Top & bottom boundaries
                    for (uint32_t row = 0; row < gridDimension; row += gridDimension - 1)
                    {
                        float3& v0 = vertices[currGridStart + i - 1 + row * gridDimension].wsPos;
                        float3& v1 = vertices[currGridStart + i + row * gridDimension].wsPos;
                        float3& v2 = vertices[currGridStart + i + 1 + row * gridDimension].wsPos;
                        v1 = v3ToF3((f3Tov3(v0) + f3Tov3(v2)) * 0.5f); //    (v0 + v2) * 0.5f;
                    }

                    // Left & right boundaries
                    for (uint32_t col = 0; col < gridDimension; col += gridDimension - 1)
                    {
                        float3& v0 = vertices[currGridStart + col + (i - 1) * gridDimension].wsPos;
                        float3& v1 = vertices[currGridStart + col + i * gridDimension].wsPos;
                        float3& v2 = vertices[currGridStart + col + (i + 1) * gridDimension].wsPos;
                        v1 = v3ToF3((f3Tov3(v0) + f3Tov3(v2)) * 0.5f); //    (v0 + v2) * 0.5f;
                    }
                }
            }

            // Configure indices

            uint32_t gridMiddle = (gridDimension - 1) / 2;
            uint32_t gridQuarter = (gridDimension - 1) / 4;

            gridPitch = gridDimension;
            gridStart = currGridStart;
            // Generate indices for the current ring
            if (currRing == 0)
            {
                *(segment++) = buildMeshSegment(0, 0, gridMiddle + 1, gridMiddle + 1, ORDER_00_TO_11, vertices);
                *(segment++) = buildMeshSegment(gridMiddle, 0, gridMiddle + 1, gridMiddle + 1, ORDER_01_TO_10, vertices);
                *(segment++) = buildMeshSegment(0, gridMiddle, gridMiddle + 1, gridMiddle + 1, ORDER_01_TO_10, vertices);
                *(segment++) = buildMeshSegment(gridMiddle, gridMiddle, gridMiddle + 1, gridMiddle + 1, ORDER_00_TO_11, vertices);
            }
            else
            {
                *(segment++) = buildMeshSegment(0, 0, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices);
                *(segment++) = buildMeshSegment(gridQuarter, 0, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices);

                *(segment++) = buildMeshSegment(gridMiddle, 0, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices);
                *(segment++) = buildMeshSegment(gridQuarter * 3, 0, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices);

                *(segment++) = buildMeshSegment(0, gridQuarter, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices);
                *(segment++) = buildMeshSegment(0, gridMiddle, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices);

                *(segment++) = buildMeshSegment(gridQuarter * 3, gridQuarter, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices);
                *(segment++) = buildMeshSegment(gridQuarter * 3, gridMiddle, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices);

                *(segment++) = buildMeshSegment(0, gridQuarter * 3, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices);
                *(segment++) = buildMeshSegment(gridQuarter, gridQuarter * 3, gridQuarter + 1, gridQuarter + 1, ORDER_01_TO_10, vertices);

                *(segment++) = buildMeshSegment(gridMiddle, gridQuarter * 3, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices);
                *(segment++) =
                    buildMeshSegment(gridQuarter * 3, gridQuarter * 3, gridQuarter + 1, gridQuarter + 1, ORDER_00_TO_11, vertices);
            }

            currGridStart += gridDimension * gridDimension;
        }

        ASSERT(segment == meshSegments + meshSegmentCount);

        *outVertexCount = vertexCount;
        *outVertices = vertices;

        *outMeshSegmentCount = meshSegmentCount;
        *outMeshSegments = meshSegments;
    }

private:
    uint32_t    gridPitch = 0;
    uint32_t    gridStart = 0;
    Renderer*   renderer = nullptr;
    HeightData* heightmap = nullptr;
    float       sampleScale = 0.0f, samplingStep = 0.0f, planetRadius = 0.0f;
    uint32_t    gridDimension = 0;

    MeshSegment buildMeshSegment(uint32_t colStart, uint32_t rowStart, uint32_t colCount, uint32_t rowCount,
                                 enum TriangulationOrder quadTriangType, const TerrainVertex* vertices)
    {
        MeshSegment mesh;

        uint32_t* indices;
        buildTriangleStrip(ORDER_UNDEFINED, gridStart, colStart, rowStart, colCount, rowCount, quadTriangType, gridPitch, &mesh.indexCount,
                           &indices);

        SyncToken      token = {};
        // mesh.indexBuffer = renderer->addIndexBuffer(mesh.indexCount, sizeof(uint32_t), STATIC, &indices.front());
        BufferLoadDesc zoneIbDesc = {};
        zoneIbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
        zoneIbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        zoneIbDesc.mDesc.mSize = (uint64_t)(mesh.indexCount * sizeof(uint32_t));
        zoneIbDesc.pData = indices;
        zoneIbDesc.ppBuffer = &mesh.indexBuffer;
        addResource(&zoneIbDesc, &token);
        waitForToken(&token);

        auto& bounds = mesh.boundingBox;
        bounds.max = float3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        bounds.min = float3(FLT_MAX, FLT_MAX, FLT_MAX);

        for (uint32_t i = 0; i < mesh.indexCount; ++i)
        {
            const auto& vert = vertices[indices[i]].wsPos;
            bounds.min.x = min(bounds.min.x, vert.x);
            bounds.min.y = min(bounds.min.y, vert.y);
            bounds.min.z = min(bounds.min.z, vert.z);

            bounds.max.x = max(bounds.max.x, vert.x);
            bounds.max.y = max(bounds.max.y, vert.y);
            bounds.max.z = max(bounds.max.z, vert.z);
        }

        tf_free(indices);

        return mesh;
    }

    void computeTerrainVertexHeight(TerrainVertex& TerrainVertex)
    {
        float3& posWs = TerrainVertex.wsPos;

        float col = posWs.x / samplingStep;
        float row = posWs.z / samplingStep;
        float displacement = heightmap->getInterpolatedHeight(col, row);
        TerrainVertex.maskUV.x = (col + (float)heightmap->colOffset + 0.5f) / (float)heightmap->colCount;
        TerrainVertex.maskUV.y = (row + (float)heightmap->rowOffset + 0.5f) / (float)heightmap->rowCount;

        float3 sphereNormal;
        sphereNormal = v3ToF3(normalize(f3Tov3(posWs)));
        posWs = v3ToF3(f3Tov3(posWs) + f3Tov3(sphereNormal) * displacement * sampleScale);
    }

    TerrainVertex createTerrainVertex(uint32_t col, uint32_t row, float gridScale)
    {
        TerrainVertex currVert;
        auto&         pos = currVert.wsPos;
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
            fDirectionScale = 1 / sqrtf(1 + fTan * fTan);
        }

        pos.x *= fDirectionScale * gridScale;
        pos.z *= fDirectionScale * gridScale;
        pos.y = sqrtf(max(0.0f, 1 - (pos.x * pos.x + pos.z * pos.z)));

        pos = v3ToF3(f3Tov3(pos) * planetRadius);

        computeTerrainVertexHeight(currVert);

        pos.y -= planetRadius;

        return currVert;
    }
};
