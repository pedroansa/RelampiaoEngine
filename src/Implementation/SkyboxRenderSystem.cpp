#include "SkyboxRenderSystem.h"
#include "EngineSwapChain.h"
#include <stdexcept>

namespace app {

    SkyboxRenderSystem::SkyboxRenderSystem(EngineDevice& device, VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout,
        std::shared_ptr<CubemapTexture> cubemap)
        : engineDevice{ device }, cubemap{ cubemap } {
        createDescriptorSetLayout();
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
        createDescriptorSets();
    }

    SkyboxRenderSystem::~SkyboxRenderSystem() {
        vkDestroyPipelineLayout(engineDevice.device(), pipelineLayout, nullptr);
    }

    void SkyboxRenderSystem::createDescriptorSetLayout() {
        // binding 0 = samplerCube for the skybox texture
        skyboxSetLayout = AppDescriptorSetLayout::Builder(engineDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();
    }

    void SkyboxRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
        // set 0 = global UBO (projection + view), set 1 = skybox cubemap
        std::vector<VkDescriptorSetLayout> layouts{
            globalSetLayout,
            skyboxSetLayout->getDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.setLayoutCount = static_cast<uint32_t>(layouts.size());
        info.pSetLayouts = layouts.data();

        if (vkCreatePipelineLayout(engineDevice.device(), &info, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create skybox pipeline layout");
    }

    void SkyboxRenderSystem::createPipeline(VkRenderPass renderPass) {
        PipelineConfigInfo config{};
        AppPipeline::defaultPipelineConfigInfo(config);

        // No vertex input — positions are hardcoded in the vertex shader
        auto vertexInput = AppPipeline::getSolidVertexInputConfig();
        vertexInput.attributeDescriptions.clear();
        vertexInput.bindingDescriptions.clear();

        // Depth test ON but depth write OFF — skybox is always behind everything
        config.depthStencilInfo.depthWriteEnable = VK_FALSE;

        // Draw inside of the cube — front face is clockwise
        config.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
        config.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        config.renderPass = renderPass;
        config.pipelineLayout = pipelineLayout;

        pipeline = std::make_unique<AppPipeline>(
            engineDevice,
            "shaders/SKYBOX/shader.vert.spv",
            "shaders/SKYBOX/shader.frag.spv",
            config, vertexInput);
    }

    void SkyboxRenderSystem::createDescriptorSets() {
        // One descriptor set per frame in flight
        skyboxPool = AppDescriptorPool::Builder(engineDevice)
            .setMaxSets(EngineSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, EngineSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

        auto imageInfo = cubemap->getDescriptorInfo();
        descriptorSets.resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < EngineSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
            AppDescriptorWriter(*skyboxSetLayout, *skyboxPool)
                .writeImage(0, &imageInfo)
                .build(descriptorSets[i]);
        }
    }

    void SkyboxRenderSystem::render(FrameInfo& frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);

        // Bind set 0 = global UBO
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

        // Bind set 1 = cubemap
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 1, 1, &descriptorSets[frameInfo.frameIndex], 0, nullptr);

        // 36 vertices = 6 faces * 2 triangles * 3 vertices, all hardcoded in the shader
        vkCmdDraw(frameInfo.commandBuffer, 36, 1, 0, 0);
    }

} // namespace app