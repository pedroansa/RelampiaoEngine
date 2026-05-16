#pragma once
#include <string>
#include <vector>
#include <cassert>
#include "EngineDevice.h"
#include "Model.h"

namespace app {

    struct VertexInputConfig {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    };

	struct PipelineConfigInfo {

        PipelineConfigInfo() {
            // Inicializa todas as structs Vulkan com zeros e define seus sTypes
            viewportinfo = {};
            viewportinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

            inputAssemblyInfo = {};
            inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

            rasterizationInfo = {};
            rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

            multisampleInfo = {};
            multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

            colorBlendAttachment = {};

            colorBlendInfo = {};
            colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

            depthStencilInfo = {};
            depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

            dynamicStateInfo = {};
            dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

            // Configurações padrão importantes
            inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

            rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizationInfo.lineWidth = 1.0f;

            multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

            colorBlendInfo.attachmentCount = 1;
            colorBlendInfo.pAttachments = &colorBlendAttachment;

            // Configurações padrão para dynamic state
            dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
            dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
        }
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		VkPipelineViewportStateCreateInfo viewportinfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;

		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};

	class AppPipeline
	{
 
	public:
		AppPipeline(EngineDevice& device,
			const std::string& vertFilepath, 
			const std::string& fragFilepath,
			const PipelineConfigInfo& configInfo);

        AppPipeline(EngineDevice& device,
            const std::string& vertFilepath,
            const std::string& fragFilepath,
            const PipelineConfigInfo& configInfo,
            const VertexInputConfig& vertexInputConfig);
      
		~AppPipeline();

		AppPipeline(const AppPipeline&) = delete;
		AppPipeline& operator=(const AppPipeline&) = delete;
		void bind(VkCommandBuffer commandBuffer);

	static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
    static void enableAlphaBlending(PipelineConfigInfo& configInfo);

       
    // Métodos estáticos para configurar diferentes modos
    static void wireframePipelineConfigInfo(PipelineConfigInfo& configInfo);
    static void pointsPipelineConfigInfo(PipelineConfigInfo& configInfo);

    static VertexInputConfig getSolidVertexInputConfig();
    static VertexInputConfig getWireframeVertexInputConfig();
    static VertexInputConfig getPointsVertexInputConfig();

	private:
		static std::vector<char> readFile(const std::string& filepath);
		void createGraphicsPipeline(const std::string& vertFilepath, 
			const std::string& fragFilepath,
			const PipelineConfigInfo& configInfo,
            const VertexInputConfig& vertexInputConfig);

		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

		EngineDevice& appDevice;
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;


	};
}


