// Generates an irradiance cube from an environment map using convolution

#version 450

layout (location = 0) in vec3 inPos;
layout (location = 0) out vec4 outColor;

layout (set = 2, binding = 0) uniform samplerCube samplerEnv;

const float M_PI = 3.141592653589793;
const float deltaPhi = 0.034906585;
const float deltaTheta = 0.024543693;

void main()
{
	vec3 N = normalize(inPos);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = cross(N, right);

	const float TWO_PI = M_PI * 2.0;
	const float HALF_PI = M_PI * 0.5;

	vec3 color = vec3(0.0);
	uint sampleCount = 0u;
	for (float phi = 0.0; phi < TWO_PI; phi += deltaPhi) {
		for (float theta = 0.0; theta < HALF_PI; theta += deltaTheta) {
			vec3 tempVec = cos(phi) * right + sin(phi) * up;
			vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;
			color += texture(samplerEnv, sampleVector).rgb * cos(theta) * sin(theta);
			sampleCount++;
		}
	}
	outColor = vec4(M_PI * color / float(sampleCount), 1.0);
}
