#version 450

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

layout(location = 0) in vec3 inPos;
layout(location = 6) in vec4 inColor0;

layout(location = 0) out vec3 outTexCoord;
layout(location = 1) out vec4 outColor0;

void main(){
	outTexCoord = inPos;
	outColor0 = inColor0;
	
	mat4 view = scene.view;
	view[3][0] = 0.0;
	view[3][1] = 0.0;
	view[3][2] = 0.0;
	
	vec4 worldPos = rootTransform.rootMatrix * nodeTransform.nodeMatrix * vec4(inPos, 1.0);
	worldPos = scene.projection * view * worldPos;

	gl_Position = worldPos.xyww;
}