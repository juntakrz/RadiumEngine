const float M_PI = 3.141592653589793;
const float TWO_PI = M_PI * 2.0;
const float HALF_PI = M_PI * 0.5;

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