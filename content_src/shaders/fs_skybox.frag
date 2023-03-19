#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 inTexCoord;
layout (location = 1) in vec4 inColor0;

// Scene bindings

layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
} scene;

// Material bindings

layout (set = 2, binding = 0) uniform samplerCube colorMap;

layout (location = 0) out vec4 outColor;

void main() {
	vec3 color = texture(colorMap, inTexCoord).rgb * inColor0.rgb;
	outColor = vec4(color, 1.0);
}