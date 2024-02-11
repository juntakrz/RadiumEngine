#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

#include "include/common.glsl"
#include "include/fragment.glsl"

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
	float exposure;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
} lighting;

layout (location = 0) out vec4 outColor;

void main() {
	vec3 baseColor = texture(samplers[material.samplerIndex[COLORMAP]], inUV).rgb;
	vec3 bloomColor = texture(samplers[material.samplerIndex[BLOOMMAP]], inUV).rgb;

	float luminance = dot(baseColor, vec3(0.2126, 0.7152, 0.0722));
	float exposureAdjustment = (luminance / lighting.exposure) * 1.5;

	baseColor *= clamp(exposureAdjustment, 0.1, 1.0);

	baseColor += bloomColor * 1.2;

	outColor = vec4(baseColor, 1.0);
}