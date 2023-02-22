#version 450

layout(set = 1, binding = 0) uniform sampler2D baseColor;
layout(set = 1, binding = 1) uniform sampler2D normalColor;
layout(set = 1, binding = 2) uniform sampler2D physicalColor;
layout(set = 1, binding = 3) uniform sampler2D occlusionColor;
layout(set = 1, binding = 4) uniform sampler2D emissiveColor;
layout(set = 1, binding = 5) uniform sampler2D extraColor;

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
	vec4 diffuse = texture(baseColor, texCoord0);
	vec4 normal = texture(normalColor, texCoord0);
	vec4 physical = texture(physicalColor, texCoord0);
	vec4 occlusion = texture(occlusionColor, texCoord0);
	vec4 emissive = texture(emissiveColor, texCoord0);
	vec4 extra = texture(extraColor, texCoord0);
	float metallic = physical.x;
	float roughness = physical.y;
	//outColor = diffuse * normal * physical;
	//outColor = physical;
	//outColor = occlusion;
	outColor = diffuse;
}