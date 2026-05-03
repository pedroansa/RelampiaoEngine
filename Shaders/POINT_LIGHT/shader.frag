#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w is intensity
  vec4 data; // x is radius, yzw are padding
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor; // w is intensity
  vec4 numLightsAndPad;
  PointLight pointLights[10];
} ubo;

layout(push_constant) uniform Push {
  vec4 position;
  vec4 color;
  float radius;
  float padding[3];
} push;

void main() {
  float dis = sqrt(dot(fragOffset, fragOffset));
  if (dis >= 1.0) {
    discard;
  }
  outColor = vec4(push.color.xyz, 1.0);
}
