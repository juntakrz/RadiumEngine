#version 460

#include "include/common.glsl"
#include "include/fragment.glsl"

#define MAX_TRANSPARENCY_LAYERS     4
#define MAX_TRANSPARENCY_THRESHOLD  0.75

struct transparencyNode{
	vec4 color;
	float depth;
    int actorUID;
	uint nextNodeIndex;
};

layout(location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 1, binding = 3) buffer transparencyLinkedListData {
	uint nodeCount;
	uint maxNodeCount;

    // Since this is the last instance draw pass - it can also
    // be used to retrieve the required UID from the raycast map
    int raycastedUID;
};

layout (set = 1, binding = 4, r32ui) uniform coherent uimage2D headIndexImage; 

// Transparency linked list buffer
layout (set = 1, binding = 5) buffer transparencyLinkedListBuffer {
	transparencyNode nodes[];
};

layout (set = 2, binding = 1, r32ui) uniform uimage2D samplersInt[];

// The higher the value - the higher the mix of ambient occlusion
const float occlusionBias = 0.25;

vec3 gaussianBlur(vec3 color) {
    float occlusionFactor = 0.0;
    const vec2 texelSize = 1.0 / vec2(textureSize(samplers[material.samplerIndex[EXTRAMAP1]], 0));
    const float blurRadius = 2.0;
    float w0 = 0.5135 / pow(blurRadius, 0.96);
    float radiusSquared = blurRadius * blurRadius;

    for (float x = -blurRadius; x <= blurRadius; x++) {
        vec2 offset = vec2(x * texelSize.x, 0.0);
        float w = w0 * exp((-x * x) / (2.0 * radiusSquared));
        occlusionFactor +=  texture(samplers[material.samplerIndex[EXTRAMAP1]], inUV + offset).r * w;
    }

    vec3 occludedColor = color * occlusionFactor;
    occlusionFactor = clamp(occlusionFactor - occlusionBias, 0.0, 1.0);
    return mix(occludedColor, color, occlusionFactor);
}


vec3 getOcclusion(vec3 color) {
    float occlusionFactor = 0.0;
    const vec2 texelSize = 1.0 / vec2(textureSize(samplers[material.samplerIndex[EXTRAMAP1]], 0));
    const int range = 2;
    int count = 0;

    for (int x = -range; x < range + 1; x++) {
        for (int y = -range; y < range + 1; y++) {
            const vec2 offset = vec2(texelSize.x * x, texelSize.y * y);
            float sampledOcclusion = texture(samplers[material.samplerIndex[EXTRAMAP1]], inUV + offset).r;
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
    //float testAlpha = 0.0;

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
    if (lighting.aoMode != AO_NONE) {
        float occlusionFactor = texture(samplers[material.samplerIndex[EXTRAMAP1]], inUV).r;
        vec3 occludedColor = sampleColor.rgb * occlusionFactor;
        occlusionFactor = clamp(occlusionFactor - occlusionBias, 0.0, 1.0);
        sampleColor.rgb = mix(occludedColor, sampleColor.rgb, occlusionFactor);
    }
    
    outColor = mix(sampleColor, color, color.a);

    // Perform raycasting as it's the last instance rendering pass
    if (scene.raycastTarget.x > -1 && scene.raycastTarget.y > -1 && ivec2(gl_FragCoord.xy) == scene.raycastTarget) { 
        if (count > 0) {
            raycastedUID = fragmentNodes[0].actorUID;
        } else {
            raycastedUID = int(imageLoad(samplersInt[material.samplerIndex[EXTRAMAP2]], ivec2(gl_FragCoord.xy)).r) - 1;
        }
    } else if (ivec2(gl_FragCoord.xy) == ivec2(0, 0) && scene.raycastTarget.x < 0 && scene.raycastTarget.y < 0) {
        raycastedUID = -1;
    }
}