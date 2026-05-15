#pragma once
#include "AppPipeline.h"
#include "EngineDevice.h"

#include "FrameInfo.h"
#pragma once
#include "AppPipeline.h"
#include "EngineDevice.h"
#include "EngineSwapChain.h"
#include "FrameInfo.h"
#include "GameObject.h"
#include "CubemapTexture.h"
#include "AppDescriptor.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace app {

    class SkyboxRenderSystem {
    public:
        SkyboxRenderSystem(EngineDevice& device, VkRenderPass renderPass,
            VkDescriptorSetLayout globalSetLayout,
            std::shared_ptr<CubemapTexture> cubemap);
        ~SkyboxRenderSystem();

        SkyboxRenderSystem(const SkyboxRenderSystem&) = delete;
        SkyboxRenderSystem& operator=(const SkyboxRenderSystem&) = delete;

        void render(FrameInfo& frameInfo);

    private:
        void createDescriptorSetLayout();
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);
        void createDescriptorSets();

        EngineDevice& engineDevice;
        std::shared_ptr<CubemapTexture> cubemap;

        // Skybox has its own descriptor set layout (binding 0 = samplerCube)
        std::unique_ptr<AppDescriptorSetLayout> skyboxSetLayout;
        std::unique_ptr<AppDescriptorPool> skyboxPool;
        std::vector<VkDescriptorSet> descriptorSets;

        std::unique_ptr<AppPipeline> pipeline;
        VkPipelineLayout pipelineLayout;
    };

} // namespace app