#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

struct PointLight {
    vec4 position;  // offset 0
    vec4 color;     // offset 16
    vec4 data;      // offset 32, x is radius, yzw are padding
};

layout(std140, set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;        // offset 0
    mat4 view;              // offset 64
    mat4 inverseView;       // offset 128
    vec4 ambientLightColor; // offset 192
    vec4 numLightsAndPad;
    PointLight pointLights[10];
} ubo;

layout(push_constant) uniform Push{
	mat4 modelMatrix; 
	mat4 normalMatrix;
} push;

const float AMBIENT_LIGHT = 0.02;


void main(){
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);    
    gl_Position = ubo.projection * ubo.view * positionWorld;
	fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
	fragPosWorld = positionWorld.xyz;
	fragColor = color;
	}