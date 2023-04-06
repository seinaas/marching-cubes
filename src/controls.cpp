#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

#include "controls.h"
#include "params.h"

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix(){
	return ViewMatrix;
}
glm::mat4 getProjectionMatrix(){
	return ProjectionMatrix;
}

glm::vec3 position = glm::vec3(0, 0, 5);
float horizontalAngle = 3.14f;
float verticalAngle = 0.0f;
float initialFoV = 70.0f;
float speed = 10.0f;
float mouseSpeed = 0.08f;

void computeMatricesFromInput(GLFWwindow *window) {
  static double lastTime = glfwGetTime();
  double currentTime = glfwGetTime();
  float deltaTime = float(currentTime - lastTime);

  auto params = (Params*)glfwGetWindowUserPointer(window);

  if (!params->cursorEnabled) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    glfwSetCursorPos(window, params->width / 2, params->height / 2);

    horizontalAngle += mouseSpeed * deltaTime * float(params->width / 2 - xpos);

    float verticalDelta = mouseSpeed * deltaTime * float(params->height / 2 - ypos);
    if (verticalAngle + verticalDelta < 1.5f && verticalAngle + verticalDelta > -1.5f) {
      verticalAngle += verticalDelta;
    }
  }

  glm::vec3 direction(cos(verticalAngle) * sin(horizontalAngle),
                      sin(verticalAngle),
                      cos(verticalAngle) * cos(horizontalAngle));

  glm::vec3 right = glm::vec3(sin(horizontalAngle - 3.14f / 2.0f), 0,
                              cos(horizontalAngle - 3.14f / 2.0f));
  
  glm::vec3 up = glm::vec3(0, 1, 0);
  
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    params->position += direction * deltaTime * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    params->position -= direction * deltaTime * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    params->position += right * deltaTime * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    params->position -= right * deltaTime * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    params->position += up * deltaTime * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    params->position -= up * deltaTime * speed;
  }

  if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    speed += 1.0f;
  }

  if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
    if (speed > 1.0f) {
      speed -= 1.0f;
    }
  }

  ProjectionMatrix = glm::perspective(glm::radians(initialFoV), 4.0f / 3.0f, 0.1f, 100.0f);
  ViewMatrix = glm::lookAt(params->position, params->position + direction, up);

  lastTime = currentTime;
}

void computeOrbitMatrices(GLFWwindow *window) {
  static double lastTime = glfwGetTime();
  double currentTime = glfwGetTime();
  float deltaTime = float(currentTime - lastTime);

  auto params = (Params*)glfwGetWindowUserPointer(window);

  // creates rotation matrix from identity matrix, rotates by deltaTime, and rotates around y axis
  glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), deltaTime * 0.5f, glm::vec3(0, 1, 0));
  // rotates position vector by rotation matrix
  params->position = rotation * glm::vec4(params->position, 1.0f);

  ProjectionMatrix = glm::perspective(glm::radians(initialFoV), 4.0f / 3.0f, 0.1f, 100.0f);
  ViewMatrix = glm::lookAt(params->position, glm::vec3(0, params->sizeY()/2, 0), glm::vec3(0, 1, 0)) * rotation;

  lastTime = currentTime;
}

void computeStaticMatrices(GLFWwindow *window) {
  ProjectionMatrix = glm::perspective(glm::radians(initialFoV), 4.0f / 3.0f, 0.1f, 100.0f);
  ViewMatrix = glm::lookAt(glm::vec3(0, 0, 50), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
}
