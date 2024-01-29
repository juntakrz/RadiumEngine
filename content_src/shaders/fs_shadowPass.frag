#version 460
#extension GL_EXT_nonuniform_qualifier : require

#define COLORMAP	0
#define MAXTEXTURES 6

layout (location = 0) in vec3 inWorldPos;
layout (location = 2) in vec2 inUV0;

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
	uint samplerIndex[MAXTEXTURES];
} material;

void main() {
	float alpha = texture(samplers[material.samplerIndex[COLORMAP]], inUV0).a;
	
	if (alpha < 0.5) {
		discard;
	}
}