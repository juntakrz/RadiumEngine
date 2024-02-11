#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "include/common.glsl"
#include "include/fragment.glsl"

layout (location = 0) in vec2 inUV;

// Material bindings

layout (set = 2, binding = 0) uniform sampler2D samplers[];

layout (location = 0) out float outColor;

void main() {
	vec3 baseColor = texture(samplers[material.samplerIndex[COLORMAP]], inUV).rgb;
	outColor = dot(baseColor, vec3(0.2126, 0.7152, 0.0722));
}