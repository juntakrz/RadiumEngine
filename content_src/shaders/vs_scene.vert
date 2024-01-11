#version 460
#extension GL_EXT_buffer_reference : require
#define RE_MAXJOINTS 128

struct Vertex {
	vec3 inPos;
	vec3 inNormal;
	vec2 inUV0;
	vec2 inUV1;
	vec4 inJoint;
	vec4 inWeight;
	vec4 inColor0;
};

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

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV0;
layout(location = 3) out vec2 outUV1;
layout(location = 4) out vec4 outColor0;

layout(std430, buffer_reference) readonly buffer vertexBufferReference { 
	Vertex vertices[];
};

layout(push_constant) uniform vertexPCB {	
	vertexBufferReference vertexBuffer;
} pcb;

void main(){
	vec4 worldPos;
	Vertex vertexData = pcb.vertexBuffer.vertices[gl_VertexIndex];

	if (node.jointCount > 0.0) {
		mat4 skinMatrix = 
			vertexData.inWeight.x * skin.jointMatrix[int(vertexData.inJoint.x)] +
			vertexData.inWeight.y * skin.jointMatrix[int(vertexData.inJoint.y)] +
			vertexData.inWeight.z * skin.jointMatrix[int(vertexData.inJoint.z)] +
			vertexData.inWeight.w * skin.jointMatrix[int(vertexData.inJoint.w)];

		worldPos = model.rootMatrix * node.nodeMatrix * skinMatrix * vec4(vertexData.inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(model.rootMatrix * node.nodeMatrix * skinMatrix))) * vertexData.inNormal);
	} else {
		worldPos = model.rootMatrix * node.nodeMatrix * vec4(vertexData.inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(model.rootMatrix * node.nodeMatrix))) * vertexData.inNormal);
	}

	outWorldPos = worldPos.xyz / worldPos.w;
	outUV0 = vertexData.inUV0;
	outUV1 = vertexData.inUV1;
	outColor0 = vertexData.inColor0;

	gl_Position = scene.projection * scene.view * vec4(outWorldPos, 1.0);
}