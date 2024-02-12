#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

#include "include/common.glsl"
#include "include/fragment.glsl"

#define TONEMAP_ACES

layout (location = 0) in vec2 inUV;

// Material bindings

layout (set = 2, binding = 0) uniform sampler2D samplers[];

layout (std430, set = 0, binding = 1) uniform UBOLighting {
	vec4 lightLocations[MAXLIGHTS];
    vec4 lightColor[MAXLIGHTS];
	mat4 lightViews[MAXSHADOWCASTERS];
	mat4 lightOrthoMatrix;
	uint samplerIndex[MAXSHADOWCASTERS];
	uint lightCount;
	float averageSceneLuminance;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
} lighting;

layout (location = 0) out vec4 outColor;

void main() {
	vec3 baseColor = texture(samplers[material.samplerIndex[COLORMAP]], inUV).rgb;
	vec3 bloomColor = texture(samplers[material.samplerIndex[BLOOMMAP]], inUV).rgb;

	float luminance = dot(baseColor, vec3(0.2126, 0.7152, 0.0722));
	float exposureAdjustment = luminance / (9.6 * lighting.averageSceneLuminance);
	baseColor *= vec3(exposureAdjustment / luminance);

#ifdef TONEMAP_ACES
    //baseColor += bloomColor * 0.5;
    baseColor = tonemapACES(baseColor);
    baseColor = vec3(1.0) - exp(-baseColor * 9.6);
	baseColor += bloomColor * 1.2;
#endif

#ifdef TONEMAP_AMD
    baseColor = vec3(1.0) - exp(-baseColor * 4.5);
    baseColor = AMDTonemapper(baseColor);
	baseColor += bloomColor;
#endif

#ifdef TONEMAP_REINHARD
    const float whitePoint = 0.5;
	vec3 numerator = baseColor * (1.0 + (baseColor / vec3(whitePoint * whitePoint)));
    baseColor = numerator / (1.0 + baseColor);
    baseColor = vec3(1.0) - exp(-baseColor * 4.5);
	baseColor += bloomColor;
#endif

#ifdef TONEMAP_NONE
	baseColor = vec3(1.0) - exp(-baseColor * 4.5);
	baseColor += bloomColor;
#endif

	outColor = vec4(baseColor, 1.0);
}