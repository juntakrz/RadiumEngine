layout (push_constant) uniform Material {
	layout(offset = 16) 
	int baseColorTextureSet;
	int normalTextureSet;
	int physicalDescriptorTextureSet;	
	int occlusionTextureSet;		// 16
	int emissiveTextureSet;
	int extraTextureSet;
	float metallicFactor;	
	float roughnessFactor;			// 32
	float alphaMask;	
	float alphaMaskCutoff;
	float bumpIntensity;
	vec4 emissiveIntensity;			// 48
	uint samplerIndex[MAXTEXTURES];
} material;