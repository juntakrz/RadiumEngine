layout (std430, set = 0, binding = 1) uniform UBOLighting {
	vec4 lightLocations[MAXLIGHTS];
    vec4 lightColor[MAXLIGHTS];
	mat4 lightViews[MAXSHADOWCASTERS];
	mat4 lightOrthoMatrix;
	uint samplerIndex[MAXSHADOWCASTERS];
	uint lightCount;
	vec4 shadowColor;
	float averageLuminance;
	float bloomIntensity;
	float exposure;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
} lighting;

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
	vec4 glowColor;					// 48
	uint samplerIndex[MAXTEXTURES];
} material;