#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "include/common.glsl"
#include "include/fragment.glsl"

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main() {
	// baseColorTextureSet is an index into the target mip level, upsampling will use the lower resolution one
	uint textureIndex = material.samplerIndex[NORMALMAP];
	int LOD = material.textureSets + 1;

	ivec2 resolution = textureSize(samplers[textureIndex], LOD);
	vec2 texelSize = 1.0 / vec2(resolution); 

    /* Take 9 samples around the center (current) texel
    a - b - c
    d - e - f
    g - h - i */
    vec3 a = textureLod(samplers[textureIndex], vec2(inUV.x - texelSize.x, inUV.y - texelSize.y), LOD).rgb;
    vec3 b = textureLod(samplers[textureIndex], vec2(inUV.x, inUV.y - texelSize.y), LOD).rgb;
    vec3 c = textureLod(samplers[textureIndex], vec2(inUV.x + texelSize.x, inUV.y - texelSize.y), LOD).rgb;

    vec3 d = textureLod(samplers[textureIndex], vec2(inUV.x - texelSize.x, inUV.y), LOD).rgb;
    vec3 e = textureLod(samplers[textureIndex], vec2(inUV.x, inUV.y), LOD).rgb;
    vec3 f = textureLod(samplers[textureIndex], vec2(inUV.x + texelSize.x, inUV.y), LOD).rgb;

    vec3 g = textureLod(samplers[textureIndex], vec2(inUV.x - texelSize.x, inUV.y + texelSize.y), LOD).rgb;
    vec3 h = textureLod(samplers[textureIndex], vec2(inUV.x, inUV.y + texelSize.y), LOD).rgb;
    vec3 i = textureLod(samplers[textureIndex], vec2(inUV.x + texelSize.x, inUV.y + texelSize.y), LOD).rgb;

    // Additional sample from yet not processed downsampled mip level for glow effect
    e += textureLod(samplers[textureIndex], vec2(inUV.x, inUV.y), LOD - 1).rgb;

	/* Apply weighted distribution, by using a 3x3 tent filter:
    1		| 1 2 1 |
    --		| 2 4 2 |
    16		| 1 2 1 | */
    vec3 color = e * 4.0;
    color += a+c+g+i;
    color += (b+d+f+h) * 2.0;
	color *= 1.0 / 16.0;

    //color += textureLod(samplers[textureIndex], vec2(inUV.x, inUV.y), LOD - 1).rgb * 0.5;

	outColor = vec4(color, 1.0);
}