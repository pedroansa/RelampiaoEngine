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
    class EngineImgui {
    public:
        EngineImgui(AppWindow& window, EngineDevice& device, VkRenderPass renderPass, uint32_t imageCount);
        ~EngineImgui();

        // Bloquear cópias
        EngineImgui(const EngineImgui&) = delete;
        EngineImgui& operator=(const EngineImgui&) = delete;

        void newFrame();
        void runDefaultUi(InitialRenderSystem& renderSystem, GameObject::Map& gameObjects);
        void render(VkCommandBuffer commandBuffer);

    private:
        EngineDevice& engineDevice;
        VkDescriptorPool imguiPool;
    };
}