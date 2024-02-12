#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

#include "include/common.glsl"
#include "include/fragment.glsl"

#define TONEMAP_ACES

layout (location = 0) in vec2 inUV;

// Material bindings

layout (set = 2, binding = 0) uniform sampler2D samplers[];

layout (location = 0) out vec4 outColor;

void main() {
	vec3 baseColor = texture(samplers[material.samplerIndex[COLORMAP]], inUV).rgb;
	vec3 bloomColor = texture(samplers[material.samplerIndex[BLOOMMAP]], inUV).rgb;
	float exposure = lighting.exposure;

	float luminance = dot(baseColor, RGB_TO_LUM);
	float exposureAdjustment = luminance / (9.6 * lighting.averageLuminance);

	baseColor = pow(baseColor, vec3(2.2));
	baseColor *= vec3(exposureAdjustment / luminance);
	
	baseColor = pow(baseColor, vec3(1.0 / 2.2));

#ifdef TONEMAP_ACES
    baseColor = tonemapACES(baseColor);
#endif

#ifdef TONEMAP_AMD
    baseColor = tonemapAMD(baseColor);
#endif

#ifdef TONEMAP_REINHARDWP
    baseColor = tonemapReinhardWP(baseColor, 0.9);
#endif

	baseColor += bloomColor * 0.6;
	baseColor = vec3(1.0) - exp(-baseColor * exposure);

	outColor = vec4(baseColor, 1.0);
}