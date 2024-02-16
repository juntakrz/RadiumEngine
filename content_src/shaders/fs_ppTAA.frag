#version 460

#include "include/common.glsl"
#include "include/fragment.glsl"

#define FASTKERNEL

layout (location = 0) in vec2 inUV;

// Material bindings

layout (set = 2, binding = 0) uniform sampler2D samplers[];

layout (location = 0) out vec4 outColor;

void getTAAClampValues(vec3 color, vec2 offsetDelta, out vec3 boxMin, out vec3 boxMax) {
	const int range = 2;
	boxMin = color;
	boxMax = color;

	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			// Skip sampling the current fragment as it is already provided
			if (x == 0 && y == 0) continue;

			vec2 offset = vec2(offsetDelta.x * x, offsetDelta.y * y);
			vec3 sampleColor = texture(samplers[material.samplerIndex[COLORMAP]], inUV + offset).rgb;
			boxMin = min(boxMin, sampleColor);
			boxMax = max(boxMax, sampleColor);
		}
	}
}

void getTAAClampValuesFast(vec3 color, vec2 offsetDelta, out vec3 boxMin, out vec3 boxMax){
	vec3 neighbourColor0 = texture(samplers[material.samplerIndex[COLORMAP]], inUV + vec2(offsetDelta.x, 0.0)).rgb;
    vec3 neighbourColor1 = texture(samplers[material.samplerIndex[COLORMAP]], inUV + vec2(0.0, offsetDelta.y)).rgb;
    vec3 neighbourColor2 = texture(samplers[material.samplerIndex[COLORMAP]], inUV + vec2(-offsetDelta.x, 0.0)).rgb;
    vec3 neighbourColor3 = texture(samplers[material.samplerIndex[COLORMAP]], inUV + vec2(0.0, -offsetDelta.y)).rgb;
    
    boxMin = min(color, min(neighbourColor0, min(neighbourColor1, min(neighbourColor2, neighbourColor3))));
    boxMax = max(color, max(neighbourColor0, max(neighbourColor1, max(neighbourColor2, neighbourColor3))));;
}

vec3 sharpenColor(vec3 color, vec2 offsetDelta) {
	// Define a 3x3 sharpening kernel
	const mat3 sharpeningKernel = mat3(
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1
    );

	const int range = 1;
	vec3 sharpColor = color * float(sharpeningKernel[1][1]);

    // Apply the sharpening kernel
	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			// Skip sampling the current fragment as it is already provided
			if (x == 0 && y == 0) continue;

			vec2 offset = vec2(offsetDelta.x * x, offsetDelta.y * y);
			sharpColor += texture(samplers[material.samplerIndex[COLORMAP]], inUV + offset).rgb * float(sharpeningKernel[x + range][y + range]);
		}
	}

    // Output the sharpened color
    return max(sharpColor, 0.0);
}

void main() {
	ivec2 texDim = textureSize(samplers[material.samplerIndex[COLORMAP]], 0);
	vec2 offsetDelta = vec2(1.0 / float(texDim.x), 1.0 / float(texDim.y));

	vec3 color = texture(samplers[material.samplerIndex[COLORMAP]], inUV).rgb;
	vec3 sharpColor = sharpenColor(color, offsetDelta);

	color = mix(color, sharpColor, 0.1);

	// Get velocity UV displacement
	vec2 velocity = texture(samplers[material.samplerIndex[PHYSMAP]], inUV).rg;
	velocity = isnan(velocity.x) ? vec2(0.0) : velocity;
	vec2 newUV = inUV - velocity;

	// Apply clamped history color
	vec3 boxMin = vec3(0.0);
	vec3 boxMax = vec3(0.0);

#ifdef FASTKERNEL
	getTAAClampValuesFast(color, offsetDelta, boxMin, boxMax);
#else
	getTAAClampValues(color, offsetDelta, boxMin, boxMax);
#endif
	
	vec3 historyColor = texture(samplers[material.samplerIndex[AOMAP]], newUV).rgb;
	historyColor = clamp(historyColor, boxMin, boxMax);

	color = mix(color, historyColor, 0.9);

	outColor = vec4(color, 1.0);
}