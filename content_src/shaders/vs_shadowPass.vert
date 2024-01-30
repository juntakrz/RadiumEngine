#version 460
#define RE_MAXJOINTS 128

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
	mat4 jointMatrix[RE_MAXJOINTS];
} skin;

layout(location = 0) in vec3 inPos;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in vec4 inJoint;
layout(location = 5) in vec4 inWeight;
layout(location = 6) in vec4 inColor0;

layout(location = 0) out vec3 outWorldPos;
layout(location = 2) out vec2 outUV0;
layout(location = 3) out vec2 outUV1;
layout(location = 4) out vec4 outColor0;

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

	outWorldPos = worldPos.xyz / worldPos.w;
	outUV0 = inUV0;
	outUV1 = inUV1;
	outColor0 = inColor0;

	float multiplier = 1.0;

	for (uint i = 0; i < pushBlock.cascadeIndex; i++) {
		multiplier *= 0.25;
	}

	mat4 newProjection = scene.projection;
	newProjection[0][0] *= multiplier;	
	newProjection[1][1] *= multiplier;

	gl_Position = newProjection * scene.view * vec4(outWorldPos, 1.0);
}