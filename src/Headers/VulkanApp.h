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
#include "EngineImgui.h"
#include "SkyboxRenderSystem.h"
#include "CubemapTexture.h"
#include "Scene.h"

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
		void loadScene(const Scene& scene, GameObject& viewer);

	private:
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
		std::unique_ptr<AppDescriptorSetLayout> globalSetLayout; 
		std::vector<std::unique_ptr<AppBuffer>> uboBuffers;     
		std::shared_ptr<Texture> defaultTexture;

		GameObject::Map gameObjects;

		std::unique_ptr<EngineImgui> engineImgui;

		std::unique_ptr<SkyboxRenderSystem> skyboxSystem;
		std::shared_ptr<CubemapTexture> skyboxCubemap;

		Scene activeScene;

		void rebuildObjectDescriptorSets();

	};
}


