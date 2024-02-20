#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "include/common.glsl"
#include "include/fragment.glsl"

layout (location = 2) in vec2 inUV0;

void main() {
	float alpha = texture(samplers[material.samplerIndex[COLORMAP]], inUV0).a;
	
	if (alpha < 0.5) {
		discard;
	}
}