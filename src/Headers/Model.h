#pragma once
#include <vector>
#include <memory>
#include <cstdint>    // for uint32_t
#include <cmath>
#include "EngineDevice.h"
#include "AppBuffer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
namespace app {
	class Model
	{
	public:
		struct Vertex {
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};
			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

			bool operator==(const Vertex& other) const {
				return position == other.position && color == other.color && normal == other.normal &&
					uv == other.uv;
			}

		};

		struct ModelBuilder {
			std::vector<Model::Vertex> vertices{};
			std::vector<uint32_t> indices{};
			void loadModel(const std::string& filepath);
			void createSphere(float radius, uint32_t sectorCount = 36, uint32_t stackCount = 18);
		};

		Model(EngineDevice& device, const ModelBuilder& builder);
		~Model();

		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;


		static std::unique_ptr<Model> createModelFromFile(EngineDevice& device, const std::string& filePath);
		static std::unique_ptr<Model> createSphereModel(
			EngineDevice& device,
			float radius,
			uint32_t sectorCount = 36,
			uint32_t stackCount = 18);

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);


		EngineDevice& appDevice;

		std::unique_ptr<AppBuffer> vertexBuffer;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		std::unique_ptr<AppBuffer> indexBuffer;
		uint32_t indexCount;
	};
}


