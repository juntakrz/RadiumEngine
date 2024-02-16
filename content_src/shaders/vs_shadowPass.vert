#version 460

#include "include/common.glsl"
#include "include/vertex.glsl"

// Per vertex
layout (location = 0) in vec3 inPos;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inJoint;
layout (location = 5) in vec4 inWeight;
layout (location = 6) in vec4 inColor0;

// Per Instance
layout (location = 7) in uvec4 inInstanceIndices;		// x - model, y - node, z - skin, w - material

layout (location = 2) out vec2 outUV0;
layout (location = 7) flat out uint outMaterialIndex;

layout (push_constant) uniform Scene {
	uint cascadeIndex;
} pushBlock;

void main(){
	const uint modelIndex = inInstanceIndices.x;
	const uint nodeIndex = inInstanceIndices.y;
	const uint skinIndex = inInstanceIndices.z;

	vec4 worldPos;

	if (node.block[nodeIndex].jointCount > 0.0) {
		mat4 skinMatrix = 
			inWeight.x * skin.block[skinIndex].jointMatrix[int(inJoint.x)] +
			inWeight.y * skin.block[skinIndex].jointMatrix[int(inJoint.y)] +
			inWeight.z * skin.block[skinIndex].jointMatrix[int(inJoint.z)] +
			inWeight.w * skin.block[skinIndex].jointMatrix[int(inJoint.w)];

		worldPos = model.block[modelIndex].matrix * node.block[nodeIndex].matrix * skinMatrix * vec4(inPos, 1.0);
	} else {
		worldPos = model.block[modelIndex].matrix * node.block[nodeIndex].matrix * vec4(inPos, 1.0);
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

	outMaterialIndex = inInstanceIndices.w;

	gl_Position = newProjection * scene.view * worldPos;
}