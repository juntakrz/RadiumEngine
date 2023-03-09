#version 450

layout(binding = 0) uniform MVP{
	mat4 model;
	mat4 view;
	mat4 projection;
	vec3 cameraPos;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord0;
layout(location = 2) in vec2 inTexCoord1;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec4 inColor0;
layout(location = 5) in vec4 inJoint;
layout(location = 6) in vec4 inWeight;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord0;
layout(location = 3) out vec2 outTexCoord1;
layout(location = 4) out vec4 outColor0;

void main(){
	outColor0 = inColor0;

	vec4 locPos;
	if (node.jointCount > 0.0) {
		// Mesh is skinned
		mat4 skinMat = 
			inWeight0.x * node.jointMatrix[int(inJoint.x)] +
			inWeight0.y * node.jointMatrix[int(inJoint.y)] +
			inWeight0.z * node.jointMatrix[int(inJoint.z)] +
			inWeight0.w * node.jointMatrix[int(inJoint.w)];

		locPos = ubo.model * node.matrix * skinMatrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(ubo.model * node.matrix * skinMatrix))) * inNormal);
	} else {
		locPos = ubo.model * node.matrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(ubo.model * node.matrix))) * inNormal);
	}

	locPos.y = -locPos.y;
	outWorldPos = locPos.xyz / locPos.w;
	outTexCoord0 = inTexCoord0;
	outTexCoord1 = inTexCoord1;
	gl_Position =  ubo.projection * ubo.view * vec4(outWorldPos, 1.0);
}