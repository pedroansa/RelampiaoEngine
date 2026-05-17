#pragma once
#include "AppPipeline.h"
#include "EngineDevice.h"
#include "GameObject.h"
#include "FrameInfo.h"

#include <memory>
#include <vector>
#include <array>

namespace app {

	// Enum para modos de renderização
	enum class PipelineMode {
		SOLID,
		WIREFRAME,
		POINTS
	};

	class InitialRenderSystem
	{
		struct SimplePushConstantData {
			glm::mat4 modelMatrix{ 1.f };
			glm::mat4 normalMatrix{ 1.f };
			float uvScale = 1.f;
			float padding[3]{};
		};


	public: 

		InitialRenderSystem(EngineDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~InitialRenderSystem();

		InitialRenderSystem(const InitialRenderSystem&) = delete;
		InitialRenderSystem& operator=(const InitialRenderSystem&) = delete;

		void renderGameObjects(FrameInfo& frameInfo);
		void changeMode(PipelineMode mode) { pipelineMode = mode; };
		PipelineMode getMode() { return pipelineMode; };


	private:

		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass& renderPass);

		EngineDevice& engineDevice;

		std::unique_ptr<AppPipeline> appPipeline;
		VkPipelineLayout pipelineLayout;

		std::unique_ptr<AppPipeline> solidPipeline;
		std::unique_ptr<AppPipeline> wireframePipeline;
		std::unique_ptr<AppPipeline> pointsPipeline;

		PipelineMode pipelineMode = PipelineMode::SOLID;
	};
}


