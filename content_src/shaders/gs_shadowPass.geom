#version 460

#define MAXCASCADES 3

layout (triangles, invocations = MAXCASCADES) in;
layout (triangle_strip, max_vertices = 3) out;

layout(location = 2) in vec2 inUV0[];

layout(location = 2) out vec2 outUV0;

const mat4 projection = mat4(
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 0.01, -0.00001,
	0.0, 0.0, 0.0, 1.0);

void main() {
	float multiplier = 1.0;

	for (uint i = 0; i < gl_InvocationID; i++) {
		multiplier *= 0.25;
	}

	mat4 newProjection = projection;
	newProjection[0][0] *= multiplier;	
	newProjection[1][1] *= multiplier;

	for (int i = 0; i < gl_in.length(); i++) {
		outUV0 = inUV0[i];

		gl_Layer = gl_InvocationID;
		gl_Position = newProjection * gl_in[i].gl_Position;
		EmitVertex();
	}

	EndPrimitive();
}