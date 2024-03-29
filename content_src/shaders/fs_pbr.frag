#version 460

#include "include/common.glsl"
#include "include/fragment.glsl"

#define OCCLUSION_SAMPLES	16

layout (location = 0) in vec2 inUV0;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outAO;

// Scene bindings
layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
	vec2 haltonJitter;
	vec2 planeData;
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
const float occlusionIntensity = 2.55;
const float occlusionRadius = 2.0;
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
	float shadowDepth = textureLod(arraySamplers[lighting.samplerIndex[SUNLIGHTINDEX]], vec3(shadowCoord.st + offset, distanceIndex), 0).r;
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

// SCREEN SPACE AMBIENT OCCLUSION
vec3 getRandomVector() {
	ivec2 posDim = textureSize(samplers[material.samplerIndex[POSITIONMAP]], 0); 
	ivec2 noiseDim = textureSize(noiseMap, 0);
	const vec2 noiseUV = vec2(float(posDim.x)/float(noiseDim.x), float(posDim.y)/(noiseDim.y)) * inUV0;  
	vec3 randomVector = vec3(textureLod(noiseMap, noiseUV, 0).rgb);

	return randomVector;
}

// HBAO
vec3 minDiff(vec3 P, vec3 Pr, vec3 Pl) {
  vec3 V1 = Pr - P;
  vec3 V2 = P - Pl;
  return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

vec3 getViewPos(vec2 UV) {
	float viewDepth = textureLod(samplers[material.samplerIndex[POSITIONMAP]], UV, 0).w;
	const vec2 A = vec2(2.0 / scene.projection[0][0], 2.0 / scene.projection[1][1]);
	const vec2 B = vec2(-1.0 / scene.projection[0][0], -1.0 / scene.projection[1][1]);
	vec3 viewPos = vec3((UV * A + B) * viewDepth, viewDepth);
	viewPos.y = -viewPos.y;

	return viewPos;
}

vec3 getViewNormal(vec2 UV, vec3 viewPos, ivec2 texDim) {
	vec2 offsetDelta = vec2(1.0 / float(texDim.x), 1.0 / float(texDim.y));
	vec3 Pr = getViewPos(UV + vec2(offsetDelta.x, 0.0));
	vec3 Pl = getViewPos(UV + vec2(-offsetDelta.x, 0.0));
	vec3 Pt = getViewPos(UV + vec2(0.0, offsetDelta.y));
	vec3 Pb = getViewPos(UV + vec2(0.0, -offsetDelta.y));

	return normalize(cross(minDiff(viewPos, Pr, Pl), minDiff(viewPos, Pt, Pb)));
}

vec2 rotateDirection(vec2 direction, vec2 cosSin) {
  return vec2(direction.x * cosSin.x - direction.y * cosSin.y,
              direction.x * cosSin.y + direction.y * cosSin.x);
}

float getHBAOPass(vec3 P, vec3 N, vec3 S) {
  vec3 V = S - P;
  float VdotV = dot(V, V);
  float NdotV = dot(N, V) * 1.0 / sqrt(VdotV);

  return clamp(NdotV - (occlusionBias * 4.0), 0.0, 1.0) * clamp((-0.25 * VdotV + 1.0), 0.0, 1.0);
}

float getHBAO(vec2 UV) {
	const int stepCount = int(sqrt(OCCLUSION_SAMPLES));
	const int directionCount = stepCount * 2;
	const ivec2 texDim = textureSize(samplers[material.samplerIndex[POSITIONMAP]], 0);
	const vec2 fTexStep = vec2(1.0 / texDim);

	vec3 viewPos = getViewPos(UV);
	vec3 viewNormal = getViewNormal(UV, viewPos, texDim);
	vec4 randomVector = vec4(getRandomVector(), 1.0);
	float radius = occlusionRadius * (float(texDim.y) * 0.25) / viewPos.z;

	// Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
	float stepSizePixels = radius / (stepCount + 1.0);

	const float alpha = 2.0 * M_PI / directionCount;
	float occlusion = 0.0;

	for (float directionIndex = 0; directionIndex < directionCount; directionIndex++) {
		float angle = alpha * directionIndex;

		// Compute normalized 2D direction
		vec2 direction = rotateDirection(vec2(cos(angle), sin(angle)), randomVector.xy);

		// Jitter starting sample within the first step
		float rayPixels = (randomVector.z * stepSizePixels + 1.0);

		for (float stepIndex = 0; stepIndex < stepCount; stepIndex++) {
			vec2 offset = round(rayPixels * direction) * fTexStep;
			vec2 snappedUV = UV + offset;
			vec3 S = getViewPos(snappedUV);
	  
			rayPixels += stepSizePixels;

			occlusion += getHBAOPass(viewPos, viewNormal, S);
		}
	}

	occlusion *= 1.0 / (directionCount * stepCount);
	return clamp(1.0 - occlusion * occlusionIntensity, 0.0, 1.0);
}

// SSAO
float getSSAO(vec3 worldPos, vec3 normal) {
	float occlusion = 0.0;

	vec4 newPos = scene.view * vec4(worldPos, 1.0);
	vec3 randomVector = getRandomVector();

	// Create TBN matrix
	vec3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
	vec3 bitangent = cross(tangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);

	float aoOcclusionRadius = occlusionRadius * 0.5;

	for(int i = 0; i < OCCLUSION_SAMPLES; i++) {		
		vec3 samplePos = TBN * occlusionData.occlusionOffsets[i].xyz; 
		samplePos = newPos.xyz + samplePos * aoOcclusionRadius; 

		vec4 sampleOffset = vec4(samplePos, 1.0);
		sampleOffset = scene.projection * sampleOffset; 
		sampleOffset.xyz /= sampleOffset.w; 
		sampleOffset.xyz = sampleOffset.xyz * 0.5 + 0.5;
		sampleOffset.y = 1.0 - sampleOffset.y;
		
		float sampleDepth = textureLod(samplers[material.samplerIndex[POSITIONMAP]], sampleOffset.xy, 0).w; 
		
		if (sampleDepth < 0.0001) {
			occlusion += 1.0;
			continue;
		}

		float rangeCheck = smoothstep(0.0, 1.0, aoOcclusionRadius / abs(newPos.z - sampleDepth));
		occlusion += (sampleDepth > samplePos.z - occlusionBias ? 1.0 : occlusionColor) * rangeCheck;
	}

	occlusion /= float(OCCLUSION_SAMPLES);

	// Trying to keep SSAO intensity in line with HBAO
	return clamp(occlusion * (1.0 / occlusionIntensity), 0.0, 1.0);
}

void main() {
	const float occlusionStrength = 1.0;	// TODO: modify by the material variable
	const vec3 shadowColor = lighting.shadowColor.rgb + vec3(1.0);
	vec3 f0 = vec3(0.04);

	// Retrieve G-buffer data
	vec4 worldPos = textureLod(samplers[material.samplerIndex[POSITIONMAP]], inUV0, 0);
	vec4 baseColor = textureLod(samplers[material.samplerIndex[COLORMAP]], inUV0, 0);
	vec3 normal = textureLod(samplers[material.samplerIndex[NORMALMAP]], inUV0, 0).rgb;
	vec3 physMap = textureLod(samplers[material.samplerIndex[PHYSMAP]], inUV0, 0).rgb;
	vec4 emissiveData = textureLod(samplers[material.samplerIndex[EMISMAP]], inUV0, 0);

	float metallic = physMap.r;
	float perceptualRoughness = physMap.g;
	float ao = physMap.b;

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
	if (lighting.aoMode == AO_NONE || baseColor.a < 0.01 || length(emissive.rgb) > 1.0 || worldPos.w > occlusionDistance) {
		outAO = vec2(1.0, scene.planeData.y);
		return;
	}

	switch (lighting.aoMode) {
		case AO_HBAO: {
			outAO = vec2(getHBAO(inUV0), worldPos.w);
			break;
		}
		default: {
			outAO = vec2(getSSAO(worldPos.xyz, normal), worldPos.w);
			break;
		}
	}

	// Ambient occlusion smooth falloff, starts at occlusionDistance - 2.0
	float aoFactor = clamp((worldPos.w - occlusionDistance + 2.0) * 0.5, 0.0, 1.0);
	outAO.r = mix(outAO.r, 1.0, aoFactor);
}