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
			if (kv.second->model != nullptr) meshCount++;
		}
		int totalSets = EngineSwapChain::MAX_FRAMES_IN_FLIGHT * (meshCount + 1); // With high values we can change scenes turn off by default

		globalPool =
			AppDescriptorPool::Builder(engineDevice)
			.setMaxSets(totalSets)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, totalSets)
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, totalSets)
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
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		defaultTexture = std::make_shared<Texture>(engineDevice, "../Models/white.png");
		rebuildObjectDescriptorSets();

		std::vector<VkDescriptorSet> globalDescriptorSets(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			auto imageInfo = defaultTexture->getDescriptorInfo();
			AppDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.writeImage(1, &imageInfo)
				.build(globalDescriptorSets[i]);
		}

		Camera camera{};
		auto viewerObject = GameObject::createGameObject();
		viewerObject.transform.translation = { 0, 0, 2.0f };
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
			auto request = engineImgui->getSceneRequest();
			if (request != SceneRequest::None) {
				vkDeviceWaitIdle(engineDevice.device()); // Para tudo!

				if (request == SceneRequest::Default) loadGameObjects();
				else if (request == SceneRequest::SolarSystem) loadSolarSystem();

				// Reconstrua o mapa ANTES de continuar
				rebuildObjectDescriptorSets();
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

	void VulkanApp::loadGameObjects()
	{
		vkDeviceWaitIdle(engineDevice.device());
		gameObjects.clear();
		std::shared_ptr<Model> model = Model::createModelFromFile(engineDevice, "models/smooth_vase.obj");
		auto flat_vase = std::make_unique<GameObject>(GameObject::createGameObject());
		flat_vase->model = model;
		flat_vase->transform.translation = { 0.0f, .0f, 0.0f };
		flat_vase->transform.scale = { 0.1f,0.1f, 0.1f };
		//flat_vase->texture = std::make_shared<Texture>(engineDevice, "../Models/bricks.png");

		gameObjects.emplace(flat_vase->getId(), std::move(flat_vase));

		model = Model::createModelFromFile(engineDevice, "models/quad.obj");
		auto floor = std::make_unique<GameObject>(GameObject::createGameObject());
		floor->model = model;
		floor->transform.translation = { 0.f, .5f, 0.f };
		floor->transform.scale = { 3.f, 1.f, 3.f };
		//floor->texture = std::make_shared<Texture>(engineDevice, "../Models/1.png");
		gameObjects.emplace(floor->getId(), std::move(floor));

		//auto sphere = Sphere::createSphere(engineDevice, 0.5f, true);
		//gameObjects.emplace(sphere.getId(), std::move(sphere));

		//Sphere sphere1 = Sphere::createSphere(2.0f, glm::vec3(1.0f, 0.0f, 0.0f)); // Red sphere with radius 2
		//gameObjects.emplace(sphere1.getId(), std::move(sphere1));

		std::vector<glm::vec3> lightColors{
		  {1.f, .1f, .1f},
		  {.1f, .1f, 1.f},
		  {.1f, 1.f, .1f},
		  {1.f, 1.f, .1f},
		  {.1f, 1.f, 1.f},
		  {1.f, 1.f, 1.f}  //
		};

		for (int i = 0; i < lightColors.size(); i++) {
			auto pointLight = std::make_unique<GameObject>(GameObject::makePointLight(100.0f));
			pointLight->color = lightColors[i];

			auto rotateLight = glm::rotate(
				glm::mat4(1.f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{ 0.f, -1.f, 0.f });

			pointLight->transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
			gameObjects.emplace(pointLight->getId(), std::move(pointLight));
		}
	}
	void VulkanApp::loadSolarSystem()
	{
		vkDeviceWaitIdle(engineDevice.device()); 
		gameObjects.clear();
		// 1. Configurações Globais
		float sunRadius = 20.0f;        // Sol um pouco maior para destaque
		float SIZE_SCALE = 2.0f;        // Escala global de tamanho
		float DIST_SCALE = 100.0f;       // Escala de distância entre órbitas
		float baseSpeed = glm::two_pi<float>() / 10.f; // 10 segundos por volta (0.628)

		// ========== SOL ==========
		auto sunMesh = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius, false));
		sunMesh->transform.translation = { 0.0f, 0.0f, 0.0f };
		sunMesh->color = { 1.0f, 0.9f, 0.3f };
		sunMesh->texture = std::make_shared<Texture>(engineDevice, "../Models/sun.jpg");
		baseSpeed / 25.0f;
		gameObjects.emplace(sunMesh->getId(), std::move(sunMesh));

		// Luz Centralizada
		auto sunLight = std::make_unique<GameObject>(GameObject::makePointLight(500.0f, 80.0f));
		sunLight->color = { 1.0f, 1.0f, 1.0f };
		sunLight->transform.translation = { 0.0f, 0.0f, 0.0f };
		gameObjects.emplace(sunLight->getId(), std::move(sunLight));

		// ========== PLANETAS ==========
		// Estrutura: Distância, Velocidade Órbita, Velocidade Rotação Própria

		// MERCÚRIO (O mais rápido em órbita)
		float mercDist = 12.0f * DIST_SCALE;
		auto mercury = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.004f * SIZE_SCALE * 10, false));
		mercury->transform.translation = { mercDist, 0.0f, 0.0f };
		mercury->orbitRadius = mercDist;
		mercury->orbitSpeed = 4.7f;
		mercury->selfRotationSpeed = baseSpeed / 58.6f;
		mercury->texture = std::make_shared<Texture>(engineDevice, "../Models/mercury.jpg");
		gameObjects.emplace(mercury->getId(), std::move(mercury));

		// VÊNUS (Rotação lenta)
		float venDist = 22.0f * DIST_SCALE;
		auto venus = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.009f * SIZE_SCALE * 10, false));
		venus->transform.translation = { venDist, 0.0f, 0.0f };
		venus->orbitRadius = venDist;
		venus->orbitSpeed = 3.5f;
		venus->selfRotationSpeed = baseSpeed / 243.0f;
		venus->texture = std::make_shared<Texture>(engineDevice, "../Models/venus.jpg");
		gameObjects.emplace(venus->getId(), std::move(venus));

		// TERRA
		float earthDist = 32.0f * DIST_SCALE;
		auto earth = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.01f * SIZE_SCALE * 10, false));
		earth->transform.translation = { earthDist, 0.0f, 0.0f };
		earth->orbitRadius = earthDist;
		earth->orbitSpeed = 2.4f;
		earth->selfRotationSpeed = baseSpeed;
		earth->texture = std::make_shared<Texture>(engineDevice, "../Models/earth.jpg");
		gameObjects.emplace(earth->getId(), std::move(earth));

		// MARTE
		float marsDist = 42.0f * DIST_SCALE;
		auto mars = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.005f * SIZE_SCALE * 10, false));
		mars->transform.translation = { marsDist, 0.0f, 0.0f };
		mars->orbitRadius = marsDist;
		mars->orbitSpeed = 2.0f;
		mars->selfRotationSpeed = baseSpeed / 1.03f;
		mars->texture = std::make_shared<Texture>(engineDevice, "../Models/mars.jpg");
		gameObjects.emplace(mars->getId(), std::move(mars));

		// JÚPITER (Gira muito rápido no próprio eixo)
		float jupDist = 60.0f * DIST_SCALE;
		auto jupiter = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.06f * SIZE_SCALE * 10, false));
		jupiter->transform.translation = { jupDist, 0.0f, 0.0f };
		jupiter->orbitRadius = jupDist;
		jupiter->orbitSpeed = 1.3f;
		jupiter->selfRotationSpeed = baseSpeed / 0.41f;
		jupiter->texture = std::make_shared<Texture>(engineDevice, "../Models/jupiter.jpg");
		gameObjects.emplace(jupiter->getId(), std::move(jupiter));

		// SATURNO
		float satDist = 80.0f * DIST_SCALE;
		auto saturn = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.05f * SIZE_SCALE * 10, false));
		saturn->transform.translation = { satDist, 0.0f, 0.0f };
		saturn->orbitRadius = satDist;
		saturn->orbitSpeed = 0.9f;
		saturn->selfRotationSpeed = baseSpeed / 0.45f;
		saturn->texture = std::make_shared<Texture>(engineDevice, "../Models/saturn.jpg");
		gameObjects.emplace(saturn->getId(), std::move(saturn));

		// URANO
		float uraDist = 100.0f * DIST_SCALE;
		auto uranus = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.02f * SIZE_SCALE * 10, false));
		uranus->transform.translation = { uraDist, 0.0f, 0.0f };
		uranus->orbitRadius = uraDist;
		uranus->orbitSpeed = 0.6f;
		uranus->selfRotationSpeed = baseSpeed / 0.72f;
		uranus->texture = std::make_shared<Texture>(engineDevice, "../Models/uranus.jpg");
		gameObjects.emplace(uranus->getId(), std::move(uranus));

		// NETUNO
		float nepDist = 120.0f * DIST_SCALE;
		auto neptune = std::make_unique<Sphere>(Sphere::createSphere(engineDevice, sunRadius * 0.02f * SIZE_SCALE * 10, false));
		neptune->transform.translation = { nepDist, 0.0f, 0.0f };
		neptune->orbitRadius = nepDist;
		neptune->orbitSpeed = 0.5f;
		neptune->selfRotationSpeed = baseSpeed / 0.67f;
		neptune->texture = std::make_shared<Texture>(engineDevice, "../Models/neptune.jpg");
		gameObjects.emplace(neptune->getId(), std::move(neptune));
	}

	void VulkanApp::rebuildObjectDescriptorSets() {
		objectDescriptorSets.clear();

		for (auto& kv : gameObjects) {
			auto& obj = kv.second;
			if (obj->model == nullptr) continue;

			auto& tex = obj->texture ? obj->texture : defaultTexture;
			auto id = obj->getId();

			objectDescriptorSets[id].resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

			for (int i = 0; i < EngineSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
				auto bufferInfo = uboBuffers[i]->descriptorInfo();
				auto imageInfo = tex->getDescriptorInfo();

				AppDescriptorWriter(*globalSetLayout, *globalPool)
					.writeBuffer(0, &bufferInfo)
					.writeImage(1, &imageInfo)
					.build(objectDescriptorSets[id][i]);
			}
		}
	}
}