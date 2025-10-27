#pragma once

#include "Camera.h"
#include <vulkan/vulkan.h>

namespace app{

	#define MAX_LIGHTS 10

	struct PointLight {
		glm::vec4 position{};
		glm::vec4 color{};
		//float radius = 1.f;
	};

	struct GlobalUbo {
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 inverseView{ 1.f };
		glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };  // w is intensity
		PointLight pointLights[MAX_LIGHTS];
		int numLights;
	};

	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		Camera& camera;
		VkDescriptorSet globalDescriptorSet;
		GameObject::Map& gameObjects;
	};
}
