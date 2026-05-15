#version 450

layout(location = 0) out vec2 inNDC;

void main() {
    // Triângulo que cobre a tela inteira
    vec2 pos[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    gl_Position = vec4(pos[gl_VertexIndex], 1.0, 1.0); // Z = 1.0 para ficar ao fundo
    inNDC = pos[gl_VertexIndex];
}