#version 460

#include "include/common.glsl"
#include "include/fragment.glsl"

layout(location = 0) in vec2 inUV0;

layout (location = 0) out vec4 outColor;

// Scene bindings
layout (set = 0, binding = 0) uniform UBOScene {
	mat4 view;
	mat4 projection;
	vec3 camPos;
} scene;


layout (set = 2, binding = 0) uniform sampler2D samplers[];
layout (set = 2, binding = 0) uniform sampler2DArray arraySamplers[];

void main() {
	ivec2 bufferCoords = ivec2(gl_FragCoord.xy);
    vec4 accum = texelFetch(samplers[material.samplerIndex[COLORMAP]], bufferCoords, 0);
    float revealage = accum.a;

    accum.a = texelFetch(samplers[material.samplerIndex[NORMALMAP]], bufferCoords, 0).r;
    // suppress underflow
    if (isinf(accum.a)) {
        accum.a = max(max(accum.r, accum.g), accum.b);
    }
    // suppress overflow
    if (any(isinf(accum.rgb))) {
        accum = vec4(isinf(accum.a) ? 1.0 : accum.a);
    }
    vec3 averageColor = accum.rgb / max(accum.a, 1e-4);
    // dst' = (accum.rgb / accum.a) * (1 - revealage) + dst * revealage
    outColor = vec4(averageColor, revealage);
}