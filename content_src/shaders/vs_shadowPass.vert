#version 460

#include "include/common.glsl"

struct NodeTransformBlock {
	mat4 nodeMatrix;
	float jointCount;
};

struct SkinTransformBlock {
	mat4 jointMatrix[MAXJOINTS];
};

layout(binding = 0) uniform UBOView {
	mat4 view;
	mat4 projection;
	vec3 cameraPos;
} scene;

layout (set = 1, binding = 0) buffer UBOMesh0 {
	mat4 rootMatrix[];
} model;

layout (set = 1, binding = 1) buffer UBOMesh1 {
	NodeTransformBlock block[];
} node;

layout (set = 1, binding = 2) buffer UBOMesh2 {
	SkinTransformBlock block[];
} skin;

// Per vertex
layout(location = 0) in vec3 inPos;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in vec4 inJoint;
layout(location = 5) in vec4 inWeight;
layout(location = 6) in vec4 inColor0;

// Per Instance
layout(location = 7) in uvec3 inInstanceTransformIndices;		// x - model, y - node, z - skin

layout(location = 2) out vec2 outUV0;

layout (push_constant) uniform Scene {
	uint cascadeIndex;
} pushBlock;

void main(){
	const uint modelIndex = inInstanceTransformIndices.x;
	const uint nodeIndex = inInstanceTransformIndices.y;
	const uint skinIndex = inInstanceTransformIndices.z;

	vec4 worldPos;

	if (node.block[nodeIndex].jointCount > 0.0) {
		mat4 skinMatrix = 
			inWeight.x * skin.block[skinIndex].jointMatrix[int(inJoint.x)] +
			inWeight.y * skin.block[skinIndex].jointMatrix[int(inJoint.y)] +
			inWeight.z * skin.block[skinIndex].jointMatrix[int(inJoint.z)] +
			inWeight.w * skin.block[skinIndex].jointMatrix[int(inJoint.w)];

		worldPos = model.rootMatrix[modelIndex] * node.block[nodeIndex].nodeMatrix * skinMatrix * vec4(inPos, 1.0);
	} else {
		worldPos = model.rootMatrix[modelIndex] * node.block[nodeIndex].nodeMatrix * vec4(inPos, 1.0);
	}

	worldPos = vec4(worldPos.xyz / worldPos.w, 1.0);
	outUV0 = inUV0;

	float FOVMultiplier = 1.0;

	for (uint i = 0; i < pushBlock.cascadeIndex; i++) {
		FOVMultiplier *= (i < 1) ? SHADOWFOVMULT : SHADOWFOVMULT * SHADOWFOVMULT;
	}

	mat4 newProjection = scene.projection;
	newProjection[0][0] *= FOVMultiplier;	
	newProjection[1][1] *= FOVMultiplier;

	gl_Position = newProjection * scene.view * worldPos;
}