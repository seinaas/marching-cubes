#version 330 core

layout(location = 3) in vec4 vertexPosition_modelspace;
// layout(location = 1) in vec3 vertexColor;

uniform mat4 MVP;
uniform mat4 V;
uniform mat4 M;

out float isInside;

void main(){
  gl_Position = MVP * vec4(vertexPosition_modelspace.xyz,1);
	
	// Vector that goes from the vertex to the camera, in camera space.
	// In camera space, the camera is at the origin (0,0,0).
	vec3 vertexPosition_cameraspace = ( V * M * vec4(vertexPosition_modelspace.xyz,1)).xyz;

	isInside = vertexPosition_modelspace.w;
	// Keep point size constant
	gl_PointSize = 100.0 * (1.0 / -vertexPosition_cameraspace.z);
}