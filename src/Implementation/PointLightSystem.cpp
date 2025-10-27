#include "PointLightSystem.h"
#include <iostream>


namespace app {

	struct PointLightPushConstants {
		glm::vec4 position{};
		glm::vec4 color{};
		float radius;
	};

	PointLightSystem::PointLightSystem(EngineDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : engineDevice{ device }
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}
	PointLightSystem::~PointLightSystem()
	{
		vkDestroyPipelineLayout(engineDevice.device(), pipelineLayout, nullptr);
	}

	void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PointLightPushConstants);

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

	void PointLightSystem::createPipeline(VkRenderPass& renderPass)
	{
		assert(pipelineLayout != nullptr && "Cannot create pipeline before layour");

		// Para pipeline sólido (original)
		PipelineConfigInfo solidConfig{};
		PipelineConfigInfo wireframeConfig{};
		PipelineConfigInfo pointConfig{};

		AppPipeline::defaultPipelineConfigInfo(solidConfig);
		auto solidVertexInput = AppPipeline::getSolidVertexInputConfig();
		solidVertexInput.attributeDescriptions.clear();
		solidVertexInput.bindingDescriptions.clear();
		solidConfig.renderPass = renderPass;
		solidConfig.pipelineLayout = pipelineLayout;
		solidPipeline = std::make_unique<AppPipeline>(
			engineDevice,
			"shaders/POINT_LIGHT/shader.vert.spv",
			"shaders/POINT_LIGHT/shader.frag.spv",
			solidConfig, solidVertexInput);

		////Para wireframe
		//AppPipeline::wireframePipelineConfigInfo(wireframeConfig);
		//auto wireFrameVertexInput = AppPipeline::getWireframeVertexInputConfig();
		//wireframeConfig.renderPass = renderPass;
		//wireframeConfig.pipelineLayout = pipelineLayout;
		//wireframePipeline = std::make_unique<AppPipeline>(
		//	engineDevice,
		//	"shaders/WIREFRAME/vert.spv",
		//	"shaders/WIREFRAME/frag.spv",
		//	wireframeConfig, wireFrameVertexInput);

		////// Para pontos
		//AppPipeline::pointsPipelineConfigInfo(pointConfig);
		//auto pointsVertexInput = AppPipeline::getPointsVertexInputConfig();
		//pointConfig.renderPass = renderPass;
		//pointConfig.pipelineLayout = pipelineLayout;
		//pointsPipeline = std::make_unique<AppPipeline>(
		//	engineDevice,
		//	"shaders/POINTS/vert.spv",
		//	"shaders/POINTS/frag.spv",
		//	pointConfig, pointsVertexInput);
	}
	void PointLightSystem::render(FrameInfo& frameInfo)
	{

		solidPipeline->bind(frameInfo.commandBuffer);

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr);

		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.pointLight == nullptr) continue;

			PointLightPushConstants push{};
			push.position = glm::vec4(obj.transform.translation, 1.f);
			push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
			push.radius = obj.transform.scale.x;

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PointLightPushConstants),
				&push);
			vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
		}
		
	}

	void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo)
	{
		auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * frameInfo.frameTime, { 0.f, -1.f, 0.f });
		int lightIndex = 0;
		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.pointLight == nullptr) continue;
			
			assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");
			obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));

			ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.f);
			ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
			//ubo.pointLights[lightIndex].radius = obj.pointLight->radius;

			lightIndex += 1;
		}
		ubo.numLights = lightIndex;
	}

}