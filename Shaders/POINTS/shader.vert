#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

struct PointLight {
    vec4 position;  // offset 0
    vec4 color;     // offset 16
    float radius;   // offset 32
    vec3 padding;   // offset 36
};

layout(std140, set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;        // offset 0
    mat4 view;              // offset 64
    mat4 inverseView;       // offset 128
    vec4 ambientLightColor; // offset 192
    PointLight pointLights[10]; // offset 208
    int numLights;          // offset 688
    vec3 padding;           // offset 692
} ubo;

layout(push_constant) uniform Push{
	mat4 modelMatrix; 
	mat4 normalMatrix;
} push;

void main() {
    gl_Position = ubo.projection * ubo.view * push.modelMatrix * vec4(position, 1.0);    
    // Para pontos, use cor sólida ou cor do vértice
    fragColor = vec3(1.0, 1.0, 1.0); // Branco fixo para pontos
    
    // Tamanho dos pontos em pixels
    gl_PointSize = 2.0;
}