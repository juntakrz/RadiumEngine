#version 460

layout (location = 0) out vec2 outUV;

// Full-screen quad vertices
const vec2 vertices[6] = vec2[](
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

void main() {
    // Vertex position in clip space
    gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);

    // Calculate UV coordinates based on vertex position
    outUV.x = (vertices[gl_VertexIndex].x + 1.0) * 0.5;
    outUV.y = (1.0 - vertices[gl_VertexIndex].y) * 0.5;
}