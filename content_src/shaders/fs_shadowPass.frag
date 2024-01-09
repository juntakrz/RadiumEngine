#version 460

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;

// material bindings
layout (set = 2, binding = 0) uniform sampler2D colorMap;

void main() {
	float alpha = texture(colorMap, inUV0).a;
	
	if (alpha < 0.5) {
		discard;
	}
}