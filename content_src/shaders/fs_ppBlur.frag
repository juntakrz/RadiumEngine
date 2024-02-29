#version 460

#include "include/common.glsl"
#include "include/fragment.glsl"

#define BLUR_BLURMAP 0
#define BLUR_AOMAP 3
#define KERNEL_RADIUS 3

layout(location = 0) in vec2 inUV;

layout (location = 0) out vec4 outBlur;

layout (set = 2, binding = 0) uniform sampler2D samplers[];
layout (set = 2, binding = 0) uniform sampler2DArray arraySamplers[];

// The higher the value - the higher the mix of ambient occlusion
const float occlusionBias = 0.25;
const float blurSharpness = 10.0;

float blurFunctionAO(int imageIndex, vec2 UV, float r, float centerDepth, inout float weightTotal) {
  vec2 cd = texture(samplers[material.samplerIndex[imageIndex]], UV).rg;
  float c = cd.x;
  float d = cd.y;
  
  const float blurSigma = float(KERNEL_RADIUS) * 0.5;
  const float blurFalloff = 1.0 / (2.0 * blurSigma * blurSigma);
  
  float dDiff = (d - centerDepth) * blurSharpness;
  float w = exp2(-r * r * blurFalloff - dDiff * dDiff);
  weightTotal += w;

  return c * w;
}

float blurAO(int imageIndex) {
    vec2 colorAndDepth = texture(samplers[material.samplerIndex[imageIndex]], inUV).rg;
    const vec2 texelSize = 1.0 / vec2(textureSize(samplers[material.samplerIndex[imageIndex]], 0));
    float centerColor = colorAndDepth.x;
    float centerDepth = colorAndDepth.y;
  
    float colorTotal = centerColor;
    float weightTotal = 1.0;

    vec2 direction = (material.textureSets == 0) ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);
  
    for (float r = 1; r <= KERNEL_RADIUS; ++r) {
        vec2 UV = inUV + direction * r;
        colorTotal += blurFunctionAO(imageIndex, UV, r, centerDepth, weightTotal);  
    }
  
    for (float r = 1; r <= KERNEL_RADIUS; ++r) {
        vec2 UV = inUV - direction * r;
        colorTotal += blurFunctionAO(imageIndex, UV, r, centerDepth, weightTotal);  
    }

    return colorTotal / weightTotal;
}

vec3 gaussianBlurAO(int imageIndex, int direction) {
    float blurredColor = 0.0;
    const vec2 texelSize = 1.0 / vec2(textureSize(samplers[material.samplerIndex[imageIndex]], 0));
    const float blurRadius = 2.0;
    float w0 = 0.5135 / pow(blurRadius, 0.96);
    float radiusSquared = blurRadius * blurRadius;
    vec2 offset = vec2(0.0);

    for (float d = -blurRadius; d <= blurRadius; d++) {
        switch (direction) {
            case 0: {
                offset.x = d * texelSize.x;
                break;
            }
            default: {
                offset.y = d * texelSize.y;
                break;
            }
        };
        float w = w0 * exp((-d * d) / (2.0 * radiusSquared));
        blurredColor += textureLod(samplers[material.samplerIndex[imageIndex]], inUV + offset, 0).r * w;
    }

    return vec3(blurredColor, 0.0, 0.0);
}

void main() {
    switch (material.textureSets) {
        // AO Blur, pass 0 - get data from the AO map and output to the Blur map
        case 0: {
            float depth = textureLod(samplers[material.samplerIndex[BLUR_AOMAP]], inUV, 0).g;
            outBlur = vec4(blurAO(BLUR_AOMAP), depth, 0.0, 1.0);
            break;
        }
        // AO Blur, pass 1 - get data from the Blur map and output to the AO map
        default: {
            float depth = textureLod(samplers[material.samplerIndex[BLUR_BLURMAP]], inUV, 0).g;
            outBlur = vec4(blurAO(BLUR_BLURMAP), depth, 0.0, 1.0);
        }
    };
}