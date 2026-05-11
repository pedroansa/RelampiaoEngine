#include "CpuRaytracer.h"
#include "Camera.h"
#include <iostream>
#include <algorithm>
#include <cmath>

// stb_image_write — header-only, coloque stb_image_write.h na sua pasta de headers
// Download: https://github.com/nothings/stb/blob/master/stb_image_write.h
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace app {

    // =========================================================================
    // Construtor
    // =========================================================================
    CpuRaytracer::CpuRaytracer(int width, int height, int maxBounces)
        : imageWidth(width), imageHeight(height), maxBounces(maxBounces) {}

    // =========================================================================
    // loadScene
    // =========================================================================
    void CpuRaytracer::loadScene(const GameObject::Map& gameObjects, const GlobalUbo& ubo) {
        triangles.clear();
        lights.clear();

        ambientLight = ubo.ambientLightColor;

        // --- Carrega luzes do UBO ---
        int numLights = static_cast<int>(ubo.numLightsAndPad.x);
        for (int i = 0; i < numLights && i < MAX_LIGHTS; i++) {
            SceneLight light{};
            light.position  = glm::vec3(ubo.pointLights[i].position);
            light.color     = glm::vec3(ubo.pointLights[i].color);
            light.intensity = ubo.pointLights[i].color.w;
            light.radius    = ubo.pointLights[i].data.x;
            lights.push_back(light);
        }

        // --- Carrega geometria dos GameObjects ---
        for (const auto& kv : gameObjects) {
            const auto& obj = kv.second;
            if (!obj->model || obj->pointLight != nullptr) continue; // pula luzes

            // Verifica se o modelo tem vertices em CPU
            if (obj->model->vertices.empty()) {
                std::cerr << "[CpuRaytracer] Model sem vertices em CPU. "
                          << "Adicione 'vertices' e 'indices' como membros publicos de Model "
                          << "e guarde uma copia no construtor de Model.\n";
                continue;
            }

            TransformComponent transform = obj->transform;
            glm::mat4 modelMatrix = transform.mat4();
            glm::mat3 normalMatrix = transform.normalMatrix();

            const auto& verts   = obj->model->vertices;
            const auto& indices = obj->model->indices;

            // Triangulos indexados
            if (!indices.empty()) {
                for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                    const auto& a = verts[indices[i]];
                    const auto& b = verts[indices[i + 1]];
                    const auto& c = verts[indices[i + 2]];

                    Triangle tri{};
                    tri.v0 = glm::vec3(modelMatrix * glm::vec4(a.position, 1.f));
                    tri.v1 = glm::vec3(modelMatrix * glm::vec4(b.position, 1.f));
                    tri.v2 = glm::vec3(modelMatrix * glm::vec4(c.position, 1.f));
                    tri.n0 = glm::normalize(normalMatrix * a.normal);
                    tri.n1 = glm::normalize(normalMatrix * b.normal);
                    tri.n2 = glm::normalize(normalMatrix * c.normal);
                    tri.c0 = a.color;
                    tri.c1 = b.color;
                    tri.c2 = c.color;
                    triangles.push_back(tri);
                }
            } else {
                // Triangulos nao indexados
                for (size_t i = 0; i + 2 < verts.size(); i += 3) {
                    Triangle tri{};
                    tri.v0 = glm::vec3(modelMatrix * glm::vec4(verts[i].position, 1.f));
                    tri.v1 = glm::vec3(modelMatrix * glm::vec4(verts[i+1].position, 1.f));
                    tri.v2 = glm::vec3(modelMatrix * glm::vec4(verts[i+2].position, 1.f));
                    tri.n0 = glm::normalize(normalMatrix * verts[i].normal);
                    tri.n1 = glm::normalize(normalMatrix * verts[i+1].normal);
                    tri.n2 = glm::normalize(normalMatrix * verts[i+2].normal);
                    tri.c0 = verts[i].color;   // <--- Use o índice i
                    tri.c1 = verts[i + 1].color; // <--- Use o índice i+1
                    tri.c2 = verts[i + 2].color; // <--- Use o índice i+2
                    triangles.push_back(tri);
                }
            }
        }

        std::cout << "[CpuRaytracer] Cena carregada: "
                  << triangles.size() << " triangulos, "
                  << lights.size() << " luzes.\n";
    }

    // =========================================================================
    // render
    // =========================================================================
    bool CpuRaytracer::render(const Camera& camera, float fovY, float aspect, const std::string& outPath) {
        std::cout << "[CpuRaytracer] Iniciando raytracing " << imageWidth << "x" << imageHeight << "...\n";

        std::vector<uint8_t> pixels(imageWidth * imageHeight * 3);

        // Reconstroi os vetores da camera a partir da inverseView
        // inverseView[3] = posicao da camera em world space
        // inverseView[0] = right, inverseView[1] = up, inverseView[2] = forward (negativo)
        glm::mat4 invView = camera.getInverseView();
        glm::vec3 camPos = glm::vec3(invView[3]);
        glm::vec3 camRight = glm::vec3(invView[0]);
        glm::vec3 camUp = -glm::vec3(invView[1]);
        glm::vec3 camFwd = glm::vec3(invView[2]); // remove o sinal negativo

        float halfH = std::tan(fovY * 0.5f);
        float halfW = halfH * aspect;

        int totalPixels = imageWidth * imageHeight;
        int lastPercent = -1;

        // Debug: testa um ray no centro da tela
        Ray centerRay{};
        centerRay.origin = camPos;
        centerRay.direction = camFwd;
        HitInfo centerHit = traceRay(centerRay);

        std::cout << "[Debug] camPos: " << camPos.x << ", " << camPos.y << ", " << camPos.z << "\n";
        std::cout << "[Debug] camFwd: " << camFwd.x << ", " << camFwd.y << ", " << camFwd.z << "\n";
        std::cout << "[Debug] camRight: " << camRight.x << ", " << camRight.y << ", " << camRight.z << "\n";
        std::cout << "[Debug] camUp: " << camUp.x << ", " << camUp.y << ", " << camUp.z << "\n";
        std::cout << "[Debug] Ray centro hit: " << (centerHit.hit ? "SIM" : "NAO") << "\n";
        if (centerHit.hit) {
            std::cout << "[Debug] Hit pos: " << centerHit.position.x << ", "
                << centerHit.position.y << ", " << centerHit.position.z << "\n";
            std::cout << "[Debug] Hit color: " << centerHit.color.x << ", "
                << centerHit.color.y << ", " << centerHit.color.z << "\n";
        }

        // Debug: imprime o primeiro triangulo
        if (!triangles.empty()) {
            const auto& t = triangles[0];
            std::cout << "[Debug] Tri[0] v0: " << t.v0.x << ", " << t.v0.y << ", " << t.v0.z << "\n";
            std::cout << "[Debug] Tri[0] v1: " << t.v1.x << ", " << t.v1.y << ", " << t.v1.z << "\n";
            std::cout << "[Debug] Tri[0] v2: " << t.v2.x << ", " << t.v2.y << ", " << t.v2.z << "\n";
        }

        for (int y = 0; y < imageHeight; y++) {
            // Progress
            int percent = (y * 100) / imageHeight;
            if (percent != lastPercent) {
                std::cout << "\r[CpuRaytracer] " << percent << "%" << std::flush;
                lastPercent = percent;
            }

            for (int x = 0; x < imageWidth; x++) {
                // NDC [-1, 1]
                float u = (2.f * (x + 0.5f) / imageWidth  - 1.f) * halfW;
                float v = (1.f - 2.f * (y + 0.5f) / imageHeight) * halfH;

                Ray ray{};
                ray.origin    = camPos;
                ray.direction = glm::normalize(camFwd + u * camRight + v * camUp);

                HitInfo hit = traceRay(ray);
                glm::vec3 color{ 0.f };
                if (hit.hit) {
                    color = shade(hit, ray, 0);
                }
                float exposure = 10.0f; // Aumente este valor (ex: 2.0, 5.0) para clarear tudo
                color *= exposure;
                color = toneMap(color);

                int idx = (y * imageWidth + x) * 3;
                pixels[idx + 0] = static_cast<uint8_t>(std::clamp(color.r, 0.f, 1.f) * 255.f);
                pixels[idx + 1] = static_cast<uint8_t>(std::clamp(color.g, 0.f, 1.f) * 255.f);
                pixels[idx + 2] = static_cast<uint8_t>(std::clamp(color.b, 0.f, 1.f) * 255.f);
            }
        }

        std::cout << "\r[CpuRaytracer] 100%\n";

        int result = stbi_write_png(outPath.c_str(), imageWidth, imageHeight, 3, pixels.data(), imageWidth * 3);
        if (!result) {
            std::cerr << "[CpuRaytracer] Erro ao salvar " << outPath << "\n";
            return false;
        }

        std::cout << "[CpuRaytracer] Salvo em: " << outPath << "\n";
        return true;
    }

    // =========================================================================
    // intersectTriangle — Moller-Trumbore
    // =========================================================================
    bool CpuRaytracer::intersectTriangle(const Ray& ray, const Triangle& tri,
                                          float& t, float& u, float& v) const {
        const float EPSILON = 1e-7f;
        glm::vec3 edge1 = tri.v1 - tri.v0;
        glm::vec3 edge2 = tri.v2 - tri.v0;
        glm::vec3 h = glm::cross(ray.direction, edge2);
        float a = glm::dot(edge1, h);

        if (std::abs(a) < EPSILON) return false; // ray paralelo ao triangulo

        float f = 1.f / a;
        glm::vec3 s = ray.origin - tri.v0;
        u = f * glm::dot(s, h);
        if (u < 0.f || u > 1.f) return false;

        glm::vec3 q = glm::cross(s, edge1);
        v = f * glm::dot(ray.direction, q);
        if (v < 0.f || u + v > 1.f) return false;

        t = f * glm::dot(edge2, q);
        return t > EPSILON;
    }

    // =========================================================================
    // traceRay — testa contra todos os triangulos, retorna o mais proximo
    // =========================================================================
    CpuRaytracer::HitInfo CpuRaytracer::traceRay(const Ray& ray) const {
        HitInfo closest{};
        closest.hit = false;
        closest.t = 1e9f;

        for (const auto& tri : triangles) {
            float t, u, v;
            if (intersectTriangle(ray, tri, t, u, v) && t < closest.t) {
                closest.hit = true;
                closest.t = t;
                closest.position = ray.origin + t * ray.direction;

                // Interpolacao baricentrica da normal
                float w = 1.f - u - v;
                closest.normal = glm::normalize(w * tri.n0 + u * tri.n1 + v * tri.n2);
                closest.color = (w * tri.c0 + u * tri.c1 + v * tri.c2);
            }
        }

        return closest;
    }

    // =========================================================================
    // shade — Blinn-Phong + sombras + reflexos recursivos
    // =========================================================================
    glm::vec3 CpuRaytracer::shade(const HitInfo& hit, const Ray& incomingRay, int depth) const {
        glm::vec3 N = hit.normal;
        glm::vec3 V = glm::normalize(-incomingRay.direction);

        // Luz ambiente
        glm::vec3 result = glm::vec3(ambientLight) * ambientLight.w * hit.color;        
        // Contribuicao de cada point light
        for (const auto& light : lights) {
            glm::vec3 toLight = light.position - hit.position;
            float dist = glm::length(toLight);
            glm::vec3 L = toLight / dist;

            // Shadow ray
            Ray shadowRay{};
            shadowRay.origin    = hit.position + N * RAY_BIAS;
            shadowRay.direction = L;

            bool shadowed = false;
            for (const auto& tri : triangles) {
                float t, u, v;
                if (intersectTriangle(shadowRay, tri, t, u, v) && t < dist) {
                    shadowed = true;
                    break;
                }
            }
            if (shadowed) continue;

            // Atenuacao
            float radius = std::max(light.radius, 0.001f);
            float attenuation = 1.f / (1.f + (dist * dist) / (radius * radius));

            // Diffuse (Lambert)
            float NdotL = std::max(glm::dot(N, L), 0.f);
            glm::vec3 diffuse = light.color * light.intensity * NdotL * attenuation;

            // Specular (Blinn-Phong)
            glm::vec3 H = glm::normalize(L + V);
            float NdotH = std::max(glm::dot(N, H), 0.f);
            glm::vec3 specular = light.color * light.intensity * std::pow(NdotH, 32.f) * attenuation;

            result += (diffuse * hit.color) + specular;
        }

        // Reflexo recursivo
        if (depth < maxBounces && reflectivity > 0.f) {
            glm::vec3 reflectDir = glm::reflect(incomingRay.direction, N);
            Ray reflectRay{};
            reflectRay.origin    = hit.position + N * RAY_BIAS;
            reflectRay.direction = glm::normalize(reflectDir);

            HitInfo reflectHit = traceRay(reflectRay);
            if (reflectHit.hit) {
                glm::vec3 reflectColor = shade(reflectHit, reflectRay, depth + 1);
                result = glm::mix(result, reflectColor, reflectivity);
                result = glm::mix(result, reflectColor, reflectivity);
            }
        }

        return result;
    }

    // =========================================================================
    // toneMap — Reinhard simples
    // =========================================================================
    glm::vec3 CpuRaytracer::toneMap(const glm::vec3& color) const {
        return color / (color + glm::vec3(1.f));
    }

} // namespace app
