#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// Textures — binding 1 = albedo (always present)
// bindings 2-5 = PBR maps (optional, white fallback if not set)
layout(set = 1, binding = 1) uniform sampler2D albedoMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;
layout(set = 1, binding = 3) uniform sampler2D metallicMap;
layout(set = 1, binding = 4) uniform sampler2D roughnessMap;
layout(set = 1, binding = 5) uniform sampler2D aoMap;

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
    vec4 directionalLightDir;   // xyz = direction, w = intensity
    vec4 directionalLightColor; // xyz = color
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    float uvScale;
    float padding[3];
} push;

const float PI = 3.14159265359;

float distributionGGX(float NdotH, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * d * d);
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(float NdotV, float NdotL, float roughness) {
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec2 uv = vec2(1.0 - fragUV.x, fragUV.y) * push.uvScale;

    // Sample textures
    vec4 albedoSample   = texture(albedoMap,    uv);
    float metallic      = texture(metallicMap,  uv).r;
    float roughness     = clamp(texture(roughnessMap, uv).r, 0.05, 1.0);
    float ao            = texture(aoMap, uv).r;

    // sRGB to linear
    vec3 albedo = albedoSample.rgb * fragColor;

    // Solar system sun shortcut — kept from original
    float distanceFromCenter = length(fragPosWorld);
    bool isSolarSystem = false;
    bool isSun = (distanceFromCenter < 1000.0);
    if (isSun && isSolarSystem) {
        vec3 sunColor = vec3(1.0, 0.9, 0.5);
        outColor = vec4(sunColor * 1.2 * albedo, 1.0);
        return;
    }

    // --- CORREÇÃO DO MAPA DE NORMAIS DIRECTX/OPENGL MISTO ---
    vec3 geomNormal = normalize(fragNormalWorld);
    
    // Obtém o relevo da textura transformando de [0, 1] para [-1, 1]
    vec3 normalSample = texture(normalMap, uv).xyz * 2.0 - 1.0;
    
    // Como você baixou o pacote do Blender (-bl), o mapa de normais já é nativo OpenGL.
    // Combinamos a inclinação da textura de relevo diretamente com a direção da face.
    vec3 N = normalize(geomNormal + vec3(normalSample.xy * 0.35, 0.0));
    // ---------------------------------------------------------

    vec3 V = normalize(ubo.inverseView[3].xyz - fragPosWorld);

    // F0 — base reflectivity (0.04 for dielectrics, albedo for metals)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    int numLights = clamp(int(ubo.numLightsAndPad.x), 0, 10);

    vec3 dirL         = normalize(-ubo.directionalLightDir.xyz);
    float dirIntensity = ubo.directionalLightDir.w;
    vec3  dirColor    = ubo.directionalLightColor.xyz;

    float dirNdotL = max(dot(N, dirL), 0.0);
    vec3  H_dir    = normalize(V + dirL);
    float dirNdotH = max(dot(N, H_dir), 0.0);
    float HdotV_dir = max(dot(H_dir, V), 0.0);

    // Cook-Torrance for directional light
    float D_dir = distributionGGX(dirNdotH, roughness);
    float G_dir = geometrySmith(max(dot(N,V),0.0), dirNdotL, roughness);
    vec3  F_dir = fresnelSchlick(HdotV_dir, F0);

    vec3 spec_dir    = (D_dir * G_dir * F_dir) / max(4.0 * max(dot(N,V),0.0) * dirNdotL, 0.001);
    vec3 kD_dir      = (vec3(1.0) - F_dir) * (1.0 - metallic);
    vec3 diffuse_dir = kD_dir * albedo / PI;

    Lo += (diffuse_dir + spec_dir) * dirColor * dirIntensity * dirNdotL;

    for (int i = 0; i < numLights; i++) {
        vec3  toLight     = ubo.pointLights[i].position.xyz - fragPosWorld;
        float dist        = length(toLight);
        vec3  L           = normalize(toLight);
        vec3  H           = normalize(V + L);
        float radius      = max(ubo.pointLights[i].data.x, 0.001);
        float attenuation = 1.0 / (1.0 + (dist * dist) / (radius * radius));
        vec3  radiance    = ubo.pointLights[i].color.xyz * ubo.pointLights[i].color.w * attenuation;

        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);

        // Cook-Torrance BRDF
        float D = distributionGGX(NdotH, roughness);
        float G = geometrySmith(NdotV, NdotL, roughness);
        vec3  F = fresnelSchlick(HdotV, F0);

        vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);
        vec3 kD       = (vec3(1.0) - F) * (1.0 - metallic);
        vec3 diffuse  = kD * albedo / PI;

        Lo += (diffuse + specular) * radiance * NdotL;
    }

    // Ambient + AO
    vec3 ambient = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w * albedo * ao;
    vec3 color   = ambient + Lo;

    // Tone mapping + gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}