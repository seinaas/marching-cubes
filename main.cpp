#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
using namespace glm;

#include "./src/controls.h"
#include "./src/pointGrid.h"

#include "./external/FastNoise.hpp"
#include "./external/imgui/imgui.h"
#include "./external/imgui/backends/imgui_impl_glfw.h"
#include "./external/imgui/backends/imgui_impl_opengl3.h"

GLuint loadShaders(const char * vertex_file_path,const char * fragment_file_path){

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open()){
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}else{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}
	
	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

GLFWwindow* initWindow(int width, int height, GLFWwindow* window) {
  glewExperimental = true;
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    exit(0);
  }

  glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1); // Debug context

  GLFWwindow* w;
  w = glfwCreateWindow(width, height, "Marching Cubes", NULL, NULL);

  if (w == NULL) {
    fprintf(stderr, "Failed to open GLFW window.");
    glfwTerminate();
    exit(0);
  }

  glfwMakeContextCurrent(w);

  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    exit(0);
  }

  return w;
}

float getSphere(int x, int y, int z, Params& p) {
  return 2 - sqrt(x * x + (y - p.sizeY()/2) * (y - p.sizeY()/2) + z * z) / p.radius;
}

float getPerlin(int x, int y, int z, Params& p) {
  FastNoiseLite noise;
  noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
  noise.SetFractalType(FastNoiseLite::FractalType_DomainWarpIndependent);
  noise.SetFrequency(0.05);
  noise.SetFractalOctaves(3);
  double val = (noise.GetNoise(x/p.density + p.xOffset, y/p.density + p.yOffset, z/p.density + p.zOffset) + 1.0)/2.0;

  return y == 0 ? 1 : -y / float(p.numUnitsY) + val;
}

float getPrism(int x, int y, int z, Params& p) {
  // Make a prism
  if ((x > -p.sizeX()/2 + 1 && x < p.sizeX()/2 - 1) && (y > 1 && y < p.sizeY() - 1) && (z > -p.sizeZ()/2 + 1 && z < p.sizeZ()/2 - 1)) {
    return 1;
  }

  return 0;
}

float getCubeConfigs(int x, int y, int z, Params& p) {
  switch(p.configIndex) {
    case 0:
      return 0;
    case 1:
      if (x == -1 && y == 0 && z == 0) return 1;
      break;
    case 2:
      if (z == 0 && y == 0) return 1;
      break;
    case 3:
      if ((x == -1 && y == 0 && z == 0) || (x == 0 && y == 1 && z == 0)) return 1;
      break;
    case 4:
      if ((x == -1 && y == 0 && z == 0) || (x == 0 && y == 1 && z == -1)) return 1;
      break;
    case 5:
      if (y == 0 && (z == -1 || x == 0)) return 1;
      break;
    case 6:
      if ((z == 0 && y == 0) || (z == -1 && y == 1 && x == 0)) return 1;
      break;
    case 7:
      if ((x == -1 && y == 1 && z == 0) || (x == 0 && y == 0 && z == 0) || (x == 0 && y == 1 && z == -1)) return 1;
      break;
    case 8:
      if (y == 0) return 1;
      break;
    case 9:
      if ((y == 0 && (x == -1 || z == -1)) || (y == 1 && x == -1 && z == -1)) return 1;
      break;
    case 10:
      if (x != z) return 1;
      break;
    case 11:
      if ((y == 0 && (z == -1 || x == -1)) || (y == 1 && x == 0 && z == -1)) return 1;
      break;
    case 12:
      if ((y == 0 && (z == -1 || x == 0)) || (y == 1 && z == 0 && x == -1)) return 1;
      break;
    case 13:
      if ((y == 0 && x != z) || (y == 1 && x == z)) return 1;
      break;
    case 14:
      if ((y == 0 && (z == -1 || x == 0)) || (y == 1 && x == -1 && z == -1)) return 1;
      break;
  }

  return 0;
}

float templateFunc(int x, int y, int z, Params& p) {
  if ((y == 0 && (z == -1 || x == -1)) || (y == 1 && x == -1 && z == -1)) return 1;

  return 0;
}

void escapeCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  auto params = (Params *)glfwGetWindowUserPointer(window);
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    if (!params->cursorEnabled) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    params->cursorEnabled = !params->cursorEnabled;
  }
}

void rerender(
  PointGrid &pointGrid,
  std::vector<glm::vec3> &vertices,
  std::vector<glm::vec3> &normals,
  std::vector<glm::vec4> &points,
  std::vector<unsigned int> &indices,
  GLuint &vertexbuffer,
  GLuint &normalbuffer,
  GLuint &pointbuffer,
  GLuint &indexbuffer,
  float (*currentFunc)(int, int, int, Params&)
) {
  pointGrid.generateScalarField(currentFunc);
  pointGrid.generateDrawData();
  vertices = pointGrid.getVertices();
  normals = pointGrid.getNormals();
  points = pointGrid.getPoints();
  indices = pointGrid.getIndices();

  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STREAM_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
  glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STREAM_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, pointbuffer);
  glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec4), &points[0], GL_STREAM_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, indexbuffer);
  glBufferData(GL_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STREAM_DRAW);
}

int main() {
  Params params;
  Params oldParams;
  // These variables are used to dynamically update the vertices
  float lastIsoValue = params.isoValue;
  float lastDensity = params.density;
  
  float lastXOffset = params.xOffset;
  float lastYOffset = params.yOffset;
  float lastZOffset = params.zOffset;

  float (*currentFunc)(int, int, int, Params&) = getSphere;

  PointGrid pointGrid(params);

  GLFWwindow* window = initWindow(params.width, params.height, window);
  glfwSetWindowUserPointer(window, &params);
  glfwSetKeyCallback(window, escapeCallback);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;

  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  GLuint VertexArrayID;
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  pointGrid.generateScalarField(currentFunc);
  pointGrid.generateDrawData();

  std::vector<glm::vec3> vertices = pointGrid.getVertices();
  std::vector<glm::vec3> normals = pointGrid.getNormals();
  std::vector<glm::vec4> points = pointGrid.getPoints();
  std::vector<GLuint> indices = pointGrid.getIndices();
  std::vector<int> numTrisPerCube = pointGrid.getNumTrisPerCube();

  // Create and bind vertex buffer
  GLuint vertexbuffer;
  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STREAM_DRAW);

  GLuint indexbuffer;
  glGenBuffers(1, &indexbuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);
  
  // Create and bind normal buffer
  GLuint normalbuffer;
  glGenBuffers(1, &normalbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
  glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STREAM_DRAW);

  GLuint pointbuffer;
  glGenBuffers(1, &pointbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, pointbuffer);
  glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec4), &points[0], GL_STREAM_DRAW);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  glfwPollEvents();
  
  glfwSetCursorPos(window, params.width / 2, params.height / 2);

  glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // glEnable(GL_CULL_FACE);
  glFrontFace(GL_CW);

  glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

  // Load mesh shaders
  GLuint programID = loadShaders("./shaders/VertexShader.glsl", "./shaders/FragmentShader.glsl");

  // Get location of uniform variables for MVP matrices to be used in shaders
  GLuint MatrixID = glGetUniformLocation(programID, "MVP");
  GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

  // Get location of uniform variables for height to be used in shaders
  GLuint heightID = glGetUniformLocation(programID, "height");

  GLuint useTerrainId = glGetUniformLocation(programID, "useTerrain");

  // Get location of uniform variables for light position to be used in shaders
  GLuint lightID = glGetUniformLocation(programID, "LightPosition_worldspace");

  // Load point shaders
  GLuint pointProgramID = loadShaders("./shaders/PointVertexShader.glsl", "./shaders/PointFragmentShader.glsl");

   // Get location of uniform variables for MVP matrices to be used in shaders
  GLuint PointMatrixID = glGetUniformLocation(pointProgramID, "MVP");
  GLuint PointViewMatrixID = glGetUniformLocation(pointProgramID, "V");
  GLuint PointModelMatrixID = glGetUniformLocation(pointProgramID, "M");
  
  int currFrame = 0;
  int currCube = 0;
  int totalTris = 0;
  do {
    if (oldParams != params) {
      oldParams = params;
      currCube = 0;
      currFrame = 0;
      totalTris = 0;

      rerender(pointGrid, vertices, normals, points, indices, vertexbuffer, normalbuffer, pointbuffer, indexbuffer, currentFunc);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    
    // Compute MVP matrix from keyboard and mouse input
    computeMatricesFromInput(window);
    glm::mat4 ProjectionMatrix = getProjectionMatrix();
    glm::mat4 ViewMatrix = getViewMatrix();
    glm::mat4 ModelMatrix = glm::mat4(1.0f);

    glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

    if (params.showMesh) {
      glUseProgram(programID);

      glEnableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
      glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)0
      );

      glEnableVertexAttribArray(1);
      glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
      glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)0
      );

      // Send transformation to currently bound shader
      glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
      glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
      glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

      // Send height to shader
      glUniform1f(heightID, params.numUnitsY);

      glUniform1i(useTerrainId, params.useTerrain);

      // Send light position to shader
      glm::vec3 lightPos = params.position;
      glUniform3f(lightID, lightPos.x, lightPos.y + 10.0f, lightPos.z);

      // glDrawArrays(GL_TRIANGLES, 0, vertices.size());
      if (params.showMarch && currCube < numTrisPerCube.size()) {
        currFrame++;
        if (currFrame % params.waitTime == 0) {
          totalTris += numTrisPerCube[currCube];
          currCube++;
          while(numTrisPerCube[currCube] == 0) {
            currCube++;
          }
        }
        glDrawElements(GL_TRIANGLES, indices.size() - (indices.size() - totalTris * 3), GL_UNSIGNED_INT, 0);
      } else {
        if (currFrame > 0 && !params.showMarch) {
          currFrame = 0;
          currCube = 0;
          totalTris = 0;
        }
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
      }
    }

    if (params.showPoints) {
      glUseProgram(pointProgramID);

      glEnableVertexAttribArray(3);
      glBindBuffer(GL_ARRAY_BUFFER, pointbuffer);
      glVertexAttribPointer(
        3,
        4,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)0
      );

      // Send transformation to currently bound shader
      glUniformMatrix4fv(PointMatrixID, 1, GL_FALSE, &MVP[0][0]);
      glUniformMatrix4fv(PointModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
      glUniformMatrix4fv(PointViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
      
      // Uncomment to make points appear on top of triangles
      // glClear(GL_DEPTH_BUFFER_BIT);
      glDrawArrays(GL_POINTS, 0, points.size());
    }

  {
      static int counter = 0;

      ImGui::Begin("Controls");

      ImGui::Checkbox("Show Points", &params.showPoints);
      ImGui::SameLine();
      ImGui::Checkbox("Show Mesh", &params.showMesh);
      ImGui::Checkbox("Interpolate", &params.interpolate);
      ImGui::SameLine();
      ImGui::Checkbox("Show March", &params.showMarch);

      ImGui::SliderFloat("IsoValue", &params.isoValue, 0.0f, 1.0f);

      ImGui::Separator();

      ImGui::BeginGroup();
      ImGui::Text("Grid Size");
      ImGui::SliderInt("X", &params.numUnitsX, 2, 100);
      ImGui::SliderInt("Y", &params.numUnitsY, 2, 100);
      ImGui::SliderInt("Z", &params.numUnitsZ, 2, 100);
      ImGui::SliderFloat("Density", &params.density, 1, 5);
      ImGui::EndGroup();

      ImGui::Separator();
      
      ImGui::BeginGroup();
      for (auto func : { getSphere, getPerlin, getPrism, getCubeConfigs }) {
        if (func == currentFunc)
          ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.45f, 0.6f, 0.6f));
        else
          ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.6f, 0.8f, 0.6f));

        if (ImGui::Button(func == getSphere ? "Sphere" : func == getPerlin ? "Perlin Noise" : func == getPrism ? "Prism" : "Cube Configs")) {
          params.showMarch = false;
          currentFunc = func;
          params.useTerrain = func == getPerlin;
          rerender(pointGrid, vertices, normals, points, indices, vertexbuffer, normalbuffer, pointbuffer, indexbuffer, currentFunc);
        }
        ImGui::SameLine();
        ImGui::PopStyleColor();
      }
      ImGui::EndGroup();

      if (currentFunc == getSphere) {
        ImGui::SliderFloat("Radius", &params.radius, 0.0f, 20.0f);
      }

      if (currentFunc == getPerlin) {
        ImGui::SliderFloat("X Offset", &params.xOffset, -100.0f, 100.0f);
        ImGui::SliderFloat("Y Offset", &params.yOffset, -100.0f, 100.0f);
        ImGui::SliderFloat("Z Offset", &params.zOffset, -100.0f, 100.0f);
      }

      if (currentFunc == getCubeConfigs) {
        if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
          if (params.configIndex > 0) {
            params.configIndex--;
            rerender(pointGrid, vertices, normals, points, indices, vertexbuffer, normalbuffer, pointbuffer, indexbuffer, currentFunc);
          }
        }
        ImGui::SameLine();
        ImGui::Text("Config %d", params.configIndex);
        ImGui::SameLine();
        if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
          if (params.configIndex < 14) {
            params.configIndex++;
            rerender(pointGrid, vertices, normals, points, indices, vertexbuffer, normalbuffer, pointbuffer, indexbuffer, currentFunc);
          }
        }
      }

      ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  
    glfwSwapBuffers(window);
    glfwPollEvents();
  } while (glfwWindowShouldClose(window) == 0);

  glDeleteBuffers(1, &vertexbuffer);
  glDeleteBuffers(1, &normalbuffer);
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &VertexArrayID);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwTerminate();

  return 0;
}
