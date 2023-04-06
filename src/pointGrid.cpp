#include "pointGrid.h"
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <set>
#include <map>
#include <chrono>
using namespace std::chrono;

PointGrid::PointGrid(
  Params& params
): p(params) {}

PointGrid::~PointGrid() {}

void PointGrid::generateScalarField(std::function<float(int, int, int, Params&)> func) {
  scalarField = new float[p.sizeX() * p.sizeY() * p.sizeZ()];
  for (int sX = 0; sX < p.sizeX(); sX ++) {
    for (int sY = 0; sY < p.sizeY(); sY++) {
      for (int sZ = 0; sZ < p.sizeZ(); sZ++) {
        int pX = sX - p.sizeX() / 2;
        int pY = sY;
        int pZ = sZ - p.sizeZ() / 2;

        // this->scalarField[sX * sizeY * sizeZ + sY * sizeZ + sZ] = (sin(pX + pY * pZ + pZ * 3) + 1.0f) / 2.0f;
        scalarField[coordsToIndex(sX, sY, sZ)] = func(pX, pY, pZ, p);
      }
    }
  }
}

/**
   2 +------+ 3     +---2--+ 
    /|     /|    10/|3  11/|
 6 +-----7+ |     +---6--+ | 1
   |0+----|-+ 1  7| +--0-|-+
   |/     |/      |/8   5|/9
 4 +------+ 5     +--4---+
*/

// +1, +2, +4
// +2, +4
// +1, +4
// +4
// +1, +2
// +2
// +1

void getEdgeSets(
  std::array<bool, 8>& activeNodes,
  std::vector<std::vector<std::array<int, 2>>>& edgeSets,
  std::set<int>& seenNodes,
  bool checkActive,
  int currentNode,
  int currentSet
) {
  if (currentNode < 8) {
    if (activeNodes[currentNode] == checkActive && !seenNodes.count(currentNode)) {
      seenNodes.insert(currentNode);
      for (int n = 0; n < 3; n++) {
        int adjacentNode = currentNode ^ 1UL << n;
        if (activeNodes[adjacentNode] == checkActive) {
          if (!seenNodes.count(adjacentNode)) {
            getEdgeSets(activeNodes, edgeSets, seenNodes, checkActive, adjacentNode, currentSet);
          }
        } else {
          if (currentSet >= edgeSets.size()) {
            edgeSets.push_back({});
          }
          // Put active node first for calculating normals later
          if (checkActive) {
            edgeSets[currentSet].push_back({currentNode, adjacentNode});
          } else {
            edgeSets[currentSet].push_back({adjacentNode, currentNode});
          }
        }
      }
    } else {
      getEdgeSets(activeNodes, edgeSets, seenNodes, checkActive, currentNode + 1, currentSet);
    }
  }
}

void PointGrid::generateDrawData() {
  // Clear old data
  vertices.clear();
  normals.clear();
  points.clear();
  indices.clear();
  numTrisPerCube.clear();

  std::map<std::string, int> vertexMap;
  std::map<std::string, std::vector<glm::vec3>> normalMap;
  for (int x = 0; x < p.sizeX() - 1; x++) {
    for (int y = 0; y < p.sizeY() - 1; y++) {
      for (int z = 0; z < p.sizeZ() - 1; z++) {
        int numActiveNodes = 0;
        std::array<bool, 8> activeNodes;
        std::vector<int> intersectedEdges;

        for (int i = 0; i < 8; i++) {
          int pX = x + i%2;
          int pY = y + (i % 4) / 2;
          int pZ = z + i / 4;
          bool active = false;
          if (scalarField[coordsToIndex(pX, pY, pZ)] >= p.isoValue) {
            active = true;
            numActiveNodes++;
          }

          activeNodes[i] = active;
          if ((pX == x || pX == p.sizeX() - 1) && (pY == y || pY == p.sizeY() - 1) && (pZ == z || pZ == p.sizeZ() - 1))
            points.push_back(glm::vec4((pX - p.sizeX() / 2)/p.density, pY/p.density, (pZ - p.sizeZ() / 2)/p.density, active ? 1.0f : 0.0f));
        }

        std::vector<std::vector<std::array<int, 2>>> edgeSets = {};
        std::set<int> seenNodes;
        int nextSet = 0;
        while (seenNodes.size() < (numActiveNodes <= 4 ? numActiveNodes : 8 - numActiveNodes)) {
          getEdgeSets(activeNodes, edgeSets, seenNodes, numActiveNodes <= 4, nextSet, nextSet);
          nextSet++;
        }

        /**
          NOTE:
          A cube configuration is unambiguous when:
          All the active nodes (or inactive if #active nodes > 4)
          are adjacent to another identical node AND
          (iff #active nodes > 2) there is one node that is
          adjacent to 2 other identical nodes
        */
        glm::vec3 avgPos;
        glm::vec3 avgIntersection;
        std::set<int> usedActiveNodes = {};
        std::vector<std::vector<std::array<glm::vec3, 3>>> intersectionSets = {};
        std::vector<glm::vec3> faceNormals = {};
        for (int set = 0; set < edgeSets.size(); set++) {
          intersectionSets.push_back({});
          avgPos = glm::vec3(0.0, 0.0, 0.0);
          avgIntersection = glm::vec3(0.0, 0.0, 0.0);
          for (auto edge : edgeSets[set]) {
            // Active Node
            int a = edge[0];
            // Inactive Node
            int b = edge[1];
            float pXa = (x - p.sizeX()/2 + a%2)/p.density;
            float pYa = (y + (a % 4) / 2)/p.density;
            float pZa = (z - p.sizeZ()/2 + a / 4)/p.density;

            float pXb = (x - p.sizeX()/2 + b%2)/p.density;
            float pYb = (y + (b % 4) / 2)/p.density;
            float pZb = (z - p.sizeZ()/2 + b / 4)/p.density;

            glm::vec3 pointA(pXa, pYa, pZa);
            glm::vec3 pointB(pXb, pYb, pZb);
            
            auto intersection = (pointA + pointB) / 2.0f;
            
            // Only add active node to avgPos
            avgPos += pointA;
            avgIntersection += intersection;
            usedActiveNodes.insert(a);


            intersectionSets[set].push_back({intersection, pointA, pointB});
          }
          avgPos /= edgeSets[set].size();
          avgIntersection /= edgeSets[set].size();

          // Calculate face normal
          glm::vec3 faceNormal = glm::normalize(
            avgIntersection - avgPos
          );
          faceNormals.push_back(faceNormal);
        }

        int trisPerCube = 0;
        for (int currentSet = 0; currentSet < intersectionSets.size(); currentSet++) {
          auto points = intersectionSets[currentSet];

          int numTris = 0;
          int currentPoint = 0;
          std::vector<int> usedPoints;
          std::array<int, 2> lastEdge = {-1, -1};
          // Specific case where we cannot just use the nearest points
          // We must ensure there is always a "plateau" area
          if (points.size() == 5) {
            // Find the point that is horizontally adjacent to 2 other points
            int numAdjacent = 0;
            for (int i = 0; i < points.size(); i++) {
              for (int j = 0; j < points.size(); j++) {
                if (i == j) continue;
                if (glm::distance(points[i][0], points[j][0]) == 1.0) {
                  numAdjacent++;
                }
              }
              if (numAdjacent == 2) {
                currentPoint = i;
                break;
              }
              numAdjacent = 0;
            }
          }
          
          while (numTris < points.size() - 2) {
            usedPoints.push_back(currentPoint);
            // Select first point
            auto pointA = points[currentPoint];
            // std::cout << "Point A: " << pointA.x << ", " << pointA.y << ", " << pointA.z << std::endl;
            // Get nearest point
            float minDist = 1000000;
            int minIndex = -1;
            for (int i = 0; i < points.size(); i++) {
              if (std::find(usedPoints.begin(), usedPoints.end(), i) != usedPoints.end()) continue;
              float dist = glm::distance(pointA[0], points[i][0]);
              if (dist < minDist) {
                minDist = dist;
                minIndex = i;
              }
            }
            // Select second point
            auto pointB = points[minIndex];
            // std::cout << "Point B: " << pointB.x << ", " << pointB.y << ", " << pointB.z << std::endl;

            // Get second nearest point
            float nextMinDist = 1000000;
            int nextMinIndex = -1;
            bool bothHyp = false;

            if (lastEdge[0] > -1) {
              if (lastEdge[0] == currentPoint) {
                nextMinIndex = lastEdge[1];
              } else {
                nextMinIndex = lastEdge[0];
              }
            } else {
              for (int i = 0; i < points.size(); i++) {
                if (i == minIndex || i == currentPoint) continue;
                float dist = glm::distance(pointA[0], points[i][0]);
                if (dist < nextMinDist) {
                  nextMinDist = dist;
                  nextMinIndex = i;
                }
              }
            }
          
            // Select third point
            auto pointC = points[nextMinIndex];
            // std::cout << "Point C: " << pointC.x << ", " << pointC.y << ", " << pointC.z << std::endl;

            lastEdge[0] = minIndex;
            lastEdge[1] = nextMinIndex;

            // Save point for next iteration
            currentPoint = nextMinIndex;
            // std::cout << "Current Triangle: " << numTris << std::endl;

            auto p1Interpolated = getInterpolatedIntersection(pointA[1], pointA[2]);
            auto p2Interpolated = getInterpolatedIntersection(pointB[1], pointB[2]);
            auto p3Interpolated = getInterpolatedIntersection(pointC[1], pointC[2]);

            // Calculate the triangle normal using winding direction and compare it to the face normal
            // If the triangle normal is in the opposite direction, swap the points
            auto currentNormal = glm::cross(glm::normalize(p2Interpolated - p1Interpolated), glm::normalize(p3Interpolated - p1Interpolated));
            if (glm::dot(currentNormal, faceNormals[currentSet]) > -0.0001) {
              std::swap(p2Interpolated, p3Interpolated);
            } else {
              currentNormal = -currentNormal;
            }

            // For VBO Indexing
            updateIndices(p1Interpolated, currentNormal, vertexMap, normalMap);
            updateIndices(p2Interpolated, currentNormal, vertexMap, normalMap);
            updateIndices(p3Interpolated, currentNormal, vertexMap, normalMap);

            numTris++;
          }
          trisPerCube += numTris;
        }
        numTrisPerCube.push_back(trisPerCube);
      }
    }
  }
  for (auto it = normalMap.begin(); it != normalMap.end(); it++) {
    auto normal = it->second;
    glm::vec3 avgNormal = glm::vec3(0);
    for (auto n : normal) {
      avgNormal += n;
    }
    avgNormal /= normal.size();
    normals[vertexMap[it->first]] = avgNormal;
  }
}

glm::vec3 PointGrid::getInterpolatedIntersection(glm::vec3& point1, glm::vec3& point2) {
  if (!p.interpolate) {
    return (point1 + point2) / 2.0f;
  }

  auto valP1 = scalarField[coordsToIndex(point1.x * p.density + p.sizeX()/2, point1.y * p.density, point1.z * p.density + p.sizeZ()/2)];
  auto valP2 = scalarField[coordsToIndex(point2.x * p.density + p.sizeX()/2, point2.y * p.density, point2.z * p.density + p.sizeZ()/2)];

  
  if (abs(valP1 - valP2) < 0.000001) {
    return point1;
  }
  auto mu = (p.isoValue - valP1) / (valP2 - valP1);
  return point1 + mu * (point2 - point1);
}

/**
  NOTE:
  We create a map of all the intersection vertices
  using their coordinates as a key and their index
  as the value. If the vertex already exists, we
  simply add the index of the vertex to the index
  buffer. This allows us to reuse vertices and
  reduces the size of the VBO.

  We also create a map of the normals for each
  vertex. This allows us to calculate the average
  face normal for each vertex and use that as the
  vertex normal.
*/
std::string vectorToKey(glm::vec3 vec) {
  return std::to_string(vec.x) + "," + std::to_string(vec.y) + "," + std::to_string(vec.z);
}

void PointGrid::updateIndices(
  glm::vec3& point,
  glm::vec3& normal,
  std::map<std::string, int>& vertexMap,
  std::map<std::string, std::vector<glm::vec3>>& normalMap
) {
  auto pointKey = vectorToKey(point);
  if (!vertexMap.count(pointKey) || (glm::dot(normal, normalMap[pointKey][0]) < 1 && !p.interpolate)) {
    vertexMap.insert({pointKey, vertices.size()});
    normalMap.insert({pointKey, {normal}});

    indices.push_back(vertices.size());
    vertices.push_back(point);
    normals.push_back(normal);
  } else {
    normalMap[pointKey].push_back(normal);
    indices.push_back(vertexMap[pointKey]);
  }
}

std::vector<glm::vec3>& PointGrid::getVertices() {
  return vertices;
}

std::vector<glm::vec3>& PointGrid::getNormals() {
  return normals;
}

std::vector<glm::vec4>& PointGrid::getPoints() {
  return points;
}

std::vector<unsigned int>& PointGrid::getIndices() {
  return indices;
}

std::vector<int>& PointGrid::getNumTrisPerCube() {
  return numTrisPerCube;
}