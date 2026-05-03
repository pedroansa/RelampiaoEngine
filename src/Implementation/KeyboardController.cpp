#include "KeyboardController.h"

namespace app {
    void KeyboardController::moveInPlaneXZ(
        GLFWwindow* window, float dt, GameObject& gameObject) {

        glm::vec3 rotate{ 0 };
        if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
        if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
        if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
        if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
            gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
        }

        // limit pitch values between about +/- 85ish degrees
        gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
        gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

        float yaw = gameObject.transform.rotation.y;
        const glm::vec3 forwardDir{ sin(yaw), 0.f, cos(yaw) };
        const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
        const glm::vec3 upDir{ 0.f, -1.f, 0.f };

        glm::vec3 moveDir{ 0.f };
        if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
        if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
        if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
        if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
        if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
        if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
            gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
        }
    }
    void KeyboardController::handleInput(GLFWwindow* window, InitialRenderSystem& renderSystem)
    {
       
        if (!(renderSystem.getMode() == PipelineMode::SOLID) && glfwGetKey(window, keys.solidShader) == GLFW_PRESS) {
            std::cout << "Switched to SOLID mode\n";
            renderSystem.changeMode(PipelineMode::SOLID);
        }
        if (!(renderSystem.getMode() == PipelineMode::WIREFRAME) && glfwGetKey(window, keys.wireframeShader) == GLFW_PRESS) {
            std::cout << "Switched to WIREFRAME mode\n";
            renderSystem.changeMode(PipelineMode::WIREFRAME);
        }
        if (!(renderSystem.getMode() == PipelineMode::POINTS) && glfwGetKey(window, keys.pointShader) == GLFW_PRESS) {
            std::cout << "Switched to POINTS mode\n";
            renderSystem.changeMode(PipelineMode::POINTS);
        }
     
    }
}