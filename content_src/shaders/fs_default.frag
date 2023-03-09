#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 1, binding = 0) uniform sampler2D inBaseColorMap;
layout(set = 1, binding = 1) uniform sampler2D inNormalMap;
layout(set = 1, binding = 2) uniform sampler2D inPhysicalMap;
layout(set = 1, binding = 3) uniform sampler2D inOcclusionMap;
layout(set = 1, binding = 4) uniform sampler2D inEmissiveMap;
layout(set = 1, binding = 5) uniform sampler2D inExtraMap;

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord0;
layout(location = 3) in vec2 inTexCoord1;
layout(location = 4) in vec4 inColor0;

layout (set = 0, binding = 1) uniform UBOParams {
	vec4 lightDir;
	float exposure;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
	float debugViewInputs;
	float debugViewEquation;
} uboParams;

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
} material;

layout(location = 0) out vec4 outColor;

struct PBRInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector
	float VdotH;                  // cos angle between view direction and half vector
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float metalness;              // metallic value at the surface
	vec3 reflectance0;            // full reflectance color (normal incidence angle)
	vec3 reflectance90;           // reflectance color at grazing angle
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	vec3 diffuseColor;            // color contribution from diffuse lighting
	vec3 specularColor;           // color contribution from specular lighting
};

vec3 tonemapSet(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec4 tonemap(vec4 color)
{
	vec3 outColor = tonemapSet(color.rgb * uboParams.exposure);
	outColor = outColor * (1.0f / tonemapSet(vec3(11.2f)));	
	return vec4(pow(outColor, vec3(1.0f / uboParams.gamma)), color.a);
}

vec4 sRGBtoLinear(vec4 sRGB)
{
	#ifdef MANUAL_SRGB
	#ifdef SRGB_FAST_APPROXIMATION
	vec3 linOut = pow(sRGB.xyz,vec3(2.2));
	#else //SRGB_FAST_APPROXIMATION
	vec3 bLess = step(vec3(0.04045),sRGB.xyz);
	vec3 linOut = mix(sRGB.xyz/vec3(12.92), pow((sRGB.xyz+vec3(0.055))/vec3(1.055),vec3(2.4)), bLess);
	#endif //SRGB_FAST_APPROXIMATION
	return vec4(linOut, sRGB.w);;
	#else //MANUAL_SRGB
	return sRGB;
	#endif //MANUAL_SRGB
}

// find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getNormal()
{
	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec3 tangentNormal = texture(inNormalMap, material.normalTextureSet == 0 ? inTexCoord0 : inTexCoord1).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(inTexCoord0);
	vec2 st2 = dFdy(inTexCoord0);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

const float M_PI = 3.141592653589793;
const float minRoughness = 0.04;

void main(){
	vec4 baseColor = texture(inBaseColorMap, inTexCoord0);
	vec4 normalColor = texture(inNormalMap, inTexCoord0);
	vec4 physicalColor = texture(inPhysicalMap, inTexCoord0);
	vec4 occlusionColor = texture(inOcclusionMap, inTexCoord0);
	vec4 emissiveColor = texture(inEmissiveMap, inTexCoord0);
	vec4 extraColor = texture(inExtraMap, inTexCoord0);
	float metallic = physicalColor.r;
	float roughness = physicalColor.g;

	// test lighting
	vec3 lightPos = vec3(10.0, 8.0, 10.0);
	float lightDotProduct = dot(inNormal, normalize(lightPos));
	lightDotProduct = clamp(lightDotProduct, 0.0, 1.0);
	vec3 diffuse = vec3(1.0) * lightDotProduct;
	vec3 ambient = vec3(0.02);
	
	outColor = tonemap(vec4((diffuse + ambient) * baseColor.rgb, 1.0));
	//outColor = vec4((diffuse + ambient) * baseColor.rgb, 1.0);
}