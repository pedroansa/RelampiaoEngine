#pragma once
#include "EngineDevice.h"
#include <string>
#include <algorithm> // Para std::max e std::min
#include <cmath>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace app {

    class Texture {
    public:
        // Carrega a textura do disco e faz upload para a GPU
        // filepath — caminho relativo ao executavel (ex: "textures/lenin.png")
        Texture(EngineDevice& device, const std::string& filepath, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        // Retorna o ImageView — usado para criar o descriptor
        VkImageView getImageView() const { return imageView; }

        // Retorna o Sampler — define filtro e wrap mode
        VkSampler getSampler() const { return sampler; }

        // Retorna as infos necessarias para escrever no descriptor set
        VkDescriptorImageInfo getDescriptorInfo() const;

        glm::vec3 sampleUV(float u, float v) const;

        // CPU pixel data — populated during load, kept for raytracing
        std::vector<uint8_t> cpuPixels;
        int texWidth = 0;
        int texHeight = 0;

    protected:
        Texture(EngineDevice& device) : device{ device } {} // for subclasses
        void createTextureImage(const std::string& filepath);
        void createImageView();
        void createSampler();
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
        void generateMipmaps();
        EngineDevice& device;
        VkImage image;
        VkDeviceMemory imageMemory;
        VkImageView imageView;
        VkSampler sampler;
        uint32_t width, height;
        uint32_t mipLevels;
        VkSamplerAddressMode addressMode;
        VkFormat format;
    };

} // namespace app