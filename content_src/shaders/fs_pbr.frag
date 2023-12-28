#version 450

#define MAX_LIGHTS 32

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;

layout (location = 0) out vec4 outColor;

// Scene bindings
layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
} scene;

layout (set = 0, binding = 1) uniform UBOLighting {
	vec4 lightLocations[MAX_LIGHTS];
    vec4 lightColor[MAX_LIGHTS];
	float lightCount;
	float exposure;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
} lighting;

// environment bindings
layout (set = 0, binding = 2) uniform samplerCube prefilteredMap;
layout (set = 0, binding = 3) uniform samplerCube irradianceMap;
layout (set = 0, binding = 4) uniform sampler2D BRDFLUTMap;

// material bindings
layout (set = 2, binding = 0) uniform sampler2D colorMap;
layout (set = 2, binding = 1) uniform sampler2D normalMap;
layout (set = 2, binding = 2) uniform sampler2D physicalMap;	// r = metalness, g = roughness, b = ambient occlusion
layout (set = 2, binding = 3) uniform sampler2D aoMap;			// fragment worldspace position RTGT_GPOS
layout (set = 2, binding = 4) uniform sampler2D emissiveMap;
layout (set = 2, binding = 5) uniform sampler2D extraMap;		// unused

layout (push_constant) uniform Material {
	vec4 baseColorFactor;
	vec4 emissiveFactor;
	vec4 f0;
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

const float M_PI = 3.141592653589793;
const float minRoughness = 0.04;

vec3 ACESTonemap(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec4 tonemap(vec4 color) {
	vec3 outColor = ACESTonemap(color.rgb);
	return vec4(pow(outColor, vec3(1.0f / lighting.gamma)), color.a);
}

vec3 getDiffuse(vec3 inColor)
{
	return inColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 getFresnelReflectance(vec3 reflectance0, vec3 reflectance90, float VdotH) {
	return reflectance0 + (reflectance90 - reflectance0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

vec3 getFresnelReflectanceR(vec3 reflectance0, vec3 reflectance90, float roughness, float VdotH) {
	return reflectance0 + (max(reflectance90 - roughness, reflectance0) - reflectance0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float getGeometricOcclusion(float NdotL, float NdotV, float roughness) {
	float r2 = roughness * roughness;
	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r2 + (1.0 - r2) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r2 + (1.0 - r2) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float getMicrofacetDistribution(float roughness, float NdotH) {
	float r2 = roughness * roughness;
	float f = (NdotH * r2 - NdotH) * NdotH + 1.0;
	return r2 / (M_PI * f * f);
}

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs
vec3 getIBLContribution(vec3 diffuseColor, vec3 specularColor, float roughness, float NdotV, vec3 n, vec3 reflection)
{
	//float lod = (roughness * lighting.prefilteredCubeMipLevels);
	float lod = (roughness * 16.0);

	// retrieve a scale and bias to F0
	vec3 brdf = (texture(BRDFLUTMap, vec2(NdotV, 1.0 - roughness))).rgb;
	vec3 diffuseLight = tonemap(texture(irradianceMap, n)).rgb;
	vec3 specularLight = tonemap(textureLod(prefilteredMap, reflection, lod)).rgb;

	diffuseLight.r = pow(diffuseLight.r, 2.2);
	diffuseLight.g = pow(diffuseLight.g, 2.2);
	diffuseLight.b = pow(diffuseLight.b, 2.2);

	specularLight.r = pow(specularLight.r, 2.2);
	specularLight.g = pow(specularLight.g, 2.2);
	specularLight.b = pow(specularLight.b, 2.2);

	vec3 diffuse = diffuseLight * diffuseColor;
	vec3 specular = specularLight * (specularColor * brdf.x + brdf.y);

	diffuse *= lighting.scaleIBLAmbient;
	specular *= lighting.scaleIBLAmbient;

	return diffuse + specular;
}

void main() {
	const float occlusionStrength = 1.0;
	const float emissiveFactor = 1.0;
	vec3 f0 = vec3(0.04);

	// retrieve G-buffer data
	vec4 baseColor = texture(colorMap, inUV0);
    vec3 normal = texture(normalMap, inUV0).rgb;
	float metallic = texture(physicalMap, inUV0).r;
	float perceptualRoughness = texture(physicalMap, inUV0).g;
	float ao = texture(physicalMap, inUV0).b;
    vec3 worldPos = texture(aoMap, inUV0).xyz;
    vec3 emissive = texture(emissiveMap, inUV0).rgb;

	// do PBR
	vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
	diffuseColor *= 1.0 - metallic;
		
	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	vec3 specularColor = mix(f0, baseColor.rgb, metallic);

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	vec3 V = normalize(scene.camPos - worldPos);			// Vector from surface point to camera
	vec3 L = normalize(lighting.lightLocations[0].xyz);		// Vector from surface point to light
	vec3 H = normalize(L + V);								// Half vector between both l and v
	vec3 reflection = -normalize(reflect(V, normal));
	reflection.y *= -1.0f;

	float NdotL = clamp(dot(normal, L), 0.001, 1.0);
	float NdotV = clamp(abs(dot(normal, V)), 0.001, 1.0);
	float NdotH = clamp(dot(normal, H), 0.0, 1.0);
	float LdotH = clamp(dot(L, H), 0.0, 1.0);
	float VdotH = clamp(dot(V, H), 0.0, 1.0);

	// Calculate the shading terms for the microfacet specular shading model
	vec3 F = getFresnelReflectanceR(specularEnvironmentR0, specularEnvironmentR90, alphaRoughness, VdotH);
	float G = getGeometricOcclusion(NdotL, NdotV, alphaRoughness);
	float D = getMicrofacetDistribution(alphaRoughness, NdotH);

	// Calculation of analytical lighting contribution
	vec3 diffuseContrib = (1.0 - F) * getDiffuse(diffuseColor);
	vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);

	// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
	vec3 color = NdotL * lighting.lightColor[0].rgb * (diffuseContrib + specContrib) * lighting.lightColor[0].a;

	// Calculate lighting contribution from image based lighting source (IBL)
	color += getIBLContribution(diffuseColor, specularColor, perceptualRoughness, NdotV, normal, reflection);
	color = mix(color, color * ao, occlusionStrength);
	color += emissive;
	
	outColor = vec4(color, baseColor.a);
}