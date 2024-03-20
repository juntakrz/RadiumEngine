#version 460
#define RE_MAXJOINTS 128

#extension GL_EXT_buffer_reference: require

#include "include/vertex.glsl"

// Per Vertex
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inJoint;
layout (location = 5) in vec4 inWeight;
layout (location = 6) in vec4 inColor0;

// Per Instance
layout (location = 7) in uvec4 inInstanceIndices;		// x - model, y - node, z - skin, w - material
layout (location = 8) in int inActorUID;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV0;
layout (location = 3) out vec2 outUV1;
layout (location = 4) out vec4 outColor0;
layout (location = 5) out vec4 outCurrentMVPPos;
layout (location = 6) out vec4 outPrevMVPPos;
layout (location = 7) flat out uint outMaterialIndex;
layout (location = 8) flat out int outActorUID;

void main() {
	const uint modelIndex = inInstanceIndices.x;
	const uint nodeIndex = inInstanceIndices.y;
	const uint skinIndex = inInstanceIndices.z;

	vec4 worldPos;
	vec4 prevWorldPos;

	if (node.block[nodeIndex].jointCount > 0.0) {
		mat4 skinMatrix = 
			inWeight.x * skin.block[skinIndex].jointMatrix[int(inJoint.x)] +
			inWeight.y * skin.block[skinIndex].jointMatrix[int(inJoint.y)] +
			inWeight.z * skin.block[skinIndex].jointMatrix[int(inJoint.z)] +
			inWeight.w * skin.block[skinIndex].jointMatrix[int(inJoint.w)];

		mat4 prevSkinMatrix = 
			inWeight.x * skin.block[skinIndex].prevJointMatrix[int(inJoint.x)] +
			inWeight.y * skin.block[skinIndex].prevJointMatrix[int(inJoint.y)] +
			inWeight.z * skin.block[skinIndex].prevJointMatrix[int(inJoint.z)] +
			inWeight.w * skin.block[skinIndex].prevJointMatrix[int(inJoint.w)];

		worldPos = model.block[modelIndex].matrix * node.block[nodeIndex].matrix * skinMatrix * vec4(inPos, 1.0);
		prevWorldPos = model.block[modelIndex].prevMatrix * node.block[nodeIndex].prevMatrix * prevSkinMatrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(model.block[modelIndex].matrix * node.block[nodeIndex].matrix * skinMatrix))) * inNormal);
	} else {
		worldPos = model.block[modelIndex].matrix * node.block[nodeIndex].matrix * vec4(inPos, 1.0);
		prevWorldPos = model.block[modelIndex].prevMatrix * node.block[nodeIndex].prevMatrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(model.block[modelIndex].matrix * node.block[nodeIndex].matrix))) * inNormal);
	}

	outWorldPos = worldPos.xyz / worldPos.w;

	outPrevMVPPos = vec4(prevWorldPos.xyz / prevWorldPos.w, 1.0);

	outUV0 = inUV0;
	outUV1 = inUV1;
	outColor0 = inColor0;

	// Storing unjittered vertex values for the velocity vector calculation
	outPrevMVPPos = scene.projection * scene.prevView * outPrevMVPPos;
	outCurrentMVPPos = scene.projection * scene.view * vec4(outWorldPos, 1.0);

	outMaterialIndex = inInstanceIndices.w;
	outActorUID = inActorUID;

	// Jittering an output vertex position for TAA history accumulation
	gl_Position = outCurrentMVPPos + vec4(scene.haltonJitter * outCurrentMVPPos.w, 0.0, 0.0);
}