#version 460
#extension GL_EXT_buffer_reference : require

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
} scene;

layout (set = 1, binding = 0) uniform UBOMesh0 {
	mat4 rootMatrix;
} rootTransform;

layout (set = 1, binding = 1) uniform UBOMesh1 {
	mat4 nodeMatrix;
} nodeTransform;

layout(location = 0) out vec3 outTexCoord;
layout(location = 1) out vec4 outColor0;

layout(std430, buffer_reference) readonly buffer vertexBufferReference { 
	Vertex vertices[];
};

layout(push_constant) uniform vertexPCB {	
	vertexBufferReference vertexBuffer;
} pcb;

void main(){
	Vertex vertexData = pcb.vertexBuffer.vertices[gl_VertexIndex];

	outTexCoord = vertexData.inPos;
	outColor0 = vertexData.inColor0;
	
	mat4 view = scene.view;
	view[3][0] = 0.0;
	view[3][1] = 0.0;
	view[3][2] = 0.0;
	
	vec4 worldPos = rootTransform.rootMatrix * nodeTransform.nodeMatrix * vec4(vertexData.inPos, 1.0);
	worldPos = scene.projection * view * worldPos;

	gl_Position = worldPos.xyww;
}