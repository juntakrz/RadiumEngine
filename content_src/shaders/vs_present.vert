#version 450

layout(binding = 0) uniform UBOView {
	mat4 view;
	mat4 projection;
} scene;

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


void main(){
	outUV0 = inUV0;

	gl_Position = vec4(inPos.xy, 0.0, 1.0);
}