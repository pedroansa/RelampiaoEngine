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
            //sphere.pointLight->radius = sphere.radius;
        }

        return sphere;
    }

    void Sphere::setRadius(float newRadius) {
        this->radius = newRadius;
        this->transform.scale = glm::vec3(newRadius);
    }

} // namespace app