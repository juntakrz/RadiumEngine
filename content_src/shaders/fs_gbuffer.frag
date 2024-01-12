#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;

// scene bindings
layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
} scene;

// material bindings
layout (set = 2, binding = 0) uniform sampler2D colorMap;
layout (set = 2, binding = 1) uniform sampler2D normalMap;
layout (set = 2, binding = 2) uniform sampler2D physicalDescriptorMap;
layout (set = 2, binding = 3) uniform sampler2D aoMap;
layout (set = 2, binding = 4) uniform sampler2D emissiveMap;
layout (set = 2, binding = 5) uniform sampler2D extraMap;		// TODO: implement extra map in shader

layout (set = 3, binding = 0) uniform sampler2D samplers[];

layout (push_constant) uniform Material {
	layout(offset = 16) 
	int baseColorTextureSet;
	int physicalDescriptorTextureSet;
	int normalTextureSet;	
	int occlusionTextureSet;	// 16
	int emissiveTextureSet;
	int extraTextureSet;
	float metallicFactor;	
	float roughnessFactor;		// 32
	float alphaMask;	
	float alphaMaskCutoff;
	float bumpIntensity;
	float emissiveIntensity;	// 48
	int materialIndex;
} material;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outPhysical;		// x = metalness, y = roughness, z = ambient occlusion
layout (location = 4) out vec4 outEmissive;

const float minRoughness = 0.04;

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

void main() {
	float perceptualRoughness;
	float metallic;

	// initialize outgoing data
	outPosition = vec4(0.0);
	outColor = vec4(1.0);
    outNormal = vec4(0.0);
	outPhysical = vec4(0.0);
	outEmissive = vec4(0.0);

	// 1. extract fragment position in world space
	outPosition = vec4(inWorldPos, 1.0);

	// 2. extract color / diffuse / albedo
	if (material.baseColorTextureSet > -1) {
		//outColor = texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1);
		outColor = texture(samplers[material.materialIndex], material.baseColorTextureSet == 0 ? inUV0 : inUV1);
	}

	// TODO: discard on alphaMask == 1.0 here and depth-sort everything, unless requested by the material
	if (material.alphaMask > 0.9 && outColor.a < material.alphaMaskCutoff && material.alphaMaskCutoff < 1.1) {
		discard;
	}

	// apply vertex colors if present
	outColor *= inColor0;

	// 3. extract normal for this fragment
	vec3 normal = (material.normalTextureSet > -1) ? getNormal() : normalize(inNormal);
	outNormal = vec4(normal, 1.0) * material.bumpIntensity;

	// 4. extract physical properties
	
	// Metallic and Roughness material properties are packed together
	// In glTF, these factors can be specified by fixed scalar values
	// or from a metallic-roughness map
	perceptualRoughness = material.roughnessFactor;
	metallic = material.metallicFactor;
	if (material.physicalDescriptorTextureSet > -1) {
		// In texture roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
		// This layout intentionally reserves the 'r' channel for (optional) ambient occlusion map data
		vec4 physicalSample = texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1);
		physicalSample.g = pow(physicalSample.g, 1.0 / 2.2);
		physicalSample.b = pow(physicalSample.b, 1.0 / 2.2);
		perceptualRoughness = physicalSample.g * perceptualRoughness;
		metallic = physicalSample.b * metallic;
	} else {
		perceptualRoughness = clamp(perceptualRoughness, minRoughness, 1.0);
		metallic = clamp(metallic, 0.0, 1.0);
	}
	
	outPhysical.r = metallic;
	outPhysical.g = perceptualRoughness;
    outPhysical.b = 1.0;

	if (material.occlusionTextureSet > -1) {
		float ao = texture(aoMap, (material.occlusionTextureSet == 0 ? inUV0 : inUV1)).r;
		outPhysical.b = ao;
	}

	// 5. extract emissive color
	if (material.emissiveTextureSet > -1) {
		vec3 emissive = texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1).rgb;
		outEmissive = vec4(emissive, 1.0);
	}

	// brighten or darken color emission
	outEmissive += vec4(material.emissiveIntensity);
}