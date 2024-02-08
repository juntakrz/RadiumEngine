#version 460

#include "include/common.glsl"

layout(binding = 0) uniform UBOView {
	mat4 view;
	mat4 projection;
	vec3 cameraPos;
} scene;

layout (set = 1, binding = 0) uniform UBOMesh0 {
	mat4 rootMatrix;
} model;

layout (set = 1, binding = 1) uniform UBOMesh1 {
	mat4 nodeMatrix;
	float jointCount;
} node;

layout (set = 1, binding = 2) uniform UBOMesh2 {
	mat4 jointMatrix[MAXJOINTS];
} skin;

layout(location = 0) in vec3 inPos;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in vec4 inJoint;
layout(location = 5) in vec4 inWeight;
layout(location = 6) in vec4 inColor0;

layout(location = 2) out vec2 outUV0;

layout (push_constant) uniform Scene {
	uint cascadeIndex;
} pushBlock;

void main(){
	vec4 worldPos;

	if (node.jointCount > 0.0) {
		mat4 skinMatrix = 
			inWeight.x * skin.jointMatrix[int(inJoint.x)] +
			inWeight.y * skin.jointMatrix[int(inJoint.y)] +
			inWeight.z * skin.jointMatrix[int(inJoint.z)] +
			inWeight.w * skin.jointMatrix[int(inJoint.w)];

		worldPos = model.rootMatrix * node.nodeMatrix * skinMatrix * vec4(inPos, 1.0);
	} else {
		worldPos = model.rootMatrix * node.nodeMatrix * vec4(inPos, 1.0);
	}

	worldPos = vec4(worldPos.xyz / worldPos.w, 1.0);
	outUV0 = inUV0;

	float FOVMultiplier = 1.0;

	for (uint i = 0; i < pushBlock.cascadeIndex; i++) {
		FOVMultiplier *= SHADOWFOVMULT;
	}

	mat4 newProjection = scene.projection;
	newProjection[0][0] *= FOVMultiplier;	
	newProjection[1][1] *= FOVMultiplier;

	gl_Position = newProjection * scene.view * worldPos;
}