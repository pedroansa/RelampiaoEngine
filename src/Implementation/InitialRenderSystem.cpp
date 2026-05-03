#include "InitialRenderSystem.h"
#include <iostream>


namespace app {

	InitialRenderSystem::InitialRenderSystem(EngineDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : engineDevice{device}
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}
	InitialRenderSystem::~InitialRenderSystem()
	{
		vkDestroyPipelineLayout(engineDevice.device(), pipelineLayout, nullptr);
	}
	
	void InitialRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };


		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(engineDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void InitialRenderSystem::createPipeline(VkRenderPass& renderPass)
	{
		assert(pipelineLayout != nullptr && "Cannot create pipeline before layour");

		// Para pipeline s�lido (original)
		PipelineConfigInfo solidConfig{};
		PipelineConfigInfo wireframeConfig{};
		PipelineConfigInfo pointConfig{};

		AppPipeline::defaultPipelineConfigInfo(solidConfig);
		auto solidVertexInput = AppPipeline::getSolidVertexInputConfig();
		solidConfig.renderPass = renderPass;
		solidConfig.pipelineLayout = pipelineLayout;
		solidPipeline = std::make_unique<AppPipeline>(
			engineDevice,
			"shaders/SOLID/shader.vert.spv",
			"shaders/SOLID/shader.frag.spv",
			solidConfig, solidVertexInput);

		//Para wireframe
		AppPipeline::wireframePipelineConfigInfo(wireframeConfig);
		auto wireFrameVertexInput = AppPipeline::getWireframeVertexInputConfig();
		wireframeConfig.renderPass = renderPass;
		wireframeConfig.pipelineLayout = pipelineLayout;
		wireframePipeline = std::make_unique<AppPipeline>(
			engineDevice,
			"shaders/WIREFRAME/shader.vert.spv",
			"shaders/WIREFRAME/shader.frag.spv",
			wireframeConfig, wireFrameVertexInput);

		//// Para pontos
		AppPipeline::pointsPipelineConfigInfo(pointConfig);
		auto pointsVertexInput = AppPipeline::getPointsVertexInputConfig();
		pointConfig.renderPass = renderPass;
		pointConfig.pipelineLayout = pipelineLayout;
		pointsPipeline = std::make_unique<AppPipeline>(
			engineDevice,
			"shaders/POINTS/shader.vert.spv",
			"shaders/POINTS/shader.frag.spv",
			pointConfig, pointsVertexInput);
	}
	void InitialRenderSystem::renderGameObjects(FrameInfo& frameInfo)
	{

		switch (pipelineMode) {
		case PipelineMode::SOLID:
			solidPipeline->bind(frameInfo.commandBuffer);
			break;
		case PipelineMode::WIREFRAME:
			wireframePipeline->bind(frameInfo.commandBuffer);
			break;
		case PipelineMode::POINTS:
			pointsPipeline->bind(frameInfo.commandBuffer);
			break;
		}

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr);

		// Render
		//auto projectionView = frameInfo.camera.getProjection() * frameInfo.camera.getView();

		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.model == nullptr) continue;

			SimplePushConstantData push{};
			// rotate
			//obj.transform.rotation.y += 0.1f * frameInfo.frameTime;
			push.modelMatrix = obj.transform.mat4();
			push.normalMatrix = obj.transform.normalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);

			obj.model->bind(frameInfo.commandBuffer);
			obj.model->draw(frameInfo.commandBuffer);

		}
	}

}