#version 450

layout (location = 0) in vec3 inPos;
layout (location = 0) out vec3 outWorldPos;

layout(binding = 0) uniform UBOView {
	mat4 view;
	mat4 projection;
} scene;

layout (set = 1, binding = 0) uniform UBOMesh {
	mat4 rootMatrix;
	mat4 nodeMatrix;
} mesh;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	outWorldPos = inPos;

	mat4 view = scene.view;
	view[3][0] = 0.0;
	view[3][1] = 0.0;
	view[3][2] = 0.0;
	
	vec4 worldPos = mesh.rootMatrix * mesh.nodeMatrix * vec4(inPos, 1.0);
	worldPos = scene.projection * view * worldPos;

	gl_Position = worldPos.xyww;
}
