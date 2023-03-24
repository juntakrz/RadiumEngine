#version 450

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

layout (set = 0, binding = 1) uniform UBOLighting {
	vec4 lightDir;
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
layout (set = 2, binding = 2) uniform sampler2D physicalDescriptorMap;
layout (set = 2, binding = 3) uniform sampler2D aoMap;
layout (set = 2, binding = 4) uniform sampler2D emissiveMap;
layout (set = 2, binding = 5) uniform sampler2D extraMap;		// TODO: implement extra map in shader

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

layout (location = 0) out vec4 outColor;

const float M_PI = 3.141592653589793;
const float minRoughness = 0.04;

vec3 Uncharted2Tonemap(vec3 color) {
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec4 tonemap(vec4 color) {
	vec3 outcol = Uncharted2Tonemap(color.rgb * lighting.exposure);
	outcol = outcol * (1.0 / Uncharted2Tonemap(vec3(11.2)));	
	return vec4(pow(outcol, vec3(1.0f / lighting.gamma)), color.a);
}

vec3 getNormal() {
	vec3 tangentNormal = vec3(vec2(texture(normalMap, material.normalTextureSet == 0 ? inUV0 : inUV1).rg * 2.0 - 1.0), 1.0);

	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(inUV0);
	vec2 st2 = dFdy(inUV0);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
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
	float lod = (roughness * 32.0);

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

	//diffuse *= lighting.scaleIBLAmbient;
	//specular *= lighting.scaleIBLAmbient;

	return diffuse + specular;
}

void main() {
	float perceptualRoughness;
	float metallic;
	vec3 diffuseColor;
	vec4 baseColor;
	
	const vec3 dirLightPos = {-0.74, 0.6428, 0.1983};
	const vec3 dirLightColor = vec3(1.0);
	const float occlusionStrength = 1.0;
	const float emissiveFactor = 1.0;
	vec3 f0 = vec3(0.04);


	if (material.baseColorTextureSet > -1) {
		baseColor = texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1) * material.baseColorFactor;
	} else {
		baseColor = material.baseColorFactor;
	}
	
	// TODO: discard on alphaMask == 1.0 here and depth-sort everything, unless requested by the material
	if (material.alphaMask > 0.9 && baseColor.a < material.alphaMaskCutoff && material.alphaMaskCutoff < 1.1) {
		discard;
	}

	// Metallic and Roughness material properties are packed together
	// In glTF, these factors can be specified by fixed scalar values
	// or from a metallic-roughness map
	perceptualRoughness = material.roughnessFactor;
	metallic = material.metallicFactor;
	if (material.physicalDescriptorTextureSet > -1) {
		// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
		// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
		vec4 physicalSample = texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1);
		physicalSample.g = pow(physicalSample.g, 1.0 / 2.2);
		physicalSample.b = pow(physicalSample.b, 1.0 / 2.2);
		perceptualRoughness = physicalSample.g * perceptualRoughness;
		metallic = physicalSample.b * metallic;
	} else {
		perceptualRoughness = clamp(perceptualRoughness, minRoughness, 1.0);
		metallic = clamp(metallic, 0.0, 1.0);
	}
	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness [2].

	// The albedo may be defined from a base texture or a flat color
	if (material.baseColorTextureSet > -1) {
		baseColor = texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1) * material.baseColorFactor;
	} else {
		baseColor = material.baseColorFactor;
	}

	if(baseColor.a < 0.5){
		discard;
	}

	baseColor *= inColor0;

	diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
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

	vec3 N = (material.normalTextureSet > -1) ? getNormal() : normalize(inNormal);

	vec3 V = normalize(scene.camPos - inWorldPos);  // Vector from surface point to camera
	vec3 L = normalize(dirLightPos);				// Vector from surface point to light
	vec3 H = normalize(L + V);                      // Half vector between both l and v
	vec3 reflection = -normalize(reflect(V, N));
	reflection.y *= -1.0f;

	float NdotL = clamp(dot(N, L), 0.001, 1.0);
	float NdotV = clamp(abs(dot(N, V)), 0.001, 1.0);
	float NdotH = clamp(dot(N, H), 0.0, 1.0);
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
	vec3 color = NdotL * dirLightColor * (diffuseContrib + specContrib);

	// Calculate lighting contribution from image based lighting source (IBL)
	color += getIBLContribution(diffuseColor, specularColor, perceptualRoughness, NdotV, N, reflection);

	if (material.occlusionTextureSet > -1) {
		float ao = texture(aoMap, (material.occlusionTextureSet == 0 ? inUV0 : inUV1)).r;
		color = mix(color, color * ao, occlusionStrength);
	}

	if (material.emissiveTextureSet > -1) {
		vec3 emissive = texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1).rgb * emissiveFactor;
		color += emissive;
	}
	
	outColor = vec4(color, baseColor.a);
}