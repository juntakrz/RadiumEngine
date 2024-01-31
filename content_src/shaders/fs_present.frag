#version 460
#extension GL_EXT_nonuniform_qualifier : require

#define COLORMAP	0
#define MAXTEXTURES 6

layout (location = 0) in vec2 inUV;

// Material bindings

layout (set = 2, binding = 0) uniform sampler2D samplers[];

layout (push_constant) uniform Material {
	layout(offset = 16) 
	int baseColorTextureSet;
	int physicalDescriptorTextureSet;
	int normalTextureSet;	
	int occlusionTextureSet;		// 16
	int emissiveTextureSet;
	int extraTextureSet;
	float metallicFactor;	
	float roughnessFactor;			// 32
	float alphaMask;	
	float alphaMaskCutoff;
	float bumpIntensity;
	float emissiveIntensity;		// 48
	vec4 colorIntensity;			// 64
	uint samplerIndex[MAXTEXTURES];
} material;

layout (location = 0) out vec4 outColor;

void main() {
	vec3 color = texture(samplers[material.samplerIndex[COLORMAP]], inUV).rgb;
	outColor = vec4(color, 1.0);
}