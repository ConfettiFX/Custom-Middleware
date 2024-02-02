/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#include "Icosahedron.h"

#include "../../../../The-Forge/Common_3/Utilities/Interfaces/ILog.h"

static const float icosahedronA = 0.85065080835204f;   // sqrt(2.0f / (5.0f - sqrt(5.0f)))
static const float icosahedronB = 0.5257311121191336f; // sqrt(2.0f / (5.0f + sqrt(5.0f)))

static const uint32_t icosahedronVertexCount = 12;

static const VertexF3 icosahedronVertices[icosahedronVertexCount] = {
    { { -icosahedronB, icosahedronA, 0 } },  { { icosahedronB, icosahedronA, 0 } },   { { -icosahedronB, -icosahedronA, 0 } },
    { { icosahedronB, -icosahedronA, 0 } },  { { 0, -icosahedronB, icosahedronA } },  { { 0, icosahedronB, icosahedronA } },
    { { 0, -icosahedronB, -icosahedronA } }, { { 0, icosahedronB, -icosahedronA } },  { { icosahedronA, 0, -icosahedronB } },
    { { icosahedronA, 0, icosahedronB } },   { { -icosahedronA, 0, -icosahedronB } }, { { -icosahedronA, 0, icosahedronB } },
};

static const uint32_t icosahedronTriangleCount = 20;
static const uint32_t icosahedronIndices[icosahedronTriangleCount * 3] = {
    0, 5, 11, 0, 1, 5, 0, 7, 1, 0, 10, 7, 0, 11, 10, 1, 9, 5, 5, 4,  11, 11, 2,  10, 10, 6, 7, 7, 8, 1,
    3, 4, 9,  3, 2, 4, 3, 6, 2, 3, 8,  6, 3, 9,  8,  4, 5, 9, 2, 11, 4,  6,  10, 2,  8,  7, 6, 9, 1, 8,
};

struct EdgeNode
{
    uint64_t key;
    uint32_t value;
};

typedef EdgeNode* EdgeMapStbDs;

static uint32_t insertOrGetMidpoint(EdgeMapStbDs* midPointMap, VertexStbDsArray* vertices, uint32_t lIndex, uint32_t rIndex)
{
    ASSERT(midPointMap);
    ASSERT(vertices && *vertices);

    uint64_t li = lIndex;
    uint64_t ri = rIndex;

    if (lIndex > rIndex)
        ri <<= 32;
    else
        li <<= 32;

    uint64_t key = ri | li;

    EdgeNode* point = hmgetp_null(*midPointMap, key);
    if (point)
        return point->value;

    VertexF3& a = (*vertices)[lIndex]; //-V595
    VertexF3& b = (*vertices)[rIndex]; //-V595

    VertexF3 c{ {
        (a.pos[0] + b.pos[0]) / 2,
        (a.pos[1] + b.pos[1]) / 2,
        (a.pos[2] + b.pos[2]) / 2,
    } };

    uint32_t index = (uint32_t)arrlen(*vertices);
    hmput(*midPointMap, key, index);
    arrpush(*vertices, c);
    return index;
}

static void Subdivide(VertexStbDsArray* outVertices, IndexStbDsArray* outIndices)
{
    EdgeMapStbDs midPointMap = NULL;

    uint32_t* oldIndices = *outIndices;

    uint32_t        numTriangles = (uint32_t)arrlen(*outIndices) / 3;
    IndexStbDsArray newIndices = NULL;
    arrsetlen(newIndices, numTriangles * 12);
    ASSERT(newIndices);

    for (uint32_t i = 0; i < numTriangles; ++i)
    {
        uint32_t t0 = oldIndices[i * 3 + 0];
        uint32_t t1 = oldIndices[i * 3 + 1];
        uint32_t t2 = oldIndices[i * 3 + 2];

        uint32_t m0 = insertOrGetMidpoint(&midPointMap, outVertices, t0, t1);
        uint32_t m1 = insertOrGetMidpoint(&midPointMap, outVertices, t1, t2);
        uint32_t m2 = insertOrGetMidpoint(&midPointMap, outVertices, t2, t0);

        uint32_t* ind = newIndices + i * 12;

        *(ind++) = t0; //-V769
        *(ind++) = m0;
        *(ind++) = m2;
        *(ind++) = m0;
        *(ind++) = t1;
        *(ind++) = m1;
        *(ind++) = m0;
        *(ind++) = m1;
        *(ind++) = m2;
        *(ind++) = m2;
        *(ind++) = m1;
        *(ind++) = t2;
    }

    hmfree(midPointMap);
    arrfree(*outIndices);
    *outIndices = newIndices;
}

void CreateIcosphere(uint32_t subdivisions, VertexStbDsArray* outVertices, IndexStbDsArray* outIndices)
{
    VertexStbDsArray vertices = NULL;
    IndexStbDsArray  indices = NULL;
    arrsetlen(vertices, icosahedronVertexCount);
    arrsetlen(indices, icosahedronTriangleCount * 3);
    memcpy((VertexF3*)vertices, icosahedronVertices, icosahedronVertexCount * sizeof(*vertices));
    memcpy(indices, icosahedronIndices, icosahedronTriangleCount * 3 * sizeof(*indices));

    for (uint32_t i = 0; i < subdivisions; ++i)
    {
        Subdivide(&vertices, &indices);
    }

    *outVertices = vertices;
    *outIndices = indices;
}
