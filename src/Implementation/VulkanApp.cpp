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
		//std::cout << "offsetof(GlobalUbo, pointLights): " << offsetof(GlobalUbo, pointLights) << std::endl;
		loadGameObjects();

		int meshCount = 0;
		for (auto& kv : gameObjects) {
			if (kv.second.model != nullptr) meshCount++;
		}
		int totalSets = EngineSwapChain::MAX_FRAMES_IN_FLIGHT * (meshCount + 1);

		globalPool =
			AppDescriptorPool::Builder(engineDevice)
			.setMaxSets(totalSets)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, totalSets)
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, totalSets)
			.build();

	}

	VulkanApp::~VulkanApp() {}

	void VulkanApp::run()
	{
		std::vector<std::unique_ptr<AppBuffer>> uboBuffers(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < uboBuffers.size(); i++) {
			uboBuffers[i] = std::make_unique<AppBuffer>(engineDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uboBuffers[i]->map();
		}

		auto globalSetLayout =
			AppDescriptorSetLayout::Builder(engineDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();
		std::vector<VkDescriptorSet> globalDescriptorSets(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);
		auto defaultTexture = std::make_shared<Texture>(engineDevice, "../Models/white.png");

		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			auto imageInfo = defaultTexture->getDescriptorInfo();
			AppDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.writeImage(1, &imageInfo)
				.build(globalDescriptorSets[i]);
		}

		for (auto& kv : gameObjects) {
			auto& obj = kv.second;
			if (obj.model == nullptr) continue;

			auto& tex = obj.texture ? obj.texture : defaultTexture;
			objectDescriptorSets[obj.getId()].resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

			for (int i = 0; i < EngineSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
				auto bufferInfo = uboBuffers[i]->descriptorInfo();
				auto imageInfo = tex->getDescriptorInfo();
				AppDescriptorWriter(*globalSetLayout, *globalPool)
					.writeBuffer(0, &bufferInfo)
					.writeImage(1, &imageInfo)
					.build(objectDescriptorSets[obj.getId()][i]);
			}
		}

		Camera camera{};
		auto viewerObject = GameObject::createGameObject();
		viewerObject.transform.translation = { -1.f, -1.0f, 2.0f };
		glm::vec3 directionToOrigin = glm::normalize(glm::vec3(0.0f) - viewerObject.transform.translation);
		viewerObject.transform.rotation.y = atan2(directionToOrigin.x, directionToOrigin.z); // yaw
		viewerObject.transform.rotation.x = asin(-directionToOrigin.y); // pitch

		auto currentTime = std::chrono::high_resolution_clock::now();

		InitialRenderSystem initialRenderSystem{ engineDevice, appRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
		PointLightSystem pointLightSystem{ engineDevice, appRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
		KeyboardController controler{ };

		CpuRaytracer raytracer{ WIDTH, HEIGHT, 3 };
		float aspect = appRenderer.getAspectRatio();
		GlobalUbo ubo{};

		while (!appWindow.shouldClose()) {
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime =
				std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			controler.moveInPlaneXZ(appWindow.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			for (auto& kv : gameObjects) {
				auto& obj = kv.second;

				// Tenta fazer cast para Sphere&
				try {
					Sphere& sphere = dynamic_cast<Sphere&>(obj);
					sphere.update(frameTime);
				}
				catch (const std::bad_cast&) {
					// Não é uma Sphere, ignora
				}
			}

			float aspect = appRenderer.getAspectRatio();
			//camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);

			if (auto commandBuffer = appRenderer.beginFrame()) {
				int frameIndex = appRenderer.getCurrentFrameIndex();

				std::unordered_map<GameObject::id_t, VkDescriptorSet> frameDescriptorSets;
				for (auto& kv : objectDescriptorSets) {
					frameDescriptorSets[kv.first] = kv.second[frameIndex];
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
				initialRenderSystem.renderGameObjects(frameInfo);
				pointLightSystem.render(frameInfo);
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

	void VulkanApp::loadGameObjects()
	{
		// Sol como referência
		float sunRadius = 1.0f;

		// Escalas separadas para tamanho e distância
		float SIZE_SCALE = 2.0f;    // Tamanhos relativos
		float DIST_SCALE = 1.0f;    // Distâncias

		// ========== SOL ==========
		auto sunMesh = Sphere::createSphere(engineDevice, sunRadius, false);
		sunMesh.transform.translation = { 0.0f, 0.0f, 0.0f };
		sunMesh.color = { 1.0f, 0.9f, 0.3f };
		sunMesh.texture = std::make_shared<Texture>(engineDevice, "../Models/sun.jpg");
		gameObjects.emplace(sunMesh.getId(), std::move(sunMesh));

		// Luz do sol
		auto sunLight = GameObject::makePointLight(200.0f);
		sunLight.color = { 1.0f, 1.0f, 1.0f };
		sunLight.transform.translation = { 0.0f, 0.0f, 0.0f };
		sunLight.pointLight->lightIntensity = 100000;
		gameObjects.emplace(sunLight.getId(), std::move(sunLight));

		// ========== PLANETAS ==========

		// Mercúrio
		float mercuryRadius = sunRadius * 0.0038f * SIZE_SCALE * 50;
		float mercuryDist = 12.0f * DIST_SCALE;
		auto mercury = Sphere::createSphere(engineDevice, mercuryRadius, false);
		mercury.transform.translation = { mercuryDist, 0.0f, 0.0f };
		mercury.color = { 0.8f, 0.6f, 0.4f };
		mercury.orbitRadius = 12.0f;
		mercury.orbitSpeed = 4.0f;
		mercury.texture = std::make_shared<Texture>(engineDevice, "../Models/mercury.jpg");
		gameObjects.emplace(mercury.getId(), std::move(mercury));

		// Vênus
		float venusRadius = sunRadius * 0.0095f * SIZE_SCALE * 50;
		float venusDist = 22.0f * DIST_SCALE;
		auto venus = Sphere::createSphere(engineDevice, venusRadius, false);
		venus.transform.translation = { venusDist, 0.0f, 0.0f };
		venus.color = { 1.0f, 0.8f, 0.6f };
		venus.texture = std::make_shared<Texture>(engineDevice, "../Models/venus.jpg");
		gameObjects.emplace(venus.getId(), std::move(venus));

		// Terra
		float earthRadius = sunRadius * 0.01f * SIZE_SCALE * 50;
		float earthDist = 30.0f * DIST_SCALE;
		auto earth = Sphere::createSphere(engineDevice, earthRadius, false);
		earth.transform.translation = { earthDist, 0.0f, 0.0f };
		earth.color = { 0.2f, 0.4f, 0.8f };
		earth.texture = std::make_shared<Texture>(engineDevice, "../Models/earth.jpg");
		gameObjects.emplace(earth.getId(), std::move(earth));
	}
}