#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

#define MAXLIGHTS 32
#define MAXSHADOWCASTERS 4
#define MAXCASCADES 4
#define SHADOWFOVMULT 0.5
#define UNSHADOWEDVALUE 0.3
#define SUNLIGHTINDEX 0
#define BLOOMTHRESHOLD 0.86

#define COLORMAP	0
#define NORMALMAP	1
#define BLOOMMAP	1
#define PHYSMAP		2
#define AOMAP       3
#define POSITIONMAP	3
#define EMISMAP		4
#define EXTRAMAP	5
#define MAXTEXTURES 6

#define RGB_TO_LUM vec3(0.2126, 0.7152, 0.0722)

const float M_PI = 3.141592653589793;
const float TWO_PI = M_PI * 2.0;
const float HALF_PI = M_PI * 0.5;

// Shadow cascade data
// Distances should be proportionally tied to the size of the shadow view frustum
// By default it's nextDistance = prevDistance * 2. First distance value is 0.
const float cascadeDistances[MAXCASCADES] = { 0.0, 2.0, 4.0, 8.0 };

float max3(vec3 v) {
  return max(max(v.x, v.y), v.z);
}

vec3 cubePixelToWorldPosition(ivec3 cubeCoord, vec2 cubemapSize) {
    vec2 texCoord = vec2(cubeCoord.xy) / cubemapSize;
    texCoord = texCoord  * 2.0 - 1.0; // -1..1
    switch(cubeCoord.z)
    {
        case 0: return vec3(1.0, -texCoord.yx);             // X+
        case 1: return vec3(-1.0, -texCoord.y, texCoord.x); // X-
        case 2: return vec3(texCoord.x, 1.0, texCoord.y);   // Y+
        case 3: return vec3(texCoord.x, -1.0, -texCoord.y); // Y-
        case 4: return vec3(texCoord.x, -texCoord.y, 1.0);  // Z+
        case 5: return vec3(-texCoord.xy, -1.0);            // Z-
    }

    return vec3(0.0);
}

ivec3 cubeWorldToPixelPosition(vec3 texCoord, vec2 cubemapSize) {
    vec3 abst = abs(texCoord);
    texCoord /= max3(abst);

    float cubeFace;
    vec2 uvCoord;
    if (abst.x > abst.y && abst.x > abst.z) 
    {
        // x major
        float negx = step(texCoord.x, 0.0);
        uvCoord = mix(-texCoord.zy, vec2(texCoord.z, -texCoord.y), negx);
        cubeFace = negx;
    } 
    else if (abst.y > abst.z) 
    {
        // y major
        float negy = step(texCoord.y, 0.0);
        uvCoord = mix(texCoord.xz, vec2(texCoord.x, -texCoord.z), negy);
        cubeFace = 2.0 + negy;
    } 
    else 
    {
        // z major
        float negz = step(texCoord.z, 0.0);
        uvCoord = mix(vec2(texCoord.x, -texCoord.y), -texCoord.xy, negz);
        cubeFace = 4.0 + negz;
    }

    uvCoord = (uvCoord + 1.0) * 0.5; // 0..1
    uvCoord = uvCoord * cubemapSize;
    uvCoord = clamp(uvCoord, vec2(0.0), cubemapSize - vec2(1.0));

    return ivec3(ivec2(uvCoord), int(cubeFace));
}

// Based on http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(vec2 co)
{
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt= dot(co.xy ,vec2(a,b));
	float sn= mod(dt,3.14);
	return fract(sin(sn) * c);
}

vec2 hammersley2D(uint i, uint N) 
{
	// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return vec2(float(i) /float(N), rdi);
}

// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
vec3 importanceSampleGGX(vec2 Xi, float roughness, vec3 normal) 
{
	// Maps a 2D point to a hemisphere with spread based on roughness
	float alpha = roughness * roughness;
	float phi = 2.0 * M_PI * Xi.x + random(normal.xz) * 0.1;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

	// Tangent space
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangentX = normalize(cross(up, normal));
	vec3 tangentY = normalize(cross(normal, tangentX));

	// Convert to world Space
	return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

// Normal Distribution function
float distributionGGX(float NdotH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(M_PI * denom*denom); 
}

// ===== TONEMAPPING =================================================

//
//  ACES Full
//

const mat3 ACES_InputMatrix = mat3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

mat3 ACES_OutputMatrix = mat3(
    1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
);

vec3 ACES_Mul(mat3 m, vec3 v) {
    float x = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2];
    float y = m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2];
    float z = m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2];
    return vec3(x, y, z);
}

vec3 ACES_RDTAndODTFit(vec3 v) {
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 tonemapACES(vec3 v) {
    v = ACES_Mul(ACES_InputMatrix, v);
    v = ACES_RDTAndODTFit(v);
    return ACES_Mul(ACES_OutputMatrix, v);
}

//
//  ACES Approximation
//

vec3 tonemapACESApprox(vec3 v) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    return clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0, 1.0);
}

//
// AMD Tonemapper (from Khronos ShaderLibVK) 
//

// General tonemapping operator, build 'b' term.
float ColToneB(float hdrMax, float contrast, float shoulder, float midIn, float midOut) 
{
    return
        -((-pow(midIn, contrast) + (midOut*(pow(hdrMax, contrast*shoulder)*pow(midIn, contrast) -
            pow(hdrMax, contrast)*pow(midIn, contrast*shoulder)*midOut)) /
            (pow(hdrMax, contrast*shoulder)*midOut - pow(midIn, contrast*shoulder)*midOut)) /
            (pow(midIn, contrast*shoulder)*midOut));
}

// General tonemapping operator, build 'c' term.
float ColToneC(float hdrMax, float contrast, float shoulder, float midIn, float midOut) 
{
    return (pow(hdrMax, contrast*shoulder)*pow(midIn, contrast) - pow(hdrMax, contrast)*pow(midIn, contrast*shoulder)*midOut) /
           (pow(hdrMax, contrast*shoulder)*midOut - pow(midIn, contrast*shoulder)*midOut);
}

// General tonemapping operator, p := {contrast,shoulder,b,c}.
float ColTone(float x, vec4 p) 
{ 
    float z = pow(x, p.r); 
    return z / (pow(z, p.g)*p.b + p.a); 
}

vec3 tonemapAMD(vec3 color)
{
    const float hdrMax = 16.0; // How much HDR range before clipping. HDR modes likely need this pushed up to say 25.0.
    const float contrast = 2.0; // Use as a baseline to tune the amount of contrast the tonemapper has.
    const float shoulder = 1.0; // Likely don’t need to mess with this factor, unless matching existing tonemapper is not working well..
    const float midIn = 0.18; // most games will have a {0.0 to 1.0} range for LDR so midIn should be 0.18.
    const float midOut = 0.18; // Use for LDR. For HDR10 10:10:10:2 use maybe 0.18/25.0 to start. For scRGB, I forget what a good starting point is, need to re-calculate.

    float b = ColToneB(hdrMax, contrast, shoulder, midIn, midOut);
    float c = ColToneC(hdrMax, contrast, shoulder, midIn, midOut);

    #define EPS 1e-6f
    float peak = max(color.r, max(color.g, color.b));
    peak = max(EPS, peak);

    vec3 ratio = color / peak;
    peak = ColTone(peak, vec4(contrast, shoulder, b, c) );
    // then process ratio

    // probably want send these pre-computed (so send over saturation/crossSaturation as a constant)
    float crosstalk = 4.0; // controls amount of channel crosstalk
    float saturation = contrast; // full tonal range saturation control
    float crossSaturation = contrast*16.0; // crosstalk saturation

    float white = 1.0;

    // wrap crosstalk in transform
    ratio = pow(abs(ratio), vec3(saturation / crossSaturation));
    ratio = mix(ratio, vec3(white), vec3(pow(peak, crosstalk)));
    ratio = pow(abs(ratio), vec3(crossSaturation));

    // then apply ratio to peak
    color = peak * ratio;
    return color;
}

//
//  Reinhard with a white point
//

vec3 tonemapReinhardWP(vec3 v, float whitePoint) {
	vec3 numerator = v * (1.0 + (v / vec3(whitePoint * whitePoint)));
    return numerator / (1.0 + v);
}