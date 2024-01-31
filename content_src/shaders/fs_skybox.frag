#version 460
#extension GL_EXT_nonuniform_qualifier : require

#define COLORMAP	0
#define MAXTEXTURES 6

layout (location = 0) in vec3 inTexCoord;
layout (location = 1) in vec4 inColor0;

layout (location = 0) out vec4 outColor;

// Material bindings
layout (set = 2, binding = 0) uniform samplerCube samplers[];

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
	vec3 color = texture(samplers[material.samplerIndex[COLORMAP]], inTexCoord).rgb * inColor0.rgb;
	outColor = vec4(color, 1.0);
}