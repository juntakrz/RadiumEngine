#version 450

layout (location = 0) in vec3 inTexCoord;
layout (location = 1) in vec4 inColor0;

// Material bindings

layout (set = 2, binding = 0) uniform samplerCube colorMap;

layout (location = 1) out vec4 outColor;

void main() {
	vec3 color = texture(colorMap, inTexCoord).rgb * inColor0.rgb;
	outColor = vec4(color, 1.0);
}