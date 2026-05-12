#include "EngineImgui.h"
#include <stdexcept>
#include <array>

namespace app {
    EngineImgui::EngineImgui(AppWindow& window, EngineDevice& device, VkRenderPass renderPass, uint32_t imageCount)
        : engineDevice{ device } {

        std::array<VkDescriptorPoolSize, 11> pool_sizes = { {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        } };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();

        if (vkCreateDescriptorPool(device.device(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS) {
            throw std::runtime_error("Falha ao criar pool do ImGui");
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(window.getGLFWwindow(), true);

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = device.getInstance();
        init_info.PhysicalDevice = device.getPhysicalDevice();
        init_info.Device = device.device();
        init_info.QueueFamily = device.findPhysicalQueueFamilies().graphicsFamily;
        init_info.Queue = device.graphicsQueue();
        init_info.DescriptorPool = imguiPool;
        init_info.MinImageCount = imageCount;
        init_info.ImageCount = imageCount;
        init_info.PipelineInfoMain.RenderPass = renderPass;
        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        ImGui_ImplVulkan_Init(&init_info);

    }

    EngineImgui::~EngineImgui() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(engineDevice.device(), imguiPool, nullptr);
    }

    void EngineImgui::newFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void EngineImgui::runDefaultUi(InitialRenderSystem& renderSystem, GameObject::Map& gameObjects) {
        ImGui::Begin("Relampiao Engine");

        // Performance Overlay
        ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)",
            1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);


        ImGui::Separator();

        if (ImGui::BeginTabBar("EngineMainTabs", ImGuiTabBarFlags_None)) {
            // --- TAB 1: RENDER SETTINGS ---
            if (ImGui::BeginTabItem("Renderer")) {
                ImGui::Text("Rasterization Mode");

                if (ImGui::Button("Solid"))     renderSystem.changeMode(PipelineMode::SOLID);
                ImGui::SameLine();
                if (ImGui::Button("Wireframe")) renderSystem.changeMode(PipelineMode::WIREFRAME);
                ImGui::SameLine();
                if (ImGui::Button("Points"))    renderSystem.changeMode(PipelineMode::POINTS);

                ImGui::EndTabItem();
            }
            // --- TAB 2: SCENE HIERARCHY ---
            if (ImGui::BeginTabItem("Hierarchy")) {
                ImGui::Text("Game Objects");
                ImGui::BeginChild("ScrollingRegion", ImVec2(0, 200), true); 

                for (auto& kv : gameObjects) {
                    auto& obj = kv.second;
                    // Only list objects that have a model (meshes)
                    if (obj->model != nullptr) {
                        std::string label = "Object " + std::to_string(obj->getId());

                        if (ImGui::TreeNode(label.c_str())) {
                            ImGui::DragFloat3("Position", &obj->transform.translation.x, 0.1f);
                            ImGui::DragFloat3("Rotation", &obj->transform.rotation.x, 0.05f);
                            ImGui::DragFloat3("Scale", &obj->transform.scale.x, 0.01f);
                            ImGui::TreePop();
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // --- TAB 3: LIGHTING ---
            if (ImGui::BeginTabItem("Lights")) {
                ImGui::Text("Point Lights");

                for (auto& kv : gameObjects) {
                    auto& obj = kv.second;
                    // Check if the object is a light source
                    if (obj->pointLight != nullptr) {
                        std::string label = "Light Source " + std::to_string(obj->getId());

                        if (ImGui::TreeNode(label.c_str())) {
                            ImGui::ColorEdit3("Color", &obj->color.r);
                            ImGui::DragFloat("Intensity", &obj->pointLight->lightIntensity, 1.0f, 0.0f, 10000.0f);
                            ImGui::DragFloat("Radius", &obj->pointLight->radius, 0.1f, 0.0f, 500.0f);
                            ImGui::DragFloat3("Position", &obj->transform.translation.x, 0.1f);
                            ImGui::TreePop();
                        }
                    }
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Scenes")) {
                ImGui::Text("Switch Environment:");

                if (ImGui::Button("Load Default Scene")) {
                    sceneRequest = SceneRequest::Default;
                }

                ImGui::SameLine();

                if (ImGui::Button("Load Solar System")) {
                    sceneRequest = SceneRequest::SolarSystem;
                }

                if (sceneRequest != SceneRequest::None) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Loading scene...");
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
    void EngineImgui::render(VkCommandBuffer commandBuffer) {
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }
}