#version 450

layout(binding = 1) uniform sampler2D baseColor;

layout(location = 0) in vec2 texCoord0;

layout(location = 0) out vec4 outColor;

void main(){
	//outColor = texture(baseColor, texCoord0);
	outColor = vec4(texCoord0, 0.0, 1.0);
}