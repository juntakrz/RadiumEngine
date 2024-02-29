#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "include/common.glsl"
#include "include/fragment.glsl"

layout (location = 0) in vec3 inTexCoord;
layout (location = 1) in vec4 inColor0;

// Per Instance
layout (location = 7) flat in uint inMaterialIndex;

layout (location = 0) out vec4 outColor;

void main() {
	vec3 color = texture(cubeSamplers[materialBlocks[inMaterialIndex].samplerIndex[COLORMAP]], inTexCoord).rgb * inColor0.rgb;
	color += material.glowColor.rgb;
	outColor = vec4(color, 1.0);
}