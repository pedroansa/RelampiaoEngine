#pragma once
#include "AppWindow.h"
#include "EngineDevice.h"
#include "AppRenderer.h"
#include "GameObject.h"
#include "Camera.h"
#include "InitialRenderSystem.h"
#include "PointLightSystem.h"
#include "KeyboardController.h"
#include "AppBuffer.h"
#include "FrameInfo.h"
#include "AppDescriptor.h"

#include <chrono>
#include <memory>
#include <vector>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace app {
	class VulkanApp
	{

	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		VulkanApp();
		~VulkanApp();

		VulkanApp(const VulkanApp&) = delete;
		VulkanApp& operator=(const VulkanApp&) = delete;

		void run();

	private:
		void loadGameObjects();
		void loadSolarSystem();

		void sierpinski(std::vector<Model::Vertex>& vertices,
			int depth,
			glm::vec2 left,
			glm::vec2 right,
			glm::vec2 top);

		AppWindow appWindow{ WIDTH, HEIGHT, "RELAMPIAO Engine" };
		EngineDevice engineDevice{ appWindow };
		AppRenderer appRenderer{ appWindow, engineDevice };


		std::unique_ptr<AppDescriptorPool> globalPool{};
		std::unordered_map<GameObject::id_t, std::vector<VkDescriptorSet>> objectDescriptorSets;

		GameObject::Map gameObjects;
	};
}


