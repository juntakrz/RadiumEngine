#version 460
#define RE_MAXJOINTS 128

#extension GL_EXT_buffer_reference: require

struct NodeTransformBlock {
	mat4 nodeMatrix;
	float jointCount;
};

struct SkinTransformBlock {
	mat4 jointMatrix[RE_MAXJOINTS];
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

// Per Vertex
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in vec4 inJoint;
layout(location = 5) in vec4 inWeight;
layout(location = 6) in vec4 inColor0;

// Per Instance
layout(location = 7) in uvec3 inInstanceTransformIndices;		// x - model, y - node, z - skin

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV0;
layout(location = 3) out vec2 outUV1;
layout(location = 4) out vec4 outColor0;

void main(){
	const uint modelIndex = inInstanceTransformIndices.x;
	const uint nodeIndex = inInstanceTransformIndices.y;
	const uint skinIndex = inInstanceTransformIndices.z;

	vec4 worldPos;

	if (node.block[nodeIndex].jointCount > 0.0) {
//		mat4 skinMatrix = 
//			inWeight.x * scene.skinTransformBuffer.block[skinIndex].jointMatrix[int(inJoint.x)] +
//			inWeight.y * scene.skinTransformBuffer.block[skinIndex].jointMatrix[int(inJoint.y)] +
//			inWeight.z * scene.skinTransformBuffer.block[skinIndex].jointMatrix[int(inJoint.z)] +
//			inWeight.w * scene.skinTransformBuffer.block[skinIndex].jointMatrix[int(inJoint.w)];

		mat4 skinMatrix = 
			inWeight.x * skin.block[skinIndex].jointMatrix[int(inJoint.x)] +
			inWeight.y * skin.block[skinIndex].jointMatrix[int(inJoint.y)] +
			inWeight.z * skin.block[skinIndex].jointMatrix[int(inJoint.z)] +
			inWeight.w * skin.block[skinIndex].jointMatrix[int(inJoint.w)];

		worldPos = model.rootMatrix[modelIndex] * node.block[nodeIndex].nodeMatrix * skinMatrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(model.rootMatrix[modelIndex] * node.block[nodeIndex].nodeMatrix * skinMatrix))) * inNormal);
	} else {
		worldPos = model.rootMatrix[modelIndex] * node.block[nodeIndex].nodeMatrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(model.rootMatrix[modelIndex] * node.block[nodeIndex].nodeMatrix))) * inNormal);
	}

//		mat4 skinMatrix = 
//			inWeight.x * skin.jointMatrix[int(inJoint.x)] +
//			inWeight.y * skin.jointMatrix[int(inJoint.y)] +
//			inWeight.z * skin.jointMatrix[int(inJoint.z)] +
//			inWeight.w * skin.jointMatrix[int(inJoint.w)];

//		worldPos = model.rootMatrix * node.nodeMatrix * skinMatrix * vec4(inPos, 1.0);
//		outNormal = normalize(transpose(inverse(mat3(model.rootMatrix * node.nodeMatrix * skinMatrix))) * inNormal);
//	} else {
//		worldPos = model.rootMatrix * node.nodeMatrix * vec4(inPos, 1.0);
//		outNormal = normalize(transpose(inverse(mat3(model.rootMatrix * node.nodeMatrix))) * inNormal);
//	}

	outWorldPos = worldPos.xyz / worldPos.w;

	outUV0 = inUV0;
	outUV1 = inUV1;
	outColor0 = inColor0;

	gl_Position = scene.projection * scene.view * vec4(outWorldPos, 1.0);
}