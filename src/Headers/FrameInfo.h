#pragma once
#include "Camera.h"
#include "GameObject.h"
#include <vulkan/vulkan.h>
#include <unordered_map>

namespace app{

	#define MAX_LIGHTS 10

	struct PointLight {
		glm::vec4 position{};    // 16 bytes - offset 0
		glm::vec4 color{};       // 16 bytes - offset 16  
		glm::vec4 data{};        // 12 bytes - offset 36
	}; // Total: 48 bytes

	


	struct GlobalUbo {
		glm::mat4 projection{ 1.f };        // 64 bytes - offset 0
		glm::mat4 view{ 1.f };              // 64 bytes - offset 64  
		glm::mat4 inverseView{ 1.f };       // 64 bytes - offset 128
		glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f }; // 16 bytes - offset 192

		glm::vec4 numLightsAndPad{0.f};
		PointLight pointLights[MAX_LIGHTS]; // 480 bytes - offset 224
		glm::vec4 directionalLightDir{ glm::normalize(glm::vec3(-1.f, -3.f, -1.f)), 1.f }; // xyz = dir, w = intensity
		glm::vec4 directionalLightColor{ 1.f, 1.f, 1.f, 1.f };
	}; 


	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		Camera& camera;
		VkDescriptorSet globalDescriptorSet;
		std::unordered_map<GameObject::id_t, VkDescriptorSet>& objectDescriptorSets;
		GameObject::Map& gameObjects;
	};
}
