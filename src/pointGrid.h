#ifndef POINTGRID
#define POINTGRID

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <functional>
#include <map>

#include "params.h"

class PointGrid {
  Params& p;

  std::vector<unsigned int> indices;
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  
  std::vector<glm::vec4> points;
  std::vector<int> numTrisPerCube;

  float* scalarField;

  public:
    PointGrid(Params& params);
    ~PointGrid();
    std::vector<glm::vec3>& getVertices();
    std::vector<glm::vec3>& getNormals();
    std::vector<unsigned int>& getIndices();

    std::vector<glm::vec4>& getPoints();
    std::vector<unsigned int>& getPointIndices();
    
    std::vector<int>& getNumTrisPerCube();

    void generateScalarField(std::function<float(int, int, int, Params&)> func);
    void generateDrawData();
    unsigned int coordsToIndex(int x, int y, int z) {
      return z + p.sizeZ() * (y + p.sizeY() * x);
    };
    void updateIndices(
      glm::vec3& point,
      glm::vec3& normal,
      std::map<std::string, int>& vertexMap,
      std::map<std::string, std::vector<glm::vec3>>& normalMap
    );
    glm::vec3 getInterpolatedIntersection(glm::vec3& point1, glm::vec3& point2);
};

#endif