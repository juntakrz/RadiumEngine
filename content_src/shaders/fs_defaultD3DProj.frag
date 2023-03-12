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
    //float a2 = roughness * roughness * roughness * roughness;
    float a2 = roughness * roughness * roughness;
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

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 dirLightPos = vec3(-10.0, 0.0, 10.0);
    vec3 dirLightColor = vec3(1.0);
    float dirLightIntensity = 1.0;
    float matIntensity = 1.0;
	
    //init values    
    vec4 baseMap = texture(baseColorMap, inTexCoord0);
    //baseMap.r = pow(baseMap.r, 2.2);
    //baseMap.g = pow(baseMap.g, 2.2);
    //baseMap.b = pow(baseMap.b, 2.2);

    vec3 albedo = baseMap.rgb;
    vec3 normal = texture(normalMap, inTexCoord0).rgb;
    float metallic = texture(physicalMap, inTexCoord0).b * material.metallicFactor;
    float roughness = texture(physicalMap, inTexCoord0).g * material.roughnessFactor;
    float ao = texture(occlusionMap, inTexCoord0).r;
    
    vec3 L, radiance, color;
    
    vec3 Lo = { 0.0, 0.0, 0.0 };
    
    //calculate material composition
    vec3 F0 = mix(material.f0.rgb, albedo, metallic);
    
    //set normal map range from (0, +1) to (-1, +1)
    normal = (normal * 2.0) - 1.0;

    vec3 N = normalize(normal.x * inT + normal.y * -inB + normal.z * inNormal);
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
    vec3 F = fresnelSchlick(HdotV, F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL;
    vec3 specular = numerator / denominator;
        
    //conservation of energy, must not reflect more light than receives
    vec3 Kd = (1.0 - F) * (1.0 - metallic);
    
    Lo += (Kd * albedo / M_PI + specular) * radiance * NdotL;
    
    vec3 globalAmbient = vec3(0.2, 0.05, 0.4);
    vec3 ambient = 0.03 * albedo * globalAmbient * ao;

    color = ambient + Lo;
    
    color = color / (color + 1.0);
    //color.r = pow(color.r, 1.0 / 2.2);
    //color.g = pow(color.g, 1.0 / 2.2);
    //color.b = pow(color.b, 1.0 / 2.2);

    outColor = vec4(color, baseMap.a);
}