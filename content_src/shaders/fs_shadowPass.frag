#version 450

#define SHADOW_THRESHOLD 0.9

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;

// material bindings
layout (set = 2, binding = 0) uniform sampler2D colorMap;

void main() {
	vec4 baseColor = texture(colorMap, inUV0);
	
	if (baseColor.a < SHADOW_THRESHOLD) {
		discard;
	}
}