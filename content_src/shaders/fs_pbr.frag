#version 460

#include "include/common.glsl"
#include "include/fragment.glsl"

#define OCCLUSION_SAMPLES	16

layout (location = 0) in vec2 inUV0;

layout (location = 0) out vec4 outColor;
layout (location = 1) out float outAO;

// Scene bindings
layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
} scene;

// environment bindings
layout (set = 0, binding = 2) uniform samplerCube prefilteredMap;
layout (set = 0, binding = 3) uniform samplerCube irradianceMap;
layout (set = 0, binding = 4) uniform sampler2D BRDFLUTMap;
layout (set = 0, binding = 5) uniform sampler2D noiseMap;

layout (set = 0, binding = 6) buffer OcclusionOffsets {
	vec4 occlusionOffsets[OCCLUSION_SAMPLES];
} occlusionData;

const float shadowBias = 0.00001;
const float occlusionRadius = 1.5;
const float occlusionBias = 0.025;
const float occlusionDistance = 6.0;
const float occlusionColor = 0.01;

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
vec3 getIBLContribution(vec3 diffuseColor, vec3 specularColor, float roughness, vec3 V, vec3 normal) {
	float NdotV = clamp(abs(dot(normal, V)), 0.001, 1.0);
	vec3 reflection = -normalize(reflect(V, normal));
	reflection.y *= -1.0f;

	float lod = (roughness * lighting.prefilteredCubeMipLevels);

	// retrieve a scale and bias to F0
	vec2 brdf = (texture(BRDFLUTMap, vec2(NdotV, 1.0 - roughness))).rg;
	vec3 diffuseLight = tonemap(texture(irradianceMap, normal).rgb);
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

	if (shadowCoord.z > shadowDepth) {
		return 0.25;
	}
			
	return 1.0;
}

float getShadow(vec3 fragmentPosition, int distanceIndex, float facing) {
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
	
	// Adjust for the polygon facing, if not 1.0 - it is a back face and thus should have a slightly larger depth
	if (facing < 0.9) {
		shadowCoord.z += 0.0001;
	}

//	TODO: find a way to smooth shadows more based on the distance between shadow and fragment depths
//	float shadowDepth = texture(arraySamplers[lighting.samplerIndex[SUNLIGHTINDEX]], vec3(shadowCoord.st, distanceIndex)).r;
//	float scale = (shadowCoord.z > shadowDepth) ? (shadowCoord.z - shadowDepth) * 2000.0 : 1.0;
//	float dx = scale * (1.0 / float(texDim.x));
//	float dy = scale * (1.0 / float(texDim.y));
	
	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			vec2 offset = vec2(dx * x, dy * y);
			shadow += filterPCF(shadowCoord, offset, distanceIndex);
			count++;
		}
	}
	
	return shadow / count;
}

vec3 getLight(uint index, vec3 worldPos, vec3 diffuseColor, vec3 specularColor, vec3 V, vec3 normal, float roughness) {
	float alphaRoughness = roughness * roughness;

	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	vec3 L = normalize(lighting.lightLocations[index].xyz - worldPos);
	vec3 H = normalize(L + V);								// Half vector between both l and v

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
	vec3 lightColor = NdotL * lighting.lightColor[index].rgb * (diffuseContrib + specContrib) * lighting.lightColor[index].a;

	return lightColor;
}

float getOcclusion(vec3 worldPos, vec3 normal) {
	float occlusion = 0.0;

	ivec2 posDim = textureSize(samplers[material.samplerIndex[POSITIONMAP]], 0); 
	ivec2 noiseDim = textureSize(noiseMap, 0);
	const vec2 noiseUV = vec2(float(posDim.x)/float(noiseDim.x), float(posDim.y)/(noiseDim.y)) * inUV0;  
	vec3 randomVec = vec3(texture(noiseMap, noiseUV).rg, 0.0) * 2.0 - 1.0;
	randomVec.z = -randomVec.z;

	vec4 newPos = scene.view * vec4(worldPos, 1.0);

	// Create TBN matrix
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(tangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);

	for(int i = 0; i < OCCLUSION_SAMPLES; i++) {		
		vec3 samplePos = TBN * occlusionData.occlusionOffsets[i].xyz; 
		samplePos = newPos.xyz + samplePos * occlusionRadius; 

		vec4 sampleOffset = vec4(samplePos, 1.0);
		sampleOffset = scene.projection * sampleOffset; 
		sampleOffset.xyz /= sampleOffset.w; 
		sampleOffset.xyz = sampleOffset.xyz * 0.5 + 0.5;
		sampleOffset.y = 1.0 - sampleOffset.y;
		
		float sampleDepth = texture(samplers[material.samplerIndex[POSITIONMAP]], sampleOffset.xy).w; 
		
		if (sampleDepth < 0.0001) {
			occlusion += 1.0;
			continue;
		}

		float rangeCheck = smoothstep(0.0, 1.0, occlusionRadius / abs(newPos.z - sampleDepth));
		occlusion += (sampleDepth > samplePos.z - occlusionBias ? 1.0 : occlusionColor) * rangeCheck;
	}

	return occlusion /= float(OCCLUSION_SAMPLES);
}

void main() {
	const float occlusionStrength = 1.0;	// TODO: modify by the material variable
	const vec3 shadowColor = lighting.shadowColor.rgb + vec3(1.0);
	vec3 f0 = vec3(0.04);

	// Retrieve G-buffer data
	vec4 worldPos = texelFetch(samplers[material.samplerIndex[POSITIONMAP]], ivec2(gl_FragCoord.xy), 0);
	vec4 baseColor = texture(samplers[material.samplerIndex[COLORMAP]], inUV0);
	vec3 normal = texture(samplers[material.samplerIndex[NORMALMAP]], inUV0).rgb;
	float metallic = texture(samplers[material.samplerIndex[PHYSMAP]], inUV0).r;
	float perceptualRoughness = texture(samplers[material.samplerIndex[PHYSMAP]], inUV0).g;
	float ao = texture(samplers[material.samplerIndex[PHYSMAP]], inUV0).b;
	vec4 emissiveData = texture(samplers[material.samplerIndex[EMISMAP]], inUV0);

	vec3 emissive = emissiveData.rgb;
	float facing = emissiveData.a;

	// Account for back faces
	if (facing < 0.9) {
		normal = -normal;
	}

	vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
	diffuseColor *= 1.0 - metallic;

	vec3 specularColor = mix(f0, diffuseColor, metallic);

	vec3 V = normalize(scene.camPos - worldPos.xyz);			// Vector from surface point to camera

	vec3 color = getIBLContribution(diffuseColor, specularColor, perceptualRoughness, V, normal);

	if (lighting.lightColor[0].a > 0.001) {
		color += getLight(0, worldPos.xyz, diffuseColor, specularColor, V, normal, perceptualRoughness);
	}

	color = mix(color, color * ao, occlusionStrength);

	// Calculate shadow and its color
	float relativeDistance = length(scene.camPos - worldPos.xyz);
	float shadowA = 0.0f;

	int distanceIndex;
	for (distanceIndex = MAXCASCADES - 1; distanceIndex > -1; distanceIndex--) {
		if (relativeDistance > cascadeDistances[distanceIndex]) { 
			shadowA = getShadow(worldPos.xyz, distanceIndex, facing);

			// Smoothly interpolate shadow cascades
			if (distanceIndex < MAXCASCADES - 1) {
				float interpolation = interpolateCascades(relativeDistance, distanceIndex);
				float shadowB = getShadow(worldPos.xyz, distanceIndex + 1, facing);
				shadowA = mix(shadowA, shadowB, interpolation);
			}
			break;
		}
	}
	
	vec3 shadow = shadowColor * shadowA;
	shadow = clamp(shadow, 0.0, 1.0);
	color *= shadow;

	// Process point lights within a reasonable fragment's distance to save performance
	if (lighting.lightCount > 1) {
		for (uint index = 1; index < lighting.lightCount; ++index) {
			float lightDistance = length(lighting.lightLocations[index].xyz - worldPos.xyz);
			float lightAttenuation = 1.0 / (lightDistance * lightDistance);
			
			if (lightAttenuation > 0.1) {
				color += getLight(index, worldPos.xyz, diffuseColor, specularColor, V, normal, perceptualRoughness) * lightAttenuation;
			}
		}
	}
	
	// Emissive colors are not affected by shadows as they are supposed to glow
	color += emissive;

	outColor = vec4(color, baseColor.a);

	// Calculate SSAO (ignore emissive materials as they shouldn't self shadow)
	if (baseColor.a < 0.01 || length(emissive.rgb) > 1.0 || worldPos.w > occlusionDistance) {
		outAO = 1.0;
		return;
	}

	outAO = getOcclusion(worldPos.xyz, normal);
	float mixAO = clamp(worldPos.w - occlusionDistance + 1.0, 0.0, 1.0);
	outAO = mix(outAO, 1.0, mixAO);
}