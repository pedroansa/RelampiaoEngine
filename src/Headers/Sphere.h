#pragma once

#include "GameObject.h"
#include "EngineDevice.h"

namespace app {

    class Sphere : public GameObject {
    public:
        static Sphere createSphere(
            EngineDevice& device,
            float radius = 1.0f,
            bool hasLight = false,
            uint32_t sectorCount = 36,
            uint32_t stackCount = 18,
            glm::vec3 position = { 0.0f, 0.0f, 0.0f },
            glm::vec3 color = { 1.0f, 1.0f, 1.0f });

        float orbitRadius = 0.0f;
        float orbitSpeed = 0.0f;
        float orbitAngle = 0.0f;
        glm::vec3 orbitCenter = { 0.0f, 0.0f, 0.0f };
        float selfRotationSpeed = 0.0f;
        void update(float deltaTime) override;

        // Constructor for creating spheres with custom models
        float getRadius() const { return radius; }
        void setRadius(float newRadius);
        Sphere(id_t objId) : GameObject(objId) {} // Private constructor


    private:
        float radius;
    };

} // namespace app