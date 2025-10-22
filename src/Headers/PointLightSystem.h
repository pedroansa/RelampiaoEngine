#pragma once
#include "AppPipeline.h"
#include "EngineDevice.h"
#include "GameObject.h"
#include "FrameInfo.h"

#include <memory>
#include <vector>
#include <array>

namespace app {

	class PointLightSystem
	{
		struct SimplePushConstantData {
			glm::mat4 modelMatrix{ 1.f };
			glm::mat4 normalMatrix{ 1.f };
		};


	public:

		PointLightSystem(EngineDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~PointLightSystem();

		PointLightSystem(const PointLightSystem&) = delete;
		PointLightSystem& operator=(const PointLightSystem&) = delete;

		void render(FrameInfo& frameInfo);
		void update(FrameInfo& frameInfo, GlobalUbo& ubo);


	private:

		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass& renderPass);

		EngineDevice& engineDevice;

		std::unique_ptr<AppPipeline> appPipeline;
		VkPipelineLayout pipelineLayout;

		std::unique_ptr<AppPipeline> solidPipeline;
		std::unique_ptr<AppPipeline> wireframePipeline;
		std::unique_ptr<AppPipeline> pointsPipeline;
	};
}


