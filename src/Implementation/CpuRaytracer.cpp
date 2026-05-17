#include "CpuRaytracer.h"
#include "Camera.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>

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
        bvhTris.clear();
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

                    BVHTri tri{};
                    tri.v0 = glm::vec3(modelMatrix * glm::vec4(a.position, 1.f));
                    tri.v1 = glm::vec3(modelMatrix * glm::vec4(b.position, 1.f));
                    tri.v2 = glm::vec3(modelMatrix * glm::vec4(c.position, 1.f));
                    tri.n0 = glm::normalize(normalMatrix * a.normal);
                    tri.n1 = glm::normalize(normalMatrix * b.normal);
                    tri.n2 = glm::normalize(normalMatrix * c.normal);
                    tri.c0 = a.color;
                    tri.c1 = b.color;
                    tri.c2 = c.color;
                    tri.uv0 = a.uv;
                    tri.uv1 = b.uv;
                    tri.uv2 = c.uv;
                    tri.uvScale = obj->uvScale;
                    if (obj->texture) {
                        tri.texPixels = obj->texture->cpuPixels.data();
                        tri.texWidth = obj->texture->texWidth;
                        tri.texHeight = obj->texture->texHeight;
                    }
                    bvhTris.push_back(tri);
                }
            } else {
                // Triangulos nao indexados
                for (size_t i = 0; i + 2 < verts.size(); i += 3) {
                    BVHTri tri{};
                    tri.v0 = glm::vec3(modelMatrix * glm::vec4(verts[i].position, 1.f));
                    tri.v1 = glm::vec3(modelMatrix * glm::vec4(verts[i+1].position, 1.f));
                    tri.v2 = glm::vec3(modelMatrix * glm::vec4(verts[i+2].position, 1.f));
                    tri.n0 = glm::normalize(normalMatrix * verts[i].normal);
                    tri.n1 = glm::normalize(normalMatrix * verts[i+1].normal);
                    tri.n2 = glm::normalize(normalMatrix * verts[i+2].normal);
                    tri.c0 = verts[i].color;   // <--- Use o índice i
                    tri.c1 = verts[i + 1].color; // <--- Use o índice i+1
                    tri.c2 = verts[i + 2].color; // <--- Use o índice i+2
                    bvhTris.push_back(tri);
                }
            }
        }
        bvh.build(bvhTris);
        std::cout << "[CpuRaytracer] Cena carregada: "
                  << bvhTris.size() << " triangulos, "
                  << lights.size() << " luzes.\n";
    }

    // =========================================================================
    // render
    // =========================================================================
    bool CpuRaytracer::render(const Camera& camera, float fovY, float aspect, const std::string& outPath) {
        std::cout << "[CpuRaytracer] Iniciando raytracing " << imageWidth << "x" << imageHeight << "...\n";

        std::vector<uint8_t> pixels(imageWidth * imageHeight * 3);

        glm::mat4 invView = camera.getInverseView();
        glm::vec3 camPos = glm::vec3(invView[3]);
        glm::vec3 camRight = glm::vec3(invView[0]);
        glm::vec3 camUp = -glm::vec3(invView[1]);
        glm::vec3 camFwd = glm::vec3(invView[2]);

        float halfH = std::tan(fovY * 0.5f);
        float halfW = halfH * aspect;

        // Divide as linhas entre as threads
        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4; // fallback
        std::cout << "[CpuRaytracer] Usando " << numThreads << " threads\n";

        std::vector<std::thread> threads;
        std::atomic<int> linesCompleted{ 0 };

        auto renderLines = [&](int startY, int endY) {
            std::mt19937 rng(std::hash<std::thread::id>{}(std::this_thread::get_id()));
            std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

            const int SAMPLES = 4;
            for (int y = startY; y < endY; y++) {
                for (int x = 0; x < imageWidth; x++) {
                    glm::vec3 colorAccum{ 0.f };
                    for (int s = 0; s < SAMPLES; s++) {
                        // Jitter offset within the pixel
                        float jitterX = dist(rng);
                        float jitterY = dist(rng);

                        float u = (2.f * (x + 0.5f + jitterX) / imageWidth - 1.f) * halfW;
                        float v = (1.f - 2.f * (y + 0.5f + jitterY) / imageHeight) * halfH;

                        Ray ray{};
                        ray.origin = camPos;
                        ray.direction = glm::normalize(camFwd + u * camRight + v * camUp);

                        HitInfo hit = traceRay(ray);
                        glm::vec3 color{ 0.f };
                        if (hit.hit) color = shade(hit, ray, 0);
                        colorAccum += toneMap(color);
                    }

                    colorAccum /= static_cast<float>(SAMPLES);

                    int idx = (y * imageWidth + x) * 3;
                    pixels[idx + 0] = static_cast<uint8_t>(std::clamp(colorAccum.r, 0.f, 1.f) * 255.f);
                    pixels[idx + 1] = static_cast<uint8_t>(std::clamp(colorAccum.g, 0.f, 1.f) * 255.f);
                    pixels[idx + 2] = static_cast<uint8_t>(std::clamp(colorAccum.b, 0.f, 1.f) * 255.f);
                }
                linesCompleted++;
            }
            };

        // Distribui linhas entre threads
        int linesPerThread = imageHeight / numThreads;
        for (unsigned int t = 0; t < numThreads; t++) {
            int startY = t * linesPerThread;
            int endY = (t == numThreads - 1) ? imageHeight : startY + linesPerThread;
            threads.emplace_back(renderLines, startY, endY);
        }

        // Progress enquanto threads rodam
        while (linesCompleted < imageHeight) {
            int percent = (linesCompleted * 100) / imageHeight;
            std::cout << "\r[CpuRaytracer] " << percent << "%" << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Espera todas terminarem
        for (auto& t : threads) t.join();
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
        HitInfo result;
        auto hit = bvh.intersect(ray.origin, ray.direction);
        if (!hit.hit) return result;

        const BVHTri& tri = bvh.getTris()[hit.triIndex];
        float w = 1.f - hit.u - hit.v;

        result.hit = true;
        result.t = hit.t;
        result.position = ray.origin + hit.t * ray.direction;
        result.normal = glm::normalize(w * tri.n0 + hit.u * tri.n1 + hit.v * tri.n2);
        result.color = w * tri.c0 + hit.u * tri.c1 + hit.v * tri.c2;
        result.uv = w * tri.uv0 + hit.u * tri.uv1 + hit.v * tri.uv2;
        result.tri = &tri;
        return result;
    }

    // =========================================================================
    // shade — Blinn-Phong + sombras + reflexos recursivos
    // =========================================================================
    glm::vec3 CpuRaytracer::shade(const HitInfo& hit, const Ray& incomingRay, int depth) const {
        glm::vec3 N = hit.normal;
        glm::vec3 V = glm::normalize(-incomingRay.direction);

        // Luz ambiente
        // ==========================================
        // TEXTURE SAMPLING (CORRIGIDO COM REPETIÇÃO E FILTRO BILINEAR)
        // ==========================================
        glm::vec3 surfaceColor = hit.color;
        if (hit.tri && hit.tri->texPixels && hit.tri->texWidth > 0 && hit.tri->texHeight > 0) {

            float texScale = hit.tri->uvScale;

            // Realiza o comportamento de repetição (Tiling / Wrap)
            float u_norm = (hit.uv.x * texScale) - std::floor(hit.uv.x * texScale);
            float v_norm = (hit.uv.y * texScale) - std::floor(hit.uv.y * texScale);

            // Inversão do eixo Y para sincronizar com o padrão do Vulkan
            v_norm = 1.f - v_norm;

            // Converte a fração contínua para coordenadas de pixel em float
            float texX = u_norm * (hit.tri->texWidth - 1);
            float texY = v_norm * (hit.tri->texHeight - 1);

            // Coordenadas dos pixels vizinhos para Interpolação Bilinear (Remove os blocos gigantes)
            int x0 = static_cast<int>(std::floor(texX));
            int y0 = static_cast<int>(std::floor(texY));
            int x1 = std::min(x0 + 1, hit.tri->texWidth - 1);
            int y1 = std::min(y0 + 1, hit.tri->texHeight - 1);

            float fX = texX - x0;
            float fY = texY - y0;

            // Lambda auxiliar para extrair a cor do pixel do array linear RGBA
            auto getPixelColor = [&](int px, int py) {
                int idx = (py * hit.tri->texWidth + px) * 4;
                return glm::vec3(
                    hit.tri->texPixels[idx + 0] / 255.f,
                    hit.tri->texPixels[idx + 1] / 255.f,
                    hit.tri->texPixels[idx + 2] / 255.f
                );
                };

            // Coleta as quatro amostras vizinhas
            glm::vec3 p00 = getPixelColor(x0, y0);
            glm::vec3 p10 = getPixelColor(x1, y0);
            glm::vec3 p01 = getPixelColor(x0, y1);
            glm::vec3 p11 = getPixelColor(x1, y1);

            // Interpolação bilinear final
            surfaceColor = glm::mix(glm::mix(p00, p10, fX), glm::mix(p01, p11, fX), fY);
        }
        // ==========================================
        glm::vec3 ambient = glm::max(glm::vec3(ambientLight) * ambientLight.w, glm::vec3(0.05f));
        glm::vec3 result = ambient * surfaceColor;
        // Contribuicao de cada point light
        for (const auto& light : lights) {
            glm::vec3 toLight = light.position - hit.position;
            float dist = glm::length(toLight);
            glm::vec3 L = toLight / dist;

            // Soft shadows — multiple shadow rays to random points on light area
            const int SHADOW_SAMPLES = 4;
            int shadowHits = 0;

            std::mt19937 shadowRng(std::hash<float>{}(hit.position.x + hit.position.y + hit.position.z));
            std::uniform_real_distribution<float> shadowJitter(-1.f, 1.f);

            for (int s = 0; s < SHADOW_SAMPLES; s++) {
                glm::vec3 lightOffset = glm::vec3(
                    shadowJitter(shadowRng),
                    shadowJitter(shadowRng),
                    shadowJitter(shadowRng)) * light.radius * 0.1f;

                glm::vec3 lightSamplePos = light.position + lightOffset;
                glm::vec3 toLightSample = lightSamplePos - hit.position;
                float distSample = glm::length(toLightSample);

                Ray shadowRay{};
                shadowRay.origin = hit.position + N * RAY_BIAS;
                shadowRay.direction = glm::normalize(toLightSample);

                auto shadowHit = bvh.intersect(shadowRay.origin, shadowRay.direction);
                if (shadowHit.hit && shadowHit.t < distSample) shadowHits++;
            }

            float shadowFactor = 1.f - (static_cast<float>(shadowHits) / SHADOW_SAMPLES);
            if (shadowFactor == 0.f) continue;

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

            result += ((diffuse * surfaceColor) + specular) * shadowFactor;
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
