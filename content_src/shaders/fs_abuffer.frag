#version 460

#include "include/common.glsl"
#include "include/fragment.glsl"

struct transparencyNode{
	vec4 color;
	float depth;
	uint nextNodeIndex;
};

layout (early_fragment_tests) in;

layout(location = 0) in vec3 inWorldPos;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;

// Scene bindings
layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
} scene;

// Counts how many nodes are stored and checks for node storage limits
layout (set = 1, binding = 3) buffer transparencyLinkedListData {
	uint nodeCount;
	uint maxNodeCount;
};

layout (set = 1, binding = 4, r32ui) uniform coherent uimage2D headIndexImage; 

// Transparency linked list buffer
layout (set = 1, binding = 5) buffer transparencyLinkedListBuffer {
	transparencyNode nodes[];
};

void main() {
	vec4 baseColor = vec4(0.0);

	int textureSet = getTextureSet(COLORMAP);
	if (textureSet > -1) {
		baseColor = texture(samplers[material.samplerIndex[COLORMAP]], textureSet == 0 ? inUV0 : inUV1);
	}

	// Increase the node count
    uint nodeIndex = atomicAdd(nodeCount, 1);

    // Check if LinkedListSBO is full
    if (nodeIndex < maxNodeCount)
    {
        // Exchange new head index and previous head index
        uint prevHeadIndex = imageAtomicExchange(headIndexImage, ivec2(gl_FragCoord.xy), nodeIndex);

        // Store node data
        nodes[nodeIndex].color = baseColor;
        nodes[nodeIndex].depth = gl_FragCoord.z;
        nodes[nodeIndex].nextNodeIndex = prevHeadIndex;
    }
}