#pragma once
#include "GameObject.h"
#include "Camera.h"
#include "FrameInfo.h"
#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace app {

    // =========================================================================
    // CpuRaytracer
    // =========================================================================
    // Raytracer de CPU que usa os dados de cena ja existentes no seu renderer.
    // Ao chamar render(), ele percorre cada pixel da imagem, lanca um ray da
    // camera, testa intersecao com todos os triangulos dos GameObjects e calcula
    // iluminacao usando as PointLights do UBO. O resultado e salvo em PNG.
    //
    // FEATURES:
    //   - Ray-triangle intersection (Moller-Trumbore)
    //   - Diffuse + Specular (Blinn-Phong), igual ao seu shader SOLID
    //   - Sombras hard (shadow rays para cada luz)
    //   - Reflexos recursivos (controlado por maxBounces)
    //   - Salvamento em PNG via stb_image_write (header-only, incluso)
    //
    // USO BASICO:
    //
    //   // 1. Construcao — passa largura, altura e profundidade de reflexo
    //   CpuRaytracer raytracer{800, 600, /*maxBounces=*/3};
    //
    //   // 2. Carrega a cena a partir dos seus GameObjects e do UBO atual
    //   //    Chame isso uma vez antes de render(), ou toda vez que a cena mudar
    //   raytracer.loadScene(gameObjects, ubo);
    //
    //   // 3. Renderiza e salva
    //   //    camera  — sua Camera atual (getView() / getInverseView())
    //   //    fovY    — mesmo valor usado em setPerspectiveProjection (ex: glm::radians(50.f))
    //   //    aspect  — largura / altura da janela
    //   //    outPath — caminho do arquivo PNG de saida
    //   raytracer.render(camera, glm::radians(50.f), aspect, "raytrace_output.png");
    //
    // EXEMPLO DE INTEGRACAO NO SEU VulkanApp::run():
    //
    //   CpuRaytracer raytracer{WIDTH, HEIGHT, 3};
    //
    //   // dentro do loop, apos pointLightSystem.update():
    //   if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !mWasPressed) {
    //       mWasPressed = true;
    //       raytracer.loadScene(gameObjects, ubo);
    //       raytracer.render(camera, glm::radians(50.f), aspect, "raytrace.png");
    //       std::cout << "Raytracing salvo em raytrace.png" << std::endl;
    //   }
    //   if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) mWasPressed = false;
    //
    // =========================================================================

    class CpuRaytracer {
    public:
        // width, height  — resolucao da imagem de saida (recomendado: igual a janela)
        // maxBounces     — profundidade maxima de reflexos (0 = sem reflexo, 3 = bom custo/qualidade)
        CpuRaytracer(int width, int height, int maxBounces = 6);

        // Carrega/atualiza a cena a partir dos GameObjects e do UBO atual.
        // Deve ser chamado antes de render() e toda vez que a cena mudar.
        // gameObjects — seu GameObject::Map
        // ubo         — GlobalUbo preenchido no frame atual (contem posicao/cor das luzes)
        void loadScene(const GameObject::Map& gameObjects, const GlobalUbo& ubo);

        // Executa o raytracing e salva o resultado em PNG.
        // camera  — Camera atual do seu renderer
        // fovY    — campo de visao vertical em radianos (ex: glm::radians(50.f))
        // aspect  — aspect ratio (largura / altura)
        // outPath — caminho do arquivo de saida (ex: "raytrace.png")
        // Retorna true se salvou com sucesso, false caso contrario.
        bool render(const Camera& camera, float fovY, float aspect, const std::string& outPath);

    private:
        // --- Estruturas internas ---

        struct Triangle {
            glm::vec3 v0, v1, v2;
            glm::vec3 n0, n1, n2;
            glm::vec3 c0, c1, c2; // Agora temos as 3 cores
        };

        struct SceneLight {
            glm::vec3 position;
            glm::vec3 color;
            float intensity;
            float radius;
        };

        struct Ray {
            glm::vec3 origin;
            glm::vec3 direction;        // deve ser normalizado
        };

        struct HitInfo {
            bool hit = false;
            float t = 1e9f;             // distancia ao longo do ray
            glm::vec3 position;         // ponto de intersecao em world space
            glm::vec3 normal;           // normal interpolada no ponto de intersecao
            glm::vec3 color;            // cor interpolada no ponto de intersecao
        };

        // --- Metodos internos ---

        // Testa intersecao ray-triangulo (Moller-Trumbore).
        // Retorna true e preenche t, u, v se houver intersecao.
        bool intersectTriangle(const Ray& ray, const Triangle& tri,
                               float& t, float& u, float& v) const;

        // Testa o ray contra todos os triangulos da cena.
        // Retorna o HitInfo do triangulo mais proximo.
        HitInfo traceRay(const Ray& ray) const;

        // Calcula a cor final de um hit, incluindo sombras e reflexos recursivos.
        // depth — nivel de recursao atual (para limitar reflexos)
        glm::vec3 shade(const HitInfo& hit, const Ray& incomingRay, int depth) const;

        // Converte cor HDR (pode passar de 1.0) para LDR com tone mapping simples.
        glm::vec3 toneMap(const glm::vec3& color) const;

        // --- Dados da cena ---
        std::vector<Triangle> triangles;    // todos os triangulos de todos os meshes
        std::vector<SceneLight> lights;     // todas as point lights
        glm::vec4 ambientLight;             // xyz = cor, w = intensidade

        // --- Configuracao ---
        int imageWidth;
        int imageHeight;
        int maxBounces;

        // Reflectividade dos materiais (0 = sem reflexo, 1 = espelho perfeito)
        // Valor global por enquanto — pode ser expandido por material no futuro
        float reflectivity = 0.3f;

        // Bias para evitar self-intersection em shadow rays e reflection rays
        static constexpr float RAY_BIAS = 1e-4f;
    };

} // namespace app
