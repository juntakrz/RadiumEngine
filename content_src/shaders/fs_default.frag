#version 450

layout(binding = 1) uniform sampler2D baseColor;

layout(location = 0) in vec2 texCoord0;

layout (push_constant) uniform MaterialData {
	vec4 baseColorFactor;
	vec4 emissiveFactor;
	int baseColorTextureSet;
	int physicalDescriptorTextureSet;
	int normalTextureSet;	
	int occlusionTextureSet;
	int emissiveTextureSet;
	int extraTextureSet;
	float metallicFactor;	
	float roughnessFactor;	
	float alphaMask;	
	float alphaMaskCutoff;
	float bumpIntensity;
	float materialIntensity;
} pcMaterial;

layout(location = 0) out vec4 outColor;

void main(){
	//outColor = texture(baseColor, texCoord0);
	outColor = vec4(texCoord0, 0.0, 1.0);
}