#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

#include "include/common.glsl"
#include "include/fragment.glsl"

layout (location = 0) in vec2 inUV;

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

// Material bindings

layout (set = 2, binding = 0) uniform sampler2D samplers[];

layout (location = 0) out vec4 outColor;

void main() {
	// baseColorTextureSet is used as an index into the original PBR texture or the mip level of the downsampled texture
	uint textureIndex = (material.baseColorTextureSet == 0) ? material.samplerIndex[COLORMAP] : material.samplerIndex[NORMALMAP];
	int LOD = (material.baseColorTextureSet == 0) ? 0 : material.baseColorTextureSet - 1;

	ivec2 resolution = textureSize(samplers[textureIndex], LOD);
	vec2 texelSize = 1.0 / vec2(resolution); 

    /* Take 13 samples around the center (current) texel
    a - b - c
    - j - k -
    d - e - f
    - l - m -
    g - h - i
    When linear filtering is used during downsampling - each sample gets a sum of 4 surrounding pixels */
    vec3 a = textureLod(samplers[textureIndex], vec2(inUV.x - 2.0 * texelSize.x, inUV.y - 2.0 * texelSize.y), LOD).rgb;
    vec3 b = textureLod(samplers[textureIndex], vec2(inUV.x, inUV.y - 2.0 * texelSize.y), LOD).rgb;
    vec3 c = textureLod(samplers[textureIndex], vec2(inUV.x + 2.0 * texelSize.x, inUV.y - 2.0 * texelSize.y), LOD).rgb;

    vec3 d = textureLod(samplers[textureIndex], vec2(inUV.x - 2.0 * texelSize.x, inUV.y), LOD).rgb;
    vec3 e = textureLod(samplers[textureIndex], vec2(inUV.x, inUV.y), LOD).rgb;
    vec3 f = textureLod(samplers[textureIndex], vec2(inUV.x + 2.0 * texelSize.x, inUV.y), LOD).rgb;

    vec3 g = textureLod(samplers[textureIndex], vec2(inUV.x - 2.0 * texelSize.x, inUV.y + 2.0 * texelSize.y), LOD).rgb;
    vec3 h = textureLod(samplers[textureIndex], vec2(inUV.x, inUV.y + 2.0 * texelSize.y), LOD).rgb;
    vec3 i = textureLod(samplers[textureIndex], vec2(inUV.x + 2.0 * texelSize.x, inUV.y + 2.0 * texelSize.y), LOD).rgb;

    vec3 j = textureLod(samplers[textureIndex], vec2(inUV.x - texelSize.x, inUV.y - texelSize.y), LOD).rgb;
    vec3 k = textureLod(samplers[textureIndex], vec2(inUV.x + texelSize.x, inUV.y - texelSize.y), LOD).rgb;
    vec3 l = textureLod(samplers[textureIndex], vec2(inUV.x - texelSize.x, inUV.y + texelSize.y), LOD).rgb;
    vec3 m = textureLod(samplers[textureIndex], vec2(inUV.x + texelSize.x, inUV.y + texelSize.y), LOD).rgb;

    /* Apply weighted distribution:
    0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    a,b,d,e * 0.125
    b,c,e,f * 0.125
    d,e,g,h * 0.125
    e,f,h,i * 0.125
    j,k,l,m * 0.5
    This shows 5 square areas that are being sampled. But some of them overlap,
    so to have an energy preserving downsample we need to make some adjustments.
    The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    contribute 0.5 to the final color output. The code below is written
    to effectively yield this sum. We get: 0.125 * 5 + 0.03125 * 4 + 0.0625 * 4 = 1 */

    vec3 color = e * 0.125;
    color += (a + c + g + i) * 0.03125;
    color += (b + d + f + h) * 0.0625;
    color += (j + k + l + m) * 0.125;

    if (material.baseColorTextureSet == 0) {
        color -= BLOOMTHRESHOLD;
        color = max(color, 0.0);
    }

	outColor = vec4(color, 1.0);
}