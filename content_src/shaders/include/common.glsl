#define MAXJOINTS 128
#define MAXLIGHTS 32
#define MAXSHADOWCASTERS 4
#define MAXCASCADES 3
#define SHADOWFOVMULT 0.5
#define SUNLIGHTINDEX 0

#define COLORMAP	0
#define NORMALMAP	1
#define PHYSMAP		2
#define AOMAP       3
#define POSITIONMAP	3
#define EMISMAP		4
#define EXTRAMAP	5
#define MAXTEXTURES 6

const float M_PI = 3.141592653589793;
const float TWO_PI = M_PI * 2.0;
const float HALF_PI = M_PI * 0.5;

// shadow cascade data
const float cascadeDistance0 = 2.0;
const float cascadeDistance1 = cascadeDistance0 * 2.0;
const float cascadeDistance2 = cascadeDistance1 * 2.0;

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