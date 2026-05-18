#include "VulkanApp.h"
#include <iostream>
#include "AppBuffer.h"
#include "Sphere.h"
#include "CpuRaytracer.h"
#include <AppTexture.h>

namespace app {

	VulkanApp::VulkanApp()
	{
		std::cout << "=== UBO Structure Debug ===" << std::endl;
		std::cout << "sizeof(GlobalUbo): " << sizeof(GlobalUbo) << std::endl;
		std::cout << "sizeof(PointLight): " << sizeof(PointLight) << std::endl;

		const uint32_t MAX_SCENE_SETS = 500; 

		globalPool =
			AppDescriptorPool::Builder(engineDevice)
			.setMaxSets(MAX_SCENE_SETS)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_SCENE_SETS)
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_SCENE_SETS)
			.build();
		engineImgui = std::make_unique<EngineImgui>(
			appWindow,
			engineDevice,
			appRenderer.getSwapChainRenderPass(),
			EngineSwapChain::MAX_FRAMES_IN_FLIGHT
		);

	}

	VulkanApp::~VulkanApp() {}

	void VulkanApp::run()
	{
		uboBuffers.resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); i++) {
			uboBuffers[i] = std::make_unique<AppBuffer>(engineDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uboBuffers[i]->map();
		}

		globalSetLayout = AppDescriptorSetLayout::Builder(engineDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // albedo
			.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // normal
			.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // metallic/roughness
			.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // ao
			.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // ao
			.build();

		defaultTexture = std::make_shared<Texture>(engineDevice, "../Models/white.png");

		Scene defaultScene;
		defaultScene.name = "Default";
		defaultScene.cameraConfig.position = { -1.f, -1.f, 2.f };
		defaultScene.cameraConfig.farPlane = 1000.f;
		defaultScene.load = [this](GameObject::Map& objects, EngineDevice& device) {
			Scene::loadGameObjects(objects, device);
			};

		Scene solarScene;
		solarScene.name = "Solar System";
		solarScene.cameraConfig.position = { 0.f, 0.f, 3000.f };
		solarScene.cameraConfig.farPlane = 5000000.f;
		solarScene.load = [this](GameObject::Map& objects, EngineDevice& device) {
			Scene::loadSolarSystem(objects, device);
			};

		Camera camera{};
		auto viewerObject = GameObject::createGameObject();
		viewerObject.transform.translation = { 0, 0, 2.0f };
		glm::vec3 directionToOrigin = glm::normalize(glm::vec3(0.0f) - viewerObject.transform.translation);
		viewerObject.transform.rotation.y = atan2(directionToOrigin.x, directionToOrigin.z); // yaw
		viewerObject.transform.rotation.x = asin(-directionToOrigin.y); // pitch

		loadScene(defaultScene, viewerObject);

		std::vector<VkDescriptorSet> globalDescriptorSets(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			auto imageInfo = defaultTexture->getDescriptorInfo();
			AppDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.writeImage(1, &imageInfo)
				.build(globalDescriptorSets[i]);
		}

		auto currentTime = std::chrono::high_resolution_clock::now();

		InitialRenderSystem initialRenderSystem{ engineDevice, appRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
		PointLightSystem pointLightSystem{ engineDevice, appRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
		KeyboardController controler{ };

		CpuRaytracer raytracer{ WIDTH, HEIGHT, 3 };
		float aspect = appRenderer.getAspectRatio();

		skyboxCubemap = std::make_shared<CubemapTexture>(engineDevice, std::array<std::string, 6>{
			"../Models/Skybox/px.png",
				"../Models/Skybox/nx.png",
				"../Models/Skybox/py.png",
				"../Models/Skybox/ny.png",
				"../Models/Skybox/pz.png",
				"../Models/Skybox/nz.png"
		});
		skyboxSystem = std::make_unique<SkyboxRenderSystem>(
			engineDevice, appRenderer.getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout(), skyboxCubemap);

		GlobalUbo ubo{};

		while (!appWindow.shouldClose()) {
			glfwPollEvents();
			auto request = engineImgui->getSceneRequest();
			if (request != SceneRequest::None) {
				vkDeviceWaitIdle(engineDevice.device()); // Para tudo!

				if (request == SceneRequest::Default) loadScene(defaultScene, viewerObject);
				else if (request == SceneRequest::SolarSystem) loadScene(solarScene, viewerObject);

				for (int i = 0; i < globalDescriptorSets.size(); i++) {
					auto bufferInfo = uboBuffers[i]->descriptorInfo();
					auto imageInfo = defaultTexture->getDescriptorInfo();
					AppDescriptorWriter(*globalSetLayout, *globalPool)
						.writeBuffer(0, &bufferInfo)
						.writeImage(1, &imageInfo)
						.build(globalDescriptorSets[i]);
				}
				engineImgui->resetSceneRequest();
				currentTime = std::chrono::high_resolution_clock::now();
				continue; // Pula o restante deste frame para limpar o cache do Command Buffer
			}

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime =
				std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			controler.moveInPlaneXZ(appWindow.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			for (auto& kv : gameObjects) {
				kv.second->update(frameTime);
			}

			float aspect = appRenderer.getAspectRatio();
			//camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 5000000.f);

			if (auto commandBuffer = appRenderer.beginFrame()) {
				int frameIndex = appRenderer.getCurrentFrameIndex();

				engineImgui->newFrame();
				engineImgui->runDefaultUi(initialRenderSystem, gameObjects);

				std::unordered_map<GameObject::id_t, VkDescriptorSet> frameDescriptorSets;
				for (auto& kv : objectDescriptorSets) {
					if (frameIndex < kv.second.size()) {
						frameDescriptorSets[kv.first] = kv.second[frameIndex];
					}
				}

				FrameInfo frameInfo{ frameIndex, frameTime, commandBuffer, camera,  globalDescriptorSets[frameIndex], frameDescriptorSets, gameObjects };

				// Update memory
				GlobalUbo ubo{};
				ubo.projection = camera.getProjection();
				ubo.view = camera.getView();
				ubo.inverseView = camera.getInverseView();
				pointLightSystem.update(frameInfo, ubo);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				controler.handleInput(appWindow.getGLFWwindow(), initialRenderSystem,
					raytracer, gameObjects, ubo, camera, glm::radians(50.f), aspect);

				// Render
				appRenderer.beginSwapChainRenderPass(commandBuffer);
				skyboxSystem->render(frameInfo);       
				initialRenderSystem.renderGameObjects(frameInfo);
				pointLightSystem.render(frameInfo);
				engineImgui->render(commandBuffer);
				appRenderer.endSwapChainRenderPass(commandBuffer);
				appRenderer.endFrame();

			}
		}

		vkDeviceWaitIdle(engineDevice.device());
	}

	std::unique_ptr<Model> createCubeModel(EngineDevice& device, glm::vec3 offset) {
		Model::ModelBuilder modelBuilder{};
		modelBuilder.vertices = {

			// left face (white)
			{{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
			{{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
			{{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
			{{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},

			// right face (yellow)
			{{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
			{{.5f, .5f, .5f}, {.8f, .8f, .1f}},
			{{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
			{{.5f, .5f, -.5f}, {.8f, .8f, .1f}},

			// top face (orange, remember y axis points down)
			{{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
			{{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
			{{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
			{{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},

			// bottom face (red)
			{{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
			{{.5f, .5f, .5f}, {.8f, .1f, .1f}},
			{{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
			{{.5f, .5f, -.5f}, {.8f, .1f, .1f}},

			// nose face (blue)
			{{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
			{{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
			{{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
			{{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},

			// tail face (green)
			{{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
			{{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
			{{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
			{{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},

		};
		for (auto& v : modelBuilder.vertices) {
			v.position += offset;
		}
		modelBuilder.indices = { 0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
			12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21 };

		return std::make_unique<Model>(device, modelBuilder);
	}

	void VulkanApp::rebuildObjectDescriptorSets() {
		objectDescriptorSets.clear();

		for (auto& kv : gameObjects) {
			auto& obj = kv.second;
			if (obj->model == nullptr) continue;

			auto id = obj->getId();

			objectDescriptorSets[id].resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

			for (int i = 0; i < EngineSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
				auto bufferInfo = uboBuffers[i]->descriptorInfo();
				auto albedoInfo = (obj->material.albedo ? obj->material.albedo : defaultTexture)->getDescriptorInfo();
				auto normalInfo = (obj->material.normalMap ? obj->material.normalMap : defaultTexture)->getDescriptorInfo();
				auto mInfo = (obj->material.metallic ? obj->material.metallic : defaultTexture)->getDescriptorInfo();
				auto rInfo = (obj->material.roughness ? obj->material.roughness : defaultTexture)->getDescriptorInfo();
				auto aoInfo = (obj->material.ao ? obj->material.ao : defaultTexture)->getDescriptorInfo();

				AppDescriptorWriter(*globalSetLayout, *globalPool)
					.writeBuffer(0, &bufferInfo)
					.writeImage(1, &albedoInfo)
					.writeImage(2, &normalInfo)
					.writeImage(3, &mInfo)
					.writeImage(4, &rInfo)
					.writeImage(5, &aoInfo)
					.build(objectDescriptorSets[id][i]);
			}
		}
	}
	void VulkanApp::loadScene(const Scene& scene, GameObject& viewer) {
		vkDeviceWaitIdle(engineDevice.device());
		activeScene = scene;
		gameObjects.clear();
		if (scene.load) scene.load(gameObjects, engineDevice);
		rebuildObjectDescriptorSets();

		viewer.transform.translation = scene.cameraConfig.position;
		viewer.transform.rotation = scene.cameraConfig.rotation;
	}

}