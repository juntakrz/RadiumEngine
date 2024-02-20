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
	int textureSets;			// bitwise texture sets
	float metallicFactor;	
	float roughnessFactor;
	float alphaMask;	
	float alphaMaskCutoff;
	float bumpIntensity;
	uint samplerIndex[MAXTEXTURES];
	vec4 glowColor;
} material;

int getTextureSet(int textureType) {
	const int firstSetLookup = 2;
	const int secondSetLookup = 3;
	const int checkTextureSet = material.textureSets >> (textureType * 2);

	if ((checkTextureSet & secondSetLookup) == secondSetLookup) return 1;
	if ((checkTextureSet & firstSetLookup) == firstSetLookup) return 0;
	return -1;
}

layout (set = 2, binding = 0) uniform sampler2D samplers[];
layout (set = 2, binding = 0) uniform sampler2DArray arraySamplers[];
layout (set = 2, binding = 0) uniform samplerCube cubeSamplers[];