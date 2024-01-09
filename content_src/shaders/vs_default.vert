#version 460

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
layout(location = 2) in vec2 inUV0;
layout(location = 6) in vec4 inColor0;

layout(location = 0) out vec2 outUV0;
layout(location = 1) out vec4 outColor0;

void main(){
	outUV0 = inUV0;
	outColor0 = inColor0;

	gl_Position = scene.projection * scene.view * (mesh.rootMatrix * mesh.nodeMatrix * vec4(inPos, 1.0));
}