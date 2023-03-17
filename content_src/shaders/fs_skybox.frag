#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 inUV0;
layout (location = 1) in vec4 inColor0;

// Scene bindings

layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
} scene;

// Material bindings

layout (set = 2, binding = 0) uniform sampler2D colorMap;

layout (location = 0) out vec4 outColor;

void main() {
	vec3 color = texture(colorMap, inUV0).rgb * inColor0.rgb;
	outColor = vec4(color, 1.0);
}