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

		globalPool = 
			AppDescriptorPool::Builder(engineDevice)
			.setMaxSets(EngineSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, EngineSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, EngineSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();
		loadGameObjects();
	}

	VulkanApp::~VulkanApp(){}

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
		auto leninTexture = std::make_shared<Texture>(engineDevice, "../Models/1.png");

		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			auto imageInfo = leninTexture->getDescriptorInfo();
			AppDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.writeImage(1, &imageInfo)
				.build(globalDescriptorSets[i]);
		}

		Camera camera{};
		auto viewerObject = GameObject::createGameObject();
		viewerObject.transform.translation = { -1.f, -1.0f, 2.0f };
		glm::vec3 directionToOrigin = glm::normalize(glm::vec3(0.0f) - viewerObject.transform.translation);
		viewerObject.transform.rotation.y = atan2(directionToOrigin.x, directionToOrigin.z); // yaw
		viewerObject.transform.rotation.x = asin(-directionToOrigin.y); // pitch
		 
		auto currentTime = std::chrono::high_resolution_clock::now();

		InitialRenderSystem initialRenderSystem{ engineDevice, appRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
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

			float aspect = appRenderer.getAspectRatio();
			//camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);

			if (auto commandBuffer = appRenderer.beginFrame()) {
				int frameIndex = appRenderer.getCurrentFrameIndex();
				FrameInfo frameInfo{ frameIndex, frameTime, commandBuffer, camera,  globalDescriptorSets[frameIndex], gameObjects };

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
		std::shared_ptr<Model> model = Model::createModelFromFile(engineDevice, "models/lenin.obj");
		auto flat_vase = GameObject::createGameObject();
		flat_vase.model = model;
		flat_vase.transform.translation = { 0.0f, .0f, 0.0f };
		flat_vase.transform.scale = { 0.1f, 0.1f, 0.1f };
		flat_vase.transform.rotation = { 1.5f , 1.5f, 0.0f };
		gameObjects.emplace(flat_vase.getId(), std::move(flat_vase));

		model = Model::createModelFromFile(engineDevice, "models/quad.obj");
		auto floor = GameObject::createGameObject();
		floor.model = model;
		floor.transform.translation = { 0.f, .5f, 0.f };
		floor.transform.scale = { 3.f, 1.f, 3.f };
		gameObjects.emplace(floor.getId(), std::move(floor));

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
			auto pointLight = GameObject::makePointLight(100.0f);
			pointLight.color = lightColors[i];

			auto rotateLight = glm::rotate(
				glm::mat4(1.f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{ 0.f, -1.f, 0.f });

			pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
			gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}
	}

}