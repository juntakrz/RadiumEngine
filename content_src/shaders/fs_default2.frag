#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 1, binding = 0) uniform sampler2D baseColorMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D physicalMap;
layout(set = 1, binding = 3) uniform sampler2D occlusionMap;
layout(set = 1, binding = 4) uniform sampler2D emissiveMap;
layout(set = 1, binding = 5) uniform sampler2D extraMap;

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord0;
layout(location = 3) in vec2 inTexCoord1;
layout(location = 4) in vec4 inColor0;
layout(location = 5) in vec3 inT;
layout(location = 6) in vec3 inB;

layout (set = 0, binding = 0) uniform UBOView {
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
} scene;

layout (set = 0, binding = 1) uniform UBOLighting {
	vec4 dirLightPos;
	float exposure;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
} lighting;

layout (push_constant) uniform materialData {
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

layout(location = 0) out vec4 outColor;

const float M_PI = 3.141592653589793;
const float minRoughness = 0.04;

vec3 tonemapSet(vec3 color) {
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
	vec3 outColor = tonemapSet(color.rgb * lighting.exposure);
	outColor = outColor * (1.0 / tonemapSet(vec3(11.2)));	
	return vec4(pow(outColor, vec3(1.0 / lighting.gamma)), color.a);
}

//PBS functions
float distributionGGX(float NdotH, float roughness)
{
    //a = roughness * roughness, a2 = a * a
    float a2 = roughness * roughness * roughness * roughness;
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = M_PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getNormal() {
	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	//vec3 tangentNormal = texture(normalMap, material.normalTextureSet == 0 ? inTexCoord0 : inTexCoord1).xyz * 2.0 - 1.0;
	vec3 tangentNormal = texture(normalMap, inTexCoord0).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(inTexCoord0);
	vec2 st2 = dFdy(inTexCoord0);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, inT));

	mat3 TBN = mat3(inT, inB, N);

	return normalize(TBN * tangentNormal);
}

vec3 diffuse(vec3 diffuseColor) {
	return diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(vec3 reflectance0, vec3 reflectance90, float VdotH) {
	return reflectance0 + (reflectance90 - reflectance0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(float NdotL, float NdotV, float alphaRoughness) {
	float r2 = alphaRoughness * alphaRoughness;
	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r2 + (1.0 - r2) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r2 + (1.0 - r2) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(float NdotH, float alphaRoughness) {
	float r2 = alphaRoughness * alphaRoughness;
	float f = (NdotH * r2 - NdotH) * NdotH + 1.0;
	return r2 / (M_PI * f * f);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
	float perceptualRoughness;
	float metallic;
	vec3 diffuseColor;
	vec4 baseColor;

	const vec3 lightPos = vec3(-10.0, 0.0, 10.0);		// replace with buffer light data
	const vec3 lightColor = vec3(1.0);

	if (material.alphaMask == 1.0) {
		if (material.baseColorTextureSet > -1) {
			baseColor = texture(baseColorMap, material.baseColorTextureSet == 0 ? inTexCoord0 : inTexCoord1) * material.baseColorFactor;
		} else {
			baseColor = material.baseColorFactor;
		}
		if (baseColor.a < material.alphaMaskCutoff) {
			discard;
		}
	}

	// Metallic and Roughness material properties are packed together
	// In glTF, these factors can be specified by fixed scalar values
	// or from a metallic-roughness map
	perceptualRoughness = material.roughnessFactor;
	metallic = material.metallicFactor;

	if (material.physicalDescriptorTextureSet > -1) {
		// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
		// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
		vec4 mrSample = texture(physicalMap, material.physicalDescriptorTextureSet == 0 ? inTexCoord0 : inTexCoord1);
		perceptualRoughness = mrSample.g * perceptualRoughness;
		metallic = mrSample.b * metallic;
	} else {
		perceptualRoughness = clamp(perceptualRoughness, minRoughness, 1.0);
		metallic = clamp(metallic, 0.0, 1.0);
	}

	// The albedo may be defined from a base texture or a flat color
	if (material.baseColorTextureSet > -1) {
		baseColor = texture(baseColorMap, material.baseColorTextureSet == 0 ? inTexCoord0 : inTexCoord1) * material.baseColorFactor;
	} else {
		baseColor = material.baseColorFactor;
	}

	baseColor *= inColor0;

	diffuseColor = baseColor.rgb * (vec3(1.0) - material.f0.rgb);
	diffuseColor *= 1.0 - metallic;
	
	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness [2].	
	float alphaRoughness = perceptualRoughness;// * perceptualRoughness;

	vec3 specularColor = mix(material.f0.rgb, baseColor.rgb, metallic);

	//vec3 n = (material.normalTextureSet > -1) ? getNormal() : normalize(inNormal);
	vec3 tN = texture(normalMap, inTexCoord0).rgb * 2.0 - 1.0;
	vec3 n = normalize(tN.x * inT + tN.y * -inB + tN.z * inNormal);
	vec3 v = normalize(scene.camPos - inWorldPos);    // Vector from surface point to camera
	//vec3 l = normalize(lighting.dirLightPos.xyz);   // Vector from surface point to directional light
	vec3 l = normalize(lightPos.xyz);
	vec3 h = normalize(l+v);                        // Half vector between both l and v
	//vec3 reflection = -normalize(reflect(v, n));
	//reflection.y *= -1.0;

	//float NdotL = clamp(dot(n, l), 0.001, 1.0);
	//float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
	//float NdotH = clamp(dot(n, h), 0.0, 1.0);
	//float VdotH = clamp(dot(v, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);

	float NdotV = max(dot(n, v), 0.00000001);           //prevent divide by zero
    float NdotL = max(dot(n, l), 0.00000001);
    float NdotH = max(dot(n, h), 0.0);
    float VdotH = max(dot(h, v), 0.0);

	// Calculate the shading terms for the microfacet specular shading model
	//vec3 F = specularReflection(specularEnvironmentR0, specularEnvironmentR90, VdotH);
	//float G = geometricOcclusion(NdotL, NdotV, alphaRoughness);
	//float D = microfacetDistribution(NdotH, alphaRoughness);

	vec3 F = fresnelSchlick(VdotH, specularColor);
	float G = geometrySmith(NdotV, NdotL, alphaRoughness);
	float D = distributionGGX(NdotH, alphaRoughness);

	// Calculation of analytical lighting contribution
	vec3 diffuseContrib = (1.0 - F) * diffuse(diffuseColor);
	vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);

	// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
	vec3 color = NdotL * lightColor * (diffuseContrib + specContrib);

	// Calculate lighting contribution from image based lighting source (IBL)
	//color += getIBLContribution(pbrInputs, n, reflection);

	const float occlusionStrength = 1.0f;

	// Apply optional PBR terms for additional (optional) shading
	if (material.occlusionTextureSet > -1) {
		float ao = texture(occlusionMap, (material.occlusionTextureSet == 0 ? inTexCoord0 : inTexCoord1)).r;
		color = mix(color, color * ao, occlusionStrength);
	}

	const float emissiveFactor = 1.0f;
	if (material.emissiveTextureSet > -1) {
		vec3 emissive = texture(emissiveMap, material.emissiveTextureSet == 0 ? inTexCoord0 : inTexCoord1).rgb * emissiveFactor;
		color += emissive;
	}

	// simplified ambient color contribution
	color += vec3(0.025, 0.01, 0.07) * diffuseContrib;	

	outColor = vec4(color, baseColor.a);
}