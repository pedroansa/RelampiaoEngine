#version 450

layout(std140, set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
} ubo;

layout(set = 1, binding = 0) uniform samplerCube skybox;
layout(location = 0) in vec2 inNDC;
layout(location = 0) out vec4 outColor;

void main() {
    // 1. Transformamos o NDC de volta para o espaço da câmera (View Space)
    // Usamos a inversa da projeção
    vec4 target = inverse(ubo.projection) * vec4(inNDC, 1.0, 1.0);
    
    // 2. No View Space, a direção do raio é o vetor que sai da origem (0,0,0) 
    // em direção ao plano de projeção.
    // Importante: normalize(target.xyz) aqui nos dá o vetor no espaço da câmera.
    vec3 rayView = normalize(target.xyz);
    
    // 3. Agora rotacionamos esse raio para o World Space usando a INVERSA da VIEW.
    // Usamos mat3 para garantir que a posição da câmera (translação) seja ignorada.
    // Isso faz com que o céu pareça estar a uma distância infinita.
    vec3 rayWorld = mat3(ubo.inverseView) * rayView;

    // 4. Amostra a Cubemap
    // Se o movimento horizontal parecer invertido, coloque um sinal de menos no rayWorld.x
    outColor = texture(skybox, rayWorld);
}