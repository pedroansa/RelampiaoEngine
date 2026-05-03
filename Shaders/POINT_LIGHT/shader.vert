#version 450

const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w is intensity
  vec4 data; // x is radius, yzw are padding
};

layout (location = 0) out vec2 fragOffset;

layout(std140, set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;        // offset 0
    mat4 view;              // offset 64
    mat4 inverseView;       // offset 128
    vec4 ambientLightColor;
    vec4 numLightsAndPad;
    PointLight pointLights[10];          // offset 692
} ubo;

layout(push_constant) uniform Push {
  vec4 position;
  vec4 color;
  float radius;
  float padding[3];
} push;

const float LIGHT_RADIUS = 0.05;

void main() {
  fragOffset = OFFSETS[gl_VertexIndex];
  vec3 cameraRightWorld = {ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]};
  vec3 cameraUpWorld = {ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]};

  vec3 positionWorld = push.position.xyz
    + push.radius * fragOffset.x * cameraRightWorld
    + push.radius * fragOffset.y * cameraUpWorld;

  gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);
}