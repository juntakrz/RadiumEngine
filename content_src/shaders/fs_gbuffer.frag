#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "include/common.glsl"
#include "include/fragment.glsl"

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;
layout (location = 5) in vec4 inCurrentWorldPos;
layout (location = 6) in vec4 inPrevWorldPos;

// Per Instance
layout (location = 7) flat in uint inMaterialIndex;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outPhysical;		// x = metalness, y = roughness, z = ambient occlusion
layout (location = 4) out vec4 outEmissive;
layout (location = 5) out vec2 outVelocity;

// scene bindings
layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
	vec2 haltonJitter;
	vec2 clipData;			// x = near plane, y = far plane
} scene;

const float minRoughness = 0.04;

vec2 getVelocity(vec4 prevPos, vec4 currentPos){
	prevPos /= prevPos.w;
	prevPos.xy = (prevPos.xy - 1.0) * 0.5;
	prevPos.y = 1 - prevPos.y;

	currentPos /= currentPos.w;
	currentPos.xy = (currentPos.xy - 1.0) * 0.5;
	currentPos.y = 1 - currentPos.y;

	return (currentPos - prevPos).xy;
}

vec3 getNormal(int textureSet) {
	vec3 tangentNormal = vec3(vec2(texture(samplers[materialBlocks[inMaterialIndex].samplerIndex[NORMALMAP]],
		textureSet == 0 ? inUV0 : inUV1).rg * 2.0 - 1.0), 1.0);

	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(inUV0);
	vec2 st2 = dFdy(inUV0);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal) * materialBlocks[inMaterialIndex].bumpIntensity;
}

void main() {
	float perceptualRoughness;
	float metallic;

	// Initialize outgoing data
	outPosition = vec4(0.0);
	outColor = vec4(1.0);
    outNormal = vec4(0.0);
	outPhysical = vec4(0.0);
	outEmissive = vec4(0.0);

	// 1. Extract fragment position in world space as well as its linear depth
	float depth = gl_FragCoord.z * 2.0 - 1.0;
	depth = (2.0 * scene.clipData.x * scene.clipData.y) / (scene.clipData.y + scene.clipData.x - depth * (scene.clipData.y - scene.clipData.x));

	outPosition = vec4(inWorldPos, depth);

	// 2. Extract color / diffuse / albedo
	int textureSet = getTextureSet(COLORMAP, inMaterialIndex);
	if (textureSet > -1) {
		outColor = texture(samplers[materialBlocks[inMaterialIndex].samplerIndex[COLORMAP]], textureSet == 0 ? inUV0 : inUV1);
	}
	
	// apply vertex colors if present
	outColor *= inColor0;

	// 3. Extract normal for this fragment
	textureSet = getTextureSet(NORMALMAP, inMaterialIndex);
	vec3 normal = (textureSet > -1) ? getNormal(textureSet) : normalize(inNormal);
	outNormal = vec4(normal, 1.0) * materialBlocks[inMaterialIndex].bumpIntensity;

	// 4. Extract physical properties
	
	// Metallic and Roughness material properties are packed together
	// In glTF, these factors can be specified by fixed scalar values
	// or from a metallic-roughness map
	textureSet = getTextureSet(PHYSMAP, inMaterialIndex);
	perceptualRoughness = materialBlocks[inMaterialIndex].roughnessFactor;
	metallic = materialBlocks[inMaterialIndex].metallicFactor;
	if (textureSet > -1) {
		// In texture roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
		// This layout intentionally reserves the 'r' channel for (optional) ambient occlusion map data
		vec4 physicalSample = texture(samplers[materialBlocks[inMaterialIndex].samplerIndex[PHYSMAP]], textureSet == 0 ? inUV0 : inUV1);
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

	textureSet = getTextureSet(AOMAP, inMaterialIndex);
	if (textureSet > -1) {
		float ao = texture(samplers[materialBlocks[inMaterialIndex].samplerIndex[AOMAP]], (textureSet == 0 ? inUV0 : inUV1)).r;
		outPhysical.b = ao;
	}

	// 5. Extract emissive color
	textureSet = getTextureSet(EMISMAP, inMaterialIndex);
	if (textureSet > -1) {
		vec3 emissive = texture(samplers[materialBlocks[inMaterialIndex].samplerIndex[EMISMAP]], textureSet == 0 ? inUV0 : inUV1).rgb;
		outEmissive.rgb = emissive;
	}

	// Brighten or darken emission by the glow color value
	outEmissive.rgb += materialBlocks[inMaterialIndex].glowColor.rgb;

	// Storing polygon facing to account for dual sided faces in a PBR pass
	outEmissive.a = (gl_FrontFacing) ? 1.0 : 0.8;

	// 6. Store velocity vector
	outVelocity = getVelocity(inPrevWorldPos, inCurrentWorldPos);
}