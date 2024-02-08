#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "include/common.glsl"
#include "include/fragment.glsl"

layout (location = 0) in vec3 inTexCoord;
layout (location = 1) in vec4 inColor0;

layout (location = 0) out vec4 outColor;

// Material bindings
layout (set = 2, binding = 0) uniform samplerCube samplers[];

void main() {
	vec3 color = texture(samplers[material.samplerIndex[COLORMAP]], inTexCoord).rgb * inColor0.rgb;
	color += material.emissiveIntensity.rgb;
	outColor = vec4(color, 1.0);
}