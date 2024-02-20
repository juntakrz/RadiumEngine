#version 460

#include "include/common.glsl"
#include "include/fragment.glsl"

layout(location = 0) in vec3 inWorldPos;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;

struct AlphaLLNode{
	vec4 color;
	float alpha;
	uint nextNodeIndex;
};

// Scene bindings
layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
} scene;

// Transparency linked list buffer
layout (set = 1, binding = 3) buffer alphaLinkedListBuffer {
	AlphaLLNode nodes[];
};

// Counts how many nodes are stored and checks for node storage limits
layout (set = 1, binding = 4) buffer alphaLinkedListData {
	uint nodeCount;
	uint maxNodeCount;
};

//layout (set = 1, binding = 5, r32ui) uniform coherent uimage2D headIndexImage; 

// PBR Input bindings
layout (set = 2, binding = 0) uniform sampler2D samplers[];
layout (set = 2, binding = 0) uniform sampler2DArray arraySamplers[];

void main() {
	vec4 baseColor = vec4(0.0);

	int textureSet = getTextureSet(COLORMAP);
	if (textureSet > -1) {
		baseColor = texture(samplers[material.samplerIndex[COLORMAP]], textureSet == 0 ? inUV0 : inUV1);
	}

	
}