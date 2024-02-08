#version 460
#extension GL_EXT_nonuniform_qualifier : require

#define COLORMAP	0
#define NORMALMAP	1
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
	// baseColorTextureSet is an index into the target mip level, upsampling will use the lower resolution one
	uint textureIndex = material.samplerIndex[NORMALMAP];
	int LOD = material.baseColorTextureSet + 1;

	ivec2 resolution = textureSize(samplers[textureIndex], LOD);
	vec2 texelSize = 1.0 / vec2(resolution); 

    /* Take 9 samples around the center (current) texel
    a - b - c
    d - e - f
    g - h - i */
    vec3 a = textureLod(samplers[textureIndex], vec2(inUV.x - texelSize.x, inUV.y - texelSize.y), LOD).rgb;
    vec3 b = textureLod(samplers[textureIndex], vec2(inUV.x, inUV.y - texelSize.y), LOD).rgb;
    vec3 c = textureLod(samplers[textureIndex], vec2(inUV.x + texelSize.x, inUV.y - texelSize.y), LOD).rgb;

    vec3 d = textureLod(samplers[textureIndex], vec2(inUV.x - texelSize.x, inUV.y), LOD).rgb;
    vec3 e = textureLod(samplers[textureIndex], vec2(inUV.x, inUV.y), LOD).rgb;
    vec3 f = textureLod(samplers[textureIndex], vec2(inUV.x + texelSize.x, inUV.y), LOD).rgb;

    vec3 g = textureLod(samplers[textureIndex], vec2(inUV.x - texelSize.x, inUV.y + texelSize.y), LOD).rgb;
    vec3 h = textureLod(samplers[textureIndex], vec2(inUV.x, inUV.y + texelSize.y), LOD).rgb;
    vec3 i = textureLod(samplers[textureIndex], vec2(inUV.x + texelSize.x, inUV.y + texelSize.y), LOD).rgb;

	/* Apply weighted distribution, by using a 3x3 tent filter:
    1		| 1 2 1 |
    --		| 2 4 2 |
    16		| 1 2 1 | */
    vec3 color = e * 4.0;
    color += a+c+g+i;
    color += (b+d+f+h) * 2.0;
	color *= 1.0 / 16.0;

	outColor = vec4(color, 1.0);
}