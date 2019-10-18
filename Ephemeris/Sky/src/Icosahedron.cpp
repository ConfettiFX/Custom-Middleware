#include "Icosahedron.h"

void Icosahedron::GetBasicData(eastl::vector<TriangleVertex> &vertices, eastl::vector<TriangleIndex> &indicies)
{
  const float X = .525731112119133606f;
  const float Z = .850650808352039932f;
  const float N = 0.f;

  /*
  {-X, N, Z}, { X,N,Z }, { -X,N,-Z }, { X,N,-Z },
  { N,Z,X }, { N,Z,-X }, { N,-Z,X }, { N,-Z,-X },
  { Z,X,N }, { -Z,X, N }, { Z,-X,N }, { -Z,-X, N }
  */

  vertices.push_back(TriangleVertex (-X, -N, Z));
  vertices.push_back(TriangleVertex(X, N, Z));
  vertices.push_back(TriangleVertex(-X, N, -Z));
  vertices.push_back(TriangleVertex(X, N, -Z));

  vertices.push_back(TriangleVertex(N, Z, X));
  vertices.push_back(TriangleVertex(N, Z, -X));
  vertices.push_back(TriangleVertex(N, -Z, X));
  vertices.push_back(TriangleVertex(N, -Z, -X));

  vertices.push_back(TriangleVertex(Z, X, N));
  vertices.push_back(TriangleVertex(-Z, X, N));
  vertices.push_back(TriangleVertex(Z, -X, N));
  vertices.push_back(TriangleVertex(-Z, -X, N));

  /*
  {0, 4, 1}, { 0,9,4 }, { 9,5,4 }, { 4,5,8 }, { 4,8,1 },
  { 8,10,1 }, { 8,3,10 }, { 5,3,8 }, { 5,2,3 }, { 2,7,3 },
  { 7,10,3 }, { 7,6,10 }, { 7,11,6 }, { 11,0,6 }, { 0,1,6 },
  { 6,1,10 }, { 9,0,11 }, { 9,11,2 }, { 9,2,5 }, { 7,2,11 }
  */

  indicies.push_back(TriangleIndex(0, 4, 1));
  indicies.push_back(TriangleIndex(0, 9, 4));
  indicies.push_back(TriangleIndex(9, 5, 4));
  indicies.push_back(TriangleIndex(4, 5, 8));
  indicies.push_back(TriangleIndex(4, 8, 1));

  indicies.push_back(TriangleIndex(8, 10, 1));
  indicies.push_back(TriangleIndex(8, 3, 10));
  indicies.push_back(TriangleIndex(5, 3, 8));
  indicies.push_back(TriangleIndex(5, 2, 3));
  indicies.push_back(TriangleIndex(2, 7, 3));

  indicies.push_back(TriangleIndex(7, 10, 3));
  indicies.push_back(TriangleIndex(7, 6, 10));
  indicies.push_back(TriangleIndex(7, 11, 6));
  indicies.push_back(TriangleIndex(11, 0, 6));
  indicies.push_back(TriangleIndex(0, 1, 6));

  indicies.push_back(TriangleIndex(6, 1, 10));
  indicies.push_back(TriangleIndex(9, 0, 11));
  indicies.push_back(TriangleIndex(9, 11, 2));
  indicies.push_back(TriangleIndex(9, 2, 5));
  indicies.push_back(TriangleIndex(7, 2, 11));
}

uint32_t Icosahedron::vertex_for_edge(eastl::map<eastl::pair<uint32_t, uint32_t>, uint32_t>& lookup, eastl::vector<TriangleVertex>& vertices, uint32_t first, uint32_t second)
{
  eastl::map<eastl::pair<uint32_t, uint32_t>, uint32_t>::key_type key(first, second);
  if (key.first > key.second)
    eastl::swap(key.first, key.second);

  auto inserted = lookup.insert({ key, (uint32_t)vertices.size() }); 
  // if it is new
  if (inserted.second)
  {
    TriangleVertex edge0 = vertices[first];
    TriangleVertex edge1 = vertices[second];
    vec3 newPoint = normalize(vec3(edge0.vertex[0], edge0.vertex[1], edge0.vertex[2]) + vec3(edge1.vertex[0], edge1.vertex[1], edge1.vertex[2]));
    TriangleVertex point = TriangleVertex(newPoint[0], newPoint[1], newPoint[2]);
    vertices.push_back(point);
  }

  return inserted.first->second;
}

eastl::vector<TriangleIndex> Icosahedron::Subdivide(eastl::vector<TriangleVertex>& vertices, eastl::vector<TriangleIndex> triangles)
{
  eastl::map<eastl::pair<uint32_t, uint32_t>, uint32_t> lookup;
  eastl::vector<TriangleIndex> result;

  uint32_t triangle_count = (uint32_t)triangles.size();

  for (uint32_t i=0; i< triangle_count; i++)
  {
    TriangleIndex curTriangle = triangles[i];
    TriangleIndex mid = TriangleIndex(0, 0, 0);
    for (int edge = 0; edge < 3; ++edge)
    {      
      mid.index[edge] = vertex_for_edge(lookup, vertices, curTriangle.index[edge], curTriangle.index[(edge + 1) % 3]);
    }

    result.push_back({ curTriangle.index[0], mid.index[0], mid.index[2] });
    result.push_back({ curTriangle.index[1], mid.index[1], mid.index[0] });
    result.push_back({ curTriangle.index[2], mid.index[2], mid.index[1] });
    result.push_back({ mid.index[0], mid.index[1], mid.index[2] });
  }

  return result;
}



void Icosahedron::CreateIcosphere(int subdivisions, eastl::vector<float> &vertices, eastl::vector<uint32_t> &indices)
{
	eastl::vector<TriangleVertex> triangleVertices;
	eastl::vector<TriangleIndex> triangleIndices;

  GetBasicData(triangleVertices, triangleIndices);

  for (int i = 0; i < subdivisions; ++i)
  {
    triangleIndices = Subdivide(triangleVertices, triangleIndices);
  }

  for (int i = 0; i < (int)triangleVertices.size(); ++i)
  {
    vertices.push_back(triangleVertices[i].vertex[0]);
    vertices.push_back(triangleVertices[i].vertex[1]);
    vertices.push_back(triangleVertices[i].vertex[2]);
  }

  for (int i = 0; i < (int)triangleIndices.size(); ++i)
  {
    indices.push_back(triangleIndices[i].index[0]);
    indices.push_back(triangleIndices[i].index[1]);
    indices.push_back(triangleIndices[i].index[2]);
  }

	IndexCout = (uint)indices.size();
}
