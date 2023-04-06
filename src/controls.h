#ifndef CONTROLS
#define CONTROLS

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

void computeMatricesFromInput(GLFWwindow *window);
void computeOrbitMatrices(GLFWwindow *window);
void computeStaticMatrices(GLFWwindow *window);
glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();

#endif