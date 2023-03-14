#version 450
#define RE_MAXJOINTS 128

layout(binding = 0) uniform UBOView {
	mat4 view;
	mat4 projection;
	vec3 cameraPos;
} scene;

layout (set = 1, binding = 0) uniform UBONode {
	mat4 rootMatrix;
	mat4 nodeMatrix;
	mat4 jointMatrix[RE_MAXJOINTS];
	float jointCount;
} mesh;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord0;
layout(location = 2) in vec2 inTexCoord1;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec4 inColor0;
layout(location = 5) in vec4 inJoint;
layout(location = 6) in vec4 inWeight;
layout(location = 7) in vec3 inTangent;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord0;
layout(location = 3) out vec2 outTexCoord1;
layout(location = 4) out vec4 outColor0;
layout(location = 5) out vec3 outTangent;

void main(){
	vec4 worldPos;

	if (mesh.jointCount > 0.0) {
		mat4 skinMatrix = 
			inWeight.x * mesh.jointMatrix[int(inJoint.x)] +
			inWeight.y * mesh.jointMatrix[int(inJoint.y)] +
			inWeight.z * mesh.jointMatrix[int(inJoint.z)] +
			inWeight.w * mesh.jointMatrix[int(inJoint.w)];

		worldPos = mesh.rootMatrix * mesh.nodeMatrix * skinMatrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(mesh.rootMatrix * mesh.nodeMatrix * skinMatrix))) * inNormal);
	} else {
		worldPos = mesh.rootMatrix * mesh.nodeMatrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(mesh.rootMatrix * mesh.nodeMatrix))) * inNormal);
	}
	
	outWorldPos = worldPos.xyz / worldPos.w;
	outTexCoord0 = inTexCoord0;
	outTexCoord1 = inTexCoord1;
	outColor0 = inColor0;
	outTangent = inTangent;

	gl_Position = scene.projection * scene.view * vec4(outWorldPos, 1.0);
}