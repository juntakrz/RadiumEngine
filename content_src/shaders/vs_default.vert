#version 450
#define RE_MAXJOINTS 128

layout(binding = 0) uniform UBOView {
	mat4 world;
	mat4 view;
	mat4 projection;
	vec3 cameraPos;
} scene;

layout (set = 2, binding = 0) uniform UBONode {
	mat4 matrix;
	mat4 jointMatrix[RE_MAXJOINTS];
	float jointCount;
} node;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord0;
layout(location = 2) in vec2 inTexCoord1;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec4 inColor0;
layout(location = 5) in vec4 inJoint;
layout(location = 6) in vec4 inWeight;
layout(location = 7) in vec3 inT;
layout(location = 8) in vec3 inB;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord0;
layout(location = 3) out vec2 outTexCoord1;
layout(location = 4) out vec4 outColor0;
layout(location = 5) out vec3 outT;
layout(location = 6) out vec3 outB;

void main(){
	vec4 worldPos;

	if (node.jointCount > 0.0) {
		mat4 skinMatrix = 
			inWeight.x * node.jointMatrix[int(inJoint.x)] +
			inWeight.y * node.jointMatrix[int(inJoint.y)] +
			inWeight.z * node.jointMatrix[int(inJoint.z)] +
			inWeight.w * node.jointMatrix[int(inJoint.w)];

		worldPos = scene.world * node.matrix * skinMatrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(scene.world * node.matrix * skinMatrix))) * inNormal);
	} else {
		worldPos = scene.world * node.matrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(scene.world * node.matrix))) * inNormal);
	}
	
	outWorldPos = worldPos.xyz / worldPos.w;
	outTexCoord0 = inTexCoord0;
	outTexCoord1 = inTexCoord1;
	outColor0 = inColor0;

	outT = inT;
	outB = inB;

	gl_Position =  scene.projection * scene.view * vec4(outWorldPos, 1.0);
}