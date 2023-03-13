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

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness){
    return F0 + (max(vec3(1.0-roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}
// ----------------------------------------------------------------------------

vec3 tonemap(vec3 color){
    return clamp((color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
}

mat3 getTBN(vec3 N, bool flipB) {
    N = normalize(N);
	vec3 B = normalize(cross(N, inT));
    B = flipB ? -B : B;    

    return mat3(inT, B, N);
}

void main() {
    vec3 dirLightPos = vec3(-10.0, 0.0, 10.0);
    vec3 dirLightColor = vec3(1.0);
    float dirLightIntensity = 1.0;
    float matIntensity = 1.0;
	
    //init values    
    vec4 baseMap = texture(baseColorMap, inTexCoord0);

    // convert color to linear space
    baseMap.r = pow(baseMap.r, lighting.gamma);
    baseMap.g = pow(baseMap.g, lighting.gamma);
    baseMap.b = pow(baseMap.b, lighting.gamma);

    vec3 albedo = baseMap.rgb;
    vec3 normal = texture(normalMap, inTexCoord0).rgb;
    float metallic = texture(physicalMap, inTexCoord0).b * material.metallicFactor;
    float roughness = texture(physicalMap, inTexCoord0).g * material.roughnessFactor * 2.0;
    float ao = texture(occlusionMap, inTexCoord0).r;
    
    vec3 L, radiance, color;
    
    vec3 Lo = { 0.0, 0.0, 0.0 };
    
    //calculate material composition
    vec3 F0 = mix(material.f0.rgb, albedo, metallic);
    
    //set normal map range from (0, +1) to (-1, +1)
    normal = (normal * 2.0) - 1.0;

    //vec3 N = normalize(normal.x * inT + normal.y * -inB + normal.z * inNormal);
    mat3 TBN = getTBN(inNormal, false);
    vec3 N = normalize(TBN * normal);
    vec3 V = normalize(scene.camPos - inWorldPos);      //view vector
    L = normalize(dirLightPos);
    radiance = dirLightColor * dirLightIntensity * matIntensity;
    vec3 H = normalize(V + L);                          //half vector between view and light
    
    //Cook-Torrance BRDF
    float NdotV = max(dot(N, V), 0.0001);               //prevent divide by zero
    float NdotL = max(dot(N, L), 0.0001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
        
    float NDF = distributionGGX(NdotH, roughness);
    float G = geometrySmith(NdotV, NdotL, roughness);
    vec3 F = fresnelSchlickRoughness(HdotV, F0, roughness);
    
    vec3 specular = NDF * G * F / 4.0 * NdotV * NdotL;
        
    //conservation of energy, must not reflect more light than receives
    vec3 Kd = (1.0 - F) * (1.0 - metallic);
    
    Lo += (Kd * albedo / M_PI + specular) * radiance * NdotL;

    // flipped specular for reflecting light directly (needs testing to see if viable)
    TBN = getTBN(inNormal, true);
    N = normalize(TBN * normal);
    NdotV = max(dot(N, V), 0.0001);               //prevent divide by zero
    NdotL = max(dot(N, L), 0.0001);
    NdotH = max(dot(N, H), 0.0);
        
    NDF = distributionGGX(NdotH, roughness);
    G = geometrySmith(NdotV, NdotL, roughness);
    
    specular = NDF * G * F / 4.0 * NdotV * NdotL;

    Lo += specular * NdotL * 0.2;
    //
    
    // replace with IBL
    vec3 globalAmbient = vec3(0.2, 0.05, 0.4) * 0.1;
    vec3 ambient = 0.03 * albedo * globalAmbient * ao;

    color = ambient + Lo;
    
    color = color / (color + 1.0);
    
    // tonemap and apply exposure
    color = tonemap(color) * lighting.exposure;

    // convert color back to sRGB
    color.r = pow(color.r, 1.0 / lighting.gamma);
    color.g = pow(color.g, 1.0 / lighting.gamma);
    color.b = pow(color.b, 1.0 / lighting.gamma);

    outColor = vec4(color, baseMap.a);
}