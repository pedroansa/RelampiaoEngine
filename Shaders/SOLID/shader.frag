#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUV;

layout(location = 0) out vec4 outColor;
layout(set = 1, binding = 1) uniform sampler2D texSampler;

struct PointLight {
    vec4 position;
    vec4 color;
    vec4 data;
};

layout(std140, set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
    vec4 numLightsAndPad;
    PointLight pointLights[10];
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    float distanceFromCenter = length(fragPosWorld);
    bool isSolarSystem = false;
    bool isSun = (distanceFromCenter < 1000.0);

    vec4 texColor = texture(texSampler, fragUV);


    if (isSun && isSolarSystem) {
        // Sol brilha sozinho, sem iluminação externa
        vec3 sunColor = vec3(1.0, 0.9, 0.5);  // Amarelo quente
        float sunGlow = 1.2;  // Brilho extra
        outColor = vec4(sunColor * sunGlow * texColor.rgb, 1.0);
        return;  // Sai da função, não calcula iluminação
    }
    vec3 ambient = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    vec3 N = normalize(fragNormalWorld);

    vec3 cameraPosWorld = ubo.inverseView[3].xyz;
    vec3 V = normalize(cameraPosWorld - fragPosWorld);

    int numLights = clamp(int(ubo.numLightsAndPad.x), 0, 10);

    for (int i = 0; i < numLights; i++) {
        vec3 toLight = ubo.pointLights[i].position.xyz - fragPosWorld;
        float dist = length(toLight);
        vec3 L = normalize(toLight);

        float radius = max(ubo.pointLights[i].data.x, 0.001);
        float attenuation = 1.0 / (1.0 + (dist * dist) / (radius * radius));

        vec3 lightColor = ubo.pointLights[i].color.xyz * ubo.pointLights[i].color.w;

        float NdotL = max(dot(N, L), 0.0);
        diffuse += lightColor * NdotL * attenuation;

        vec3 H = normalize(L + V);
        float shininess = 32.0;
        float NdotH = max(dot(N, H), 0.0);
        float specTerm = pow(NdotH, shininess);
    
        specular += lightColor * specTerm * attenuation;
    }

  outColor = vec4((ambient + diffuse) * fragColor * texColor.rgb, 1.0);
}