#include <glm/glm.hpp>
using namespace glm;

struct Params {
  // Window Params
  int width = 1024;
  int height = 768;

  // PointGrid Params
  float density = 1.0f;
  int numUnitsX = 40;
  int numUnitsY = 40;
  int numUnitsZ = 40;
  float isoValue = 0.5f;
  int sizeX() { return numUnitsX * density; }
  int sizeY() { return numUnitsY * density; }
  int sizeZ() { return numUnitsZ * density; }

  // Rendering Params
  int waitTime = 5;
  bool cursorEnabled = false;
  bool showMesh = true;
  bool showMarch = false;
  bool showPoints = false;
  bool interpolate = false;
  bool useTerrain = false;
  glm::vec3 position = glm::vec3(0.0, 30, 45);

  // Sphere Params
  float radius = 9.0f;

  // Perlin Params
  float xOffset = 0.0f;
  float yOffset = 0.0f;
  float zOffset = 0.0f;

  // Cube Configuration Params
  int configIndex = 0;

  bool operator== (const Params& p){
    return (
      p.density == density &&
      p.numUnitsX == numUnitsX &&
      p.numUnitsY == numUnitsY &&
      p.numUnitsZ == numUnitsZ &&
      p.isoValue == isoValue &&
      p.showMesh == showMesh &&
      p.showMarch == showMarch &&
      p.interpolate == interpolate &&
      p.xOffset == xOffset &&
      p.yOffset == yOffset &&
      p.zOffset == zOffset &&
      p.radius == radius &&
      p.configIndex == configIndex
    );
  }
  bool operator!= (const Params& p) {
    return !(*this == p);
  }
};