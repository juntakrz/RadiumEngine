#version 460

layout (location = 0) out vec2 outUV;

void main() {
    // "Quad" is rendered on a triangle that spreads outside the screen space
    outUV = vec2(1 << gl_VertexIndex & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV.x * 2.0 - 1.0, outUV.y * -2.0 + 1.0, 0.0, 1.0);
}