#version 450

layout(binding = 0) uniform MVP{
	mat4 model;
	mat4 view;
	mat4 proj;
} inMVP;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord0;

layout(location = 0) out vec2 outTexCoord0;

void main(){
	gl_Position = inMVP.proj * inMVP.view * inMVP.model * vec4(inPos, 1.0);
	outTexCoord0 = inTexCoord0;
}