#ifndef UTILS
#define UTILS

#include <glm/glm.hpp>
#include <vector>

#include "lookupTables.h"

// Define the cube vertices as 8 points in a 3D space
struct CubeVertices {
    glm::vec3 p0, p1, p2, p3, p4, p5, p6, p7;
};

// Define the corner indices of a cube
const int CornerIndex[8][3] = {
    {0, 0, 0},
    {1, 0, 0},
    {1, 0, 1},
    {0, 0, 1},
    {0, 1, 0},
    {1, 1, 0},
    {1, 1, 1},
    {0, 1, 1},
};

// Generate a mesh using the Marching Cubes algorithm without a lookup table
void GenerateMeshMarchingCubes(float* voxels, int voxelWidth, int voxelHeight, int voxelDepth, float isoLevel, std::vector<glm::vec3>& vertices, std::vector<int>& indices)
{
  // Iterate through each voxel in the grid
  for (int z = 0; z < voxelDepth - 1; z++) {
    for (int y = 0; y < voxelHeight - 1; y++) {
      for (int x = 0; x < voxelWidth - 1; x++) {
        // Define the cube vertices and their values
        glm::vec3 cube[8];
        float val[8];
        for (int i = 0; i < 8; i++) {
          int cx = x + CornerIndex[i][0];
          int cy = y + CornerIndex[i][1];
          int cz = z + CornerIndex[i][2];
          int index = cz * voxelWidth * voxelHeight + cy * voxelWidth + cx;
          cube[i] = glm::vec3(cx, cy, cz);
          val[i] = voxels[index];
        }

        // Determine the cube configuration
        int cubeIndex = 0;
        if (val[0] < isoLevel) cubeIndex |= 1;
        if (val[1] < isoLevel) cubeIndex |= 2;
        if (val[2] < isoLevel) cubeIndex |= 4;
        if (val[3] < isoLevel) cubeIndex |= 8;
        if (val[4] < isoLevel) cubeIndex |= 16;
        if (val[5] < isoLevel) cubeIndex |= 32;
        if (val[6] < isoLevel) cubeIndex |= 64;
        if (val[7] < isoLevel) cubeIndex |= 128;

        // Generate the triangle vertices
        for (int i = 0; i < 3; i++) {
          // Calculate the edge vertices and their values
          int edgeIndex = edgeTable[cubeIndex * i];
          int v1Index = edgeIndex & 0x7;
          int v2Index = (edgeIndex >> 4) & 0x7;
          float v1Val = val[v1Index];
          float v2Val = val[v2Index];
          glm::vec3 v1 = cube[v1Index];
          glm::vec3 v2 = cube[v2Index];

          // Calculate the intersection point of the iso-surface with the edge
          glm::vec3 vertex = (v1Val - isoLevel) * (v2 - v1) / (v2Val - v1Val) + v1;

          // Add the vertex to the mesh
          vertices.push_back(vertex);

          // Add the vertex index to the triangle
          indices.push_back(vertices.size() - 1);
        }
      }
    }
  }
}

#endif