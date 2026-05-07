#include "Sphere.h"
#include "Model.h"

namespace app {

    Sphere Sphere::createSphere(
        EngineDevice& device,
        float radius,
        bool hasLight,
        uint32_t sectorCount,
        uint32_t stackCount,
        glm::vec3 position,
        glm::vec3 color){

        auto sphereModelUnique = Model::createSphereModel(device, radius, sectorCount, stackCount);
        std::shared_ptr<Model> sphereModel = std::move(sphereModelUnique);

        // Create the sphere game object
        auto gameObj = GameObject::createGameObject();
        Sphere sphere(gameObj.getId());

        // Set sphere properties
        sphere.model = sphereModel;  // Now this should work
        sphere.color = color;
        sphere.radius = radius;
        sphere.transform.translation = position;
        sphere.transform.scale = glm::vec3(radius);

        if (hasLight) {
            sphere.pointLight = std::make_unique<PointLightComponent>();
            sphere.pointLight->lightIntensity = 1.f;
            sphere.pointLight->radius = sphere.radius;
        }

        return sphere;
    }

    void Sphere::update(float deltaTime) {
        // Atualiza órbita
        if (orbitSpeed > 0.0f && orbitRadius > 0.0f) {
            orbitAngle += orbitSpeed * deltaTime;
            if (orbitAngle > glm::two_pi<float>()) {
                orbitAngle -= glm::two_pi<float>();
            }

            transform.translation.x = orbitCenter.x + orbitRadius * cos(orbitAngle);
            transform.translation.z = orbitCenter.z + orbitRadius * sin(orbitAngle);
        }

        // Atualiza rotação própria
        if (selfRotationSpeed > 0.0f) {
            transform.rotation.y += selfRotationSpeed * deltaTime;
        }
    }

    void Sphere::setRadius(float newRadius) {
        this->radius = newRadius;
        this->transform.scale = glm::vec3(newRadius);
    }

} // namespace app