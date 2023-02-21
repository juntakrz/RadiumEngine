#version 450

layout(binding = 0) uniform uboMVP{
	mat4 model;
	mat4 view;
	mat4 proj;
} inMVP;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outColor;

void main(){
	//gl_Position = inMVP.model * inMVP.view * inMVP.proj * vec4(inPos, 1.0);
	gl_Position = inMVP.model * vec4(inPos, 1.0);
	outColor = inColor;
}