#version 450

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;

// Material bindings

layout (set = 2, binding = 0) uniform sampler2D colorMap;

layout (location = 0) out vec4 outColor;

void main() {
	vec3 color = texture(colorMap, inUV0).rgb;
	outColor = vec4(color, 1.0);
}