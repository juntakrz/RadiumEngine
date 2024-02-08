#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "include/common.glsl"
#include "include/fragment.glsl"

layout (location = 0) in vec2 inUV;

// Material bindings

layout (set = 2, binding = 0) uniform sampler2D samplers[];

layout (location = 0) out vec4 outColor;

void main() {
	vec3 baseColor = texture(samplers[material.samplerIndex[COLORMAP]], inUV).rgb;
	vec3 bloomColor = texture(samplers[material.samplerIndex[BLOOMMAP]], inUV).rgb;
	baseColor = mix(baseColor, bloomColor, 0.15);
	outColor = vec4(baseColor, 1.0);
}