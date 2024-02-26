#version 460

#include "include/common.glsl"
#include "include/fragment.glsl"

#define MIN_TRANSPARENCY_THRESHOLD	0.05
#define MAX_TRANSPARENCY_THRESHOLD	0.75

struct transparencyNode{
	vec4 color;
	float depth;
	uint nextNodeIndex;
};

layout (early_fragment_tests) in;

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;

// Scene bindings
layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
} scene;

// Environment bindings
layout (set = 0, binding = 2) uniform samplerCube prefilteredMap;
layout (set = 0, binding = 3) uniform samplerCube irradianceMap;
layout (set = 0, binding = 4) uniform sampler2D BRDFLUTMap;

// Counts how many nodes are stored and checks for node storage limits
layout (set = 1, binding = 3) buffer transparencyLinkedListData {
	uint nodeCount;
	uint maxNodeCount;
};

layout (set = 1, binding = 4, r32ui) uniform coherent uimage2D headIndexImage; 

// Transparency linked list buffer
layout (set = 1, binding = 5) buffer transparencyLinkedListBuffer {
	transparencyNode nodes[];
};

const float minRoughness = 0.04;
const float shadowBias = 0.00001;

vec3 tonemap(vec3 v) {
	vec3 color = tonemapACESApprox(v);
	return pow(color, vec3(1.0 / lighting.gamma));
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
	vec3 diffuseLight = tonemap(texture(irradianceMap, n).rgb);
	vec3 specularLight = tonemap(textureLod(prefilteredMap, reflection, lod).rgb);

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

float interpolateCascades(float dist, int cascadeIndex) {
	float cascadeStart = cascadeDistances[cascadeIndex];
    float cascadeEnd = cascadeDistances[cascadeIndex + 1];
    return (dist - cascadeStart) / (cascadeEnd - cascadeStart);
}

float filterPCF(vec3 shadowCoord, vec2 offset, uint distanceIndex) {
	float shadowDepth = texture(arraySamplers[lighting.samplerIndex[SUNLIGHTINDEX]], vec3(shadowCoord.st + offset, distanceIndex)).r;
	shadowDepth += shadowBias * float(distanceIndex + 1);

	if (!gl_FrontFacing) {
		shadowCoord.z += 0.0001;
	}

	if (shadowCoord.z > shadowDepth) {
		return 0.25;
	}
			
	return 1.0;
}

float getShadow(vec3 fragmentPosition, int distanceIndex) {
	

	ivec2 texDim = textureSize(samplers[lighting.samplerIndex[SUNLIGHTINDEX]], 0);
	float shadow = 0.0;
	float scale = 1.0;
	float dx = scale * (1.0 / float(texDim.x));
	float dy = scale * (1.0 / float(texDim.y));
	int count = 0;
	int range = 4 - distanceIndex;

	float FOVMultiplier = 1.0;

	for (int i = 0; i < distanceIndex; i++) {
		FOVMultiplier *= (i < 1) ? SHADOWFOVMULT : SHADOWFOVMULT * SHADOWFOVMULT;
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
			vec2 offset = vec2(dx * x, dy * y);
			shadow += filterPCF(shadowCoord, offset, distanceIndex);
			count++;
		}
	}
	
	return shadow / count;
}

vec3 getNormal(int textureSet) {
	vec3 tangentNormal = vec3(vec2(texture(samplers[material.samplerIndex[NORMALMAP]], textureSet == 0 ? inUV0 : inUV1).rg * 2.0 - 1.0), 1.0);

	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(inUV0);
	vec2 st2 = dFdy(inUV0);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal) * material.bumpIntensity;
}

vec4 getColor(vec4 baseColor) {
	const float occlusionStrength = 1.0;	// TODO: modify by the material variable
	const vec3 shadowColor = lighting.shadowColor.rgb + vec3(1.0);
	vec3 emissiveColor = vec3(0.0);
	vec3 f0 = vec3(0.04);
	
	float perceptualRoughness = material.roughnessFactor;
	float metallic = material.metallicFactor;

	// Extract normal for this fragment
	int textureSet = getTextureSet(NORMALMAP);
	vec3 normal = (textureSet > -1 && baseColor.a > MAX_TRANSPARENCY_THRESHOLD) ? getNormal(textureSet) : normalize(inNormal);

	if (!gl_FrontFacing) {
		normal = -normal;
	}

	// Get metallic and roughness properties from the texture if available
	textureSet = getTextureSet(PHYSMAP);
	if (textureSet > -1) {
		// In texture roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
		// This layout intentionally reserves the 'r' channel for (optional) ambient occlusion map data
		vec4 physicalSample = texture(samplers[material.samplerIndex[PHYSMAP]], textureSet == 0 ? inUV0 : inUV1);
		physicalSample.g = pow(physicalSample.g, 1.0 / 2.2);
		physicalSample.b = pow(physicalSample.b, 1.0 / 2.2);
		perceptualRoughness = physicalSample.g * perceptualRoughness;
		metallic = physicalSample.b * metallic;
	} else {
		perceptualRoughness = clamp(perceptualRoughness, minRoughness, 1.0);
		metallic = clamp(metallic, 0.0, 1.0);
	}

	// Apply vertex colors if present
	vec3 diffuseColor = baseColor.rgb * inColor0.rgb;

	diffuseColor *= vec3(1.0) - f0;
	diffuseColor *= 1.0 - metallic;	

	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	vec3 specularColor = mix(f0, baseColor.rgb, metallic);

	// Compute reflectance
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	vec3 V = normalize(scene.camPos - inWorldPos);			// Vector from surface point to camera
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
	
	// Get ambient occlusion from the texture if available
	textureSet = getTextureSet(AOMAP);
	if (textureSet > -1) {
		float ao = texture(samplers[material.samplerIndex[AOMAP]], (textureSet == 0 ? inUV0 : inUV1)).r;
		color = mix(color, color * ao, occlusionStrength);
	}

	// Extract emissive color from the texture if available
	textureSet = getTextureSet(EMISMAP);
	if (textureSet > -1) {
		emissiveColor = texture(samplers[material.samplerIndex[EMISMAP]], textureSet == 0 ? inUV0 : inUV1).rgb;
	}

	// Brighten or darken emission by the glow color value
	emissiveColor += material.glowColor.rgb;

	// Calculate shadow and its color
	float relativeDistance = length(scene.camPos - inWorldPos);
	float shadowA = 0.0f;

	int distanceIndex;
	for (distanceIndex = MAXCASCADES - 1; distanceIndex > -1; distanceIndex--) {
		if (relativeDistance > cascadeDistances[distanceIndex]) { 
			shadowA = getShadow(inWorldPos, distanceIndex);

			// Smoothly interpolate shadow cascades
			if (distanceIndex < MAXCASCADES - 1) {
				float interpolation = interpolateCascades(relativeDistance, distanceIndex);
				float shadowB = getShadow(inWorldPos, distanceIndex + 1);
				shadowA = mix(shadowA, shadowB, interpolation);
			}
			break;
		}
	}
	
	vec3 shadow = shadowColor * shadowA;
	shadow = clamp(shadow, 0.0, 1.0);
	color *= shadow;

	color += emissiveColor;

	return vec4(color, baseColor.a);
}

void main() {
	vec4 baseColor = vec4(0.0);

	int textureSet = getTextureSet(COLORMAP);
	if (textureSet > -1) {
		baseColor = texture(samplers[material.samplerIndex[COLORMAP]], textureSet == 0 ? inUV0 : inUV1);
	}

	if (baseColor.a < MIN_TRANSPARENCY_THRESHOLD) {
		discard;
	}

	// Increase the node count
    uint nodeIndex = atomicAdd(nodeCount, 1);

    // Check if a linked list is full
    if (nodeIndex < maxNodeCount)
    {
        // Exchange new head index and previous head index
        uint prevHeadIndex = imageAtomicExchange(headIndexImage, ivec2(gl_FragCoord.xy), nodeIndex);

        // Store node data
        nodes[nodeIndex].color = getColor(baseColor);
        nodes[nodeIndex].depth = gl_FragCoord.z;
        nodes[nodeIndex].nextNodeIndex = prevHeadIndex;
    }
}