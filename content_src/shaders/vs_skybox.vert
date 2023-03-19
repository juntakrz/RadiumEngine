#version 450

layout(binding = 0) uniform UBOView {
	mat4 view;
	mat4 projection;
	vec3 cameraPos;
} scene;

layout (set = 1, binding = 0) uniform UBOMesh {
	mat4 rootMatrix;
	mat4 nodeMatrix;
} mesh;

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

	vec4 worldPos = mesh.rootMatrix * mesh.nodeMatrix * vec4(inPos, 1.0);

	gl_Position = scene.projection * view * worldPos;
}