#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

#include "include/common.glsl"

layout (location = 0) in vec2 inUV0;

layout (location = 0) out vec4 outColor;

// Scene bindings
layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
} scene;

layout (std430, set = 0, binding = 1) uniform UBOLighting {
	vec4 lightLocations[MAXLIGHTS];
    vec4 lightColor[MAXLIGHTS];
	mat4 lightViews[MAXSHADOWCASTERS];
	mat4 lightOrthoMatrix;
	uint samplerIndex[MAXSHADOWCASTERS];
	uint lightCount;
	float exposure;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
} lighting;

// environment bindings
layout (set = 0, binding = 2) uniform samplerCube prefilteredMap;
layout (set = 0, binding = 3) uniform samplerCube irradianceMap;
layout (set = 0, binding = 4) uniform sampler2D BRDFLUTMap;

// PBR Input bindings
layout (set = 2, binding = 0) uniform sampler2D samplers[];
layout (set = 2, binding = 0) uniform sampler2DArray arraySamplers[];

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

vec3 getDiffuse(vec3 inColor) {
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
vec3 getIBLContribution(vec3 diffuseColor, vec3 specularColor, float roughness, float NdotV, vec3 n, vec3 reflection) {
	float lod = (roughness * lighting.prefilteredCubeMipLevels);

	// retrieve a scale and bias to F0
	vec2 brdf = (texture(BRDFLUTMap, vec2(NdotV, 1.0 - roughness))).rg;
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

float filterPCF(vec3 shadowCoord, vec2 offset, uint distanceIndex) {
	float shadowDepth = texture(arraySamplers[lighting.samplerIndex[SUNLIGHTINDEX]], vec3(shadowCoord.st + offset, distanceIndex)).r;
	shadowDepth += 0.0001 * float(distanceIndex + 2);

	if (shadowCoord.z > shadowDepth) {
		return 0.1;
	}
			
	return 1.0;
}

float getShadow(vec3 fragmentPosition, uint distanceIndex) {
	ivec2 texDim = textureSize(samplers[lighting.samplerIndex[SUNLIGHTINDEX]], 0);
	float shadow = 0.0;
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);
	int count = 0;
	int range = 3 - int(distanceIndex);

	float FOVMultiplier = 1.0;

	for (uint i = 0; i < distanceIndex; i++) {
		FOVMultiplier *= 0.25;
	}

	mat4 newProjection = lighting.lightOrthoMatrix;
	newProjection[0][0] *= FOVMultiplier;	
	newProjection[1][1] *= FOVMultiplier;

	vec4 shadowPosition = newProjection * lighting.lightViews[0] * vec4(fragmentPosition, 1.0);
	shadowPosition /= shadowPosition.w;

	vec3 shadowCoord = vec3((shadowPosition.x * 0.5 + 0.5), 1.0 - (shadowPosition.y * 0.5 + 0.5), shadowPosition.z);
	
	if (shadowCoord.x < 0.0 || shadowCoord.x > 1.0 || shadowCoord.y < 0.0 || shadowCoord.y > 1.0) {
		return 1.0;
	}
	
	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			vec2 offset = vec2(dx*x, dy*y);
			shadow += filterPCF(shadowCoord, offset, distanceIndex);
			count++;
		}
	}
	
	return shadow / count;
}

void main() {
	const float occlusionStrength = 1.0;
	const float emissiveFactor = 1.0;
	vec3 f0 = vec3(0.04);

	// retrieve G-buffer data
	vec3 worldPos = texture(samplers[material.samplerIndex[POSITIONMAP]], inUV0).xyz;
	vec4 baseColor = texture(samplers[material.samplerIndex[COLORMAP]], inUV0);
	vec3 normal = texture(samplers[material.samplerIndex[NORMALMAP]], inUV0).rgb;
	float metallic = texture(samplers[material.samplerIndex[PHYSMAP]], inUV0).r;
	float perceptualRoughness = texture(samplers[material.samplerIndex[PHYSMAP]], inUV0).g;
	float ao = texture(samplers[material.samplerIndex[PHYSMAP]], inUV0).b;
	vec3 emissive = texture(samplers[material.samplerIndex[EMISMAP]], inUV0).rgb;

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

	// Calculate shadow
	float relativeLength = length(scene.camPos - worldPos);

	uint distanceIndex = 0;
	if (relativeLength > cascadeDistance0) distanceIndex = 1;
	if (relativeLength > cascadeDistance1) distanceIndex = 2;

	color *= getShadow(worldPos, distanceIndex);

	color += emissive;
	
	outColor = vec4(color, baseColor.a);
}