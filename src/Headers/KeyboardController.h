#pragma once
#include "AppWindow.h"
#include "InitialRenderSystem.h"
#include "GameObject.h"

#include <iostream>

namespace app {
    class KeyboardController
    {
    public:
        struct KeyMappings {
            int moveLeft = GLFW_KEY_A;
            int moveRight = GLFW_KEY_D;
            int moveForward = GLFW_KEY_W;
            int moveBackward = GLFW_KEY_S;
            int moveUp = GLFW_KEY_E;
            int moveDown = GLFW_KEY_Q;
            int lookLeft = GLFW_KEY_LEFT;
            int lookRight = GLFW_KEY_RIGHT;
            int lookUp = GLFW_KEY_UP;
            int lookDown = GLFW_KEY_DOWN;

            // Pipeline mode controler
            int solidShader = GLFW_KEY_1;
            int wireframeShader = GLFW_KEY_2;
            int pointShader = GLFW_KEY_3;
        };

        KeyboardController() {}

        void moveInPlaneXZ(GLFWwindow* window, float dt, GameObject& gameObject);
        void handleInput(GLFWwindow* window, InitialRenderSystem& renderSystem);

        KeyMappings keys{};
        float moveSpeed{ 1.f };
        float lookSpeed{ 1.5f };
    };
}


