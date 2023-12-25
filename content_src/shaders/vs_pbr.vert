#version 450

layout(binding = 0) uniform UBOView {
	mat4 view;
	mat4 projection;
} scene;

layout(location = 0) in vec3 inPos;

layout(location = 2) out vec3 outTexCoord;

void main(){
	outTexCoord = inPos;
	
	mat4 view = scene.view;
	view[3][0] = 0.0;
	view[3][1] = 0.0;
	view[3][2] = 0.0;
	
	vec4 worldPos = scene.projection * view * vec4(inPos.xy, 0.0, 1.0);

	gl_Position = worldPos.xyww;
}