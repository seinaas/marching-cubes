#version 330 core

in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;

out vec3 color;

uniform mat4 MV;
uniform vec3 LightPosition_worldspace;
uniform float height;
uniform bool useTerrain;

void main(){
  vec3 LightColor = vec3(1,1,1);
	float LightPower = 5.0f;
  

  float step1 = height * 0.0;
  float step2 = height * 0.4;
  float step3 = height * 0.6;
  float step4 = height;

  vec3 white = vec3(1.0,1.0,1.0);
  vec3 gray = vec3(0.2,0.2,0.2);
  vec3 brown = vec3(0.5,0.25,0.0);
  vec3 green = vec3(0.0,0.5,0.0);

  vec3 MaterialDiffuseColor;
  vec3 MaterialSpecularColor;

  if (useTerrain) {
    MaterialDiffuseColor = mix(green, brown, smoothstep(step1, step2, Position_worldspace.y));
    MaterialDiffuseColor = mix(MaterialDiffuseColor, gray, smoothstep(step2, step3, Position_worldspace.y));
    MaterialDiffuseColor = mix(MaterialDiffuseColor, white, smoothstep(step3, step4, Position_worldspace.y));

    MaterialSpecularColor = vec3(0,0,0);
  } else {
    MaterialDiffuseColor = vec3(0.3,0.7,0.7);
    MaterialSpecularColor = vec3(0.3,0.3,0.3);
  }

  // vec3 MaterialAmbientColor = vec3(1,1,1); //TODO: FOR DEBUGGING ONLY
  vec3 MaterialAmbientColor = vec3(0.5,0.5,0.5) * MaterialDiffuseColor;
	

  float dist = length( LightPosition_worldspace - Position_worldspace );

  vec3 n = normalize( Normal_cameraspace );
  vec3 l = normalize( LightDirection_cameraspace );

  float cosTheta = clamp( dot( n,l ), 0,1 );

  vec3 E = normalize(EyeDirection_cameraspace);

  vec3 R = reflect(-l,n);

  float cosAlpha = clamp( dot( E,R ), 0,1 );

  color = 
		// Ambient : simulates indirect lighting
		MaterialAmbientColor +
		// Diffuse : "color" of the object
		MaterialDiffuseColor * LightColor * LightPower * cosTheta / (1 + dist * 0.1 + dist * dist * 0.001) +
		// Specular : reflective highlight, like a mirror
		MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha,10) / (1 + dist * 0.1 + dist * dist * 0.001);
}