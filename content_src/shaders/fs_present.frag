#version 460

layout (location = 0) in vec2 inUV;

// Material bindings

layout (set = 2, binding = 0) uniform sampler2D colorMap;

layout (location = 0) out vec4 outColor;

void main() {
	vec3 color = texture(colorMap, inUV).rgb;
	outColor = vec4(color, 1.0);
}