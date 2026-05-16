#pragma once
#include "GameObject.h"
#include "Camera.h"
#include "Sphere.h"
#include <glm/glm.hpp>
#include <functional>
#include <string>

namespace app {

    struct CameraConfig {
        glm::vec3 position{ 0.f, 0.f, 2.f };
        glm::vec3 rotation{ 0.f, 0.f, 0.f };
        float fovY = glm::radians(50.f);
        float nearPlane = 0.1f;
        float farPlane = 5000000.f;
    };

    class Scene {
    public:
        std::string name;
        CameraConfig cameraConfig;

        // Called once to populate gameObjects
        std::function<void(GameObject::Map&, EngineDevice&)> load;

        // Called every frame — optional, for dynamic scenes
        // dt = delta time, gameObjects can be modified
        std::function<void(GameObject::Map&, float dt)> update = nullptr;

        static void loadGameObjects(GameObject::Map& gameObjects, EngineDevice& engineDevice);
        static void loadSolarSystem(GameObject::Map& gameObjects, EngineDevice& engineDevice);
    };

} // namespace app