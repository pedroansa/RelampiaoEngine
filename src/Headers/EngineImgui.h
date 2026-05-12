#pragma once

#include "EngineDevice.h"
#include "AppWindow.h"

// ImGui Headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <GameObject.h>
#include <InitialRenderSystem.h>

namespace app {
    enum class SceneRequest { None, Default, SolarSystem };

    class EngineImgui {
    public:
        EngineImgui(AppWindow& window, EngineDevice& device, VkRenderPass renderPass, uint32_t imageCount);
        ~EngineImgui();

        EngineImgui(const EngineImgui&) = delete;
        EngineImgui& operator=(const EngineImgui&) = delete;

        SceneRequest getSceneRequest() const { return sceneRequest; }
        void resetSceneRequest() { sceneRequest = SceneRequest::None; }

        void newFrame();
        void runDefaultUi(InitialRenderSystem& renderSystem, GameObject::Map& gameObjects);
        void render(VkCommandBuffer commandBuffer);

    private:
        EngineDevice& engineDevice;
        VkDescriptorPool imguiPool;
        SceneRequest sceneRequest = SceneRequest::None;
    };
}