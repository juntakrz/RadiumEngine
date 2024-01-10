#version 460
#extension GL_EXT_buffer_reference : require
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
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in vec4 inJoint;
layout(location = 5) in vec4 inWeight;
layout(location = 6) in vec4 inColor0;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV0;
layout(location = 3) out vec2 outUV1;
layout(location = 4) out vec4 outColor0;

struct Vertex {
	vec3 inPos;
	vec3 inNormal;
	vec2 inUV0;
	vec2 inUV1;
	vec4 inJoint;
	vec4 inWeight;
	vec4 inColor0;
};

layout(std430, buffer_reference) readonly buffer vertexBufferReference { 
	Vertex vertices[];
};

layout(push_constant) uniform vertexPCB {	
	vertexBufferReference vertexBuffer;
} pcb;

void main(){
	vec4 worldPos;

	if (node.jointCount > 0.0) {
//		mat4 skinMatrix = 
//			inWeight.x * skin.jointMatrix[int(inJoint.x)] +
//			inWeight.y * skin.jointMatrix[int(inJoint.y)] +
//			inWeight.z * skin.jointMatrix[int(inJoint.z)] +
//			inWeight.w * skin.jointMatrix[int(inJoint.w)];

		mat4 skinMatrix = 
			inWeight.x * skin.jointMatrix[int(pcb.vertexBuffer.vertices[gl_VertexIndex].inJoint.x)] +
			inWeight.y * skin.jointMatrix[int(pcb.vertexBuffer.vertices[gl_VertexIndex].inJoint.y)] +
			inWeight.z * skin.jointMatrix[int(pcb.vertexBuffer.vertices[gl_VertexIndex].inJoint.z)] +
			inWeight.w * skin.jointMatrix[int(pcb.vertexBuffer.vertices[gl_VertexIndex].inJoint.w)];

		//worldPos = model.rootMatrix * node.nodeMatrix * skinMatrix * vec4(inPos, 1.0);
		worldPos = model.rootMatrix * node.nodeMatrix * skinMatrix * vec4(pcb.vertexBuffer.vertices[gl_VertexIndex].inPos, 1.0);
		//outNormal = normalize(transpose(inverse(mat3(model.rootMatrix * node.nodeMatrix * skinMatrix))) * inNormal);
		outNormal = normalize(transpose(inverse(mat3(model.rootMatrix * node.nodeMatrix * skinMatrix))) * pcb.vertexBuffer.vertices[gl_VertexIndex].inNormal);
	} else {
		//worldPos = model.rootMatrix * node.nodeMatrix * vec4(inPos, 1.0);
		worldPos = model.rootMatrix * node.nodeMatrix * vec4(pcb.vertexBuffer.vertices[gl_VertexIndex].inPos, 1.0);
		//outNormal = normalize(transpose(inverse(mat3(model.rootMatrix * node.nodeMatrix))) * inNormal);
		outNormal = normalize(transpose(inverse(mat3(model.rootMatrix * node.nodeMatrix))) * pcb.vertexBuffer.vertices[gl_VertexIndex].inNormal);
	}

	outWorldPos = worldPos.xyz / worldPos.w;
	//outUV0 = inUV0;
	outUV0 = pcb.vertexBuffer.vertices[gl_VertexIndex].inUV0;
	//outUV1 = inUV1;
	outUV1 = pcb.vertexBuffer.vertices[gl_VertexIndex].inUV1;
	//outColor0 = inColor0;
	outColor0 = pcb.vertexBuffer.vertices[gl_VertexIndex].inColor0;

	gl_Position = scene.projection * scene.view * vec4(outWorldPos, 1.0);
}