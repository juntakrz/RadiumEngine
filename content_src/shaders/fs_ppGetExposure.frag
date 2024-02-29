#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "include/common.glsl"
#include "include/fragment.glsl"

layout (location = 0) in vec2 inUV;

layout (location = 0) out float outColor;

void main() {
	vec3 baseColor = texture(samplers[material.samplerIndex[EXTRAMAP0]], inUV).rgb;
	outColor = dot(baseColor, RGB_TO_LUM);
}