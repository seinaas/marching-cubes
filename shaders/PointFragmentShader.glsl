#version 330 core

in float isInside;

out vec3 color;

void main(){
  vec2 pos = gl_PointCoord - vec2(0.5);
  if(length(pos) > 0.5)
    discard;
  if(isInside > 0.0)
    color = vec3(0, 1, 0);
  else
    color = vec3(1, 0, 0);
}