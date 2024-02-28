#version 460

#include "include/common.glsl"
#include "include/fragment.glsl"

#define MAX_TRANSPARENCY_LAYERS     4
#define MAX_TRANSPARENCY_THRESHOLD  0.75

struct transparencyNode{
	vec4 color;
	float depth;
	uint nextNodeIndex;
};

layout(location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 1, binding = 3) buffer transparencyLinkedListData {
	uint nodeCount;
	uint maxNodeCount;
};

layout (set = 1, binding = 4, r32ui) uniform coherent uimage2D headIndexImage; 

// Transparency linked list buffer
layout (set = 1, binding = 5) buffer transparencyLinkedListBuffer {
	transparencyNode nodes[];
};

layout (set = 2, binding = 0) uniform sampler2D samplers[];
layout (set = 2, binding = 0) uniform sampler2DArray arraySamplers[];

// The higher the value - the higher the mix of ambient occlusion
const float occlusionBias = 0.25;

vec3 getOcclusion(vec3 color) {
    float occlusionFactor = 0.0;
    const vec2 texelSize = 1.0 / vec2(textureSize(samplers[material.samplerIndex[EXTRAMAP1]], 0));
    const int range = 2;
    int count = 0;

    for (int x = -range; x < range + 1; x++) {
        for (int y = -range; y < range + 1; y++) {
            const vec2 offset = vec2(texelSize.x * x, texelSize.y * y);
            float sampledOcclusion = textureLod(samplers[material.samplerIndex[EXTRAMAP1]], inUV + offset, 0).r;
            occlusionFactor += sampledOcclusion;
            count++;
        }
    }

    occlusionFactor /= float(count);

    vec3 occludedColor = color * occlusionFactor;
    occlusionFactor = clamp(occlusionFactor - occlusionBias, 0.0, 1.0);
    return mix(occludedColor, color, occlusionFactor);
}

void main() {
    transparencyNode fragmentNodes[MAX_TRANSPARENCY_LAYERS];
    int count = 0;

    uint nodeIndex = imageLoad(headIndexImage, ivec2(gl_FragCoord.xy)).r;

    while (nodeIndex != 0xFFFFFFFF && count < MAX_TRANSPARENCY_LAYERS) {
        fragmentNodes[count] = nodes[nodeIndex];
        nodeIndex = fragmentNodes[count].nextNodeIndex;
        ++count;
    }
    
    // Sort nodes by depth
    for (uint i = 1; i < count; ++i) {
        transparencyNode currentNode = fragmentNodes[i];
        uint j = i;

        while (j > 0 && currentNode.depth > fragmentNodes[j - 1].depth) {
            fragmentNodes[j] = fragmentNodes[j - 1];
            --j;
        }

        fragmentNodes[j] = currentNode;
    }

    // Do transparency blending
    vec4 color = vec4(0.0);
    for (int i = 0; i < count; ++i) {
        color = mix(color, fragmentNodes[i].color, fragmentNodes[i].color.a);
    }

    // Get opaque PBR + Forward /Skybox results and blend with the final transparent color
    vec4 sampleColor = texelFetch(samplers[material.samplerIndex[COLORMAP]], ivec2(gl_FragCoord.xy), 0);

    // Adjust final mix power when alpha is above threshold to deal with potential transparency errors
    if (color.a > MAX_TRANSPARENCY_THRESHOLD) { 
        color.a = 1.0;
    }

    // Reset transparency node count
    nodeCount = 0;

    // Apply ambient occlusion
    sampleColor.rgb = getOcclusion(sampleColor.rgb);
    
    outColor = mix(sampleColor, color, color.a);
}