#version 460

struct NodeTransformBlock {
	mat4 matrix;
	float jointCount;
	float padding[15];
};

layout(binding = 0) uniform UBOView {
	mat4 view;
	mat4 projection;
} scene;

layout (set = 1, binding = 0) buffer UBOMesh0 {
	mat4 matrix[];
} model;

layout (set = 1, binding = 1) buffer UBOMesh1 {
	NodeTransformBlock block[];
} node;

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

layout(location = 0) out vec3 outTexCoord;
layout(location = 1) out vec4 outColor0;

void main(){
	const uint modelIndex = inInstanceTransformIndices.x;
	const uint nodeIndex = inInstanceTransformIndices.y;

	outTexCoord = inPos;
	outColor0 = inColor0;
	
	mat4 view = scene.view;
	view[3][0] = 0.0;
	view[3][1] = 0.0;
	view[3][2] = 0.0;
	
	vec4 worldPos = model.matrix[modelIndex] * node.block[nodeIndex].matrix * vec4(inPos, 1.0);
	worldPos = scene.projection * view * worldPos;

	gl_Position = worldPos.xyww;
}