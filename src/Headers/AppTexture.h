#pragma once
#include "EngineDevice.h"
#include <string>
#include <vulkan/vulkan.h>

namespace app {

    class Texture {
    public:
        // Carrega a textura do disco e faz upload para a GPU
        // filepath — caminho relativo ao executavel (ex: "textures/lenin.png")
        Texture(EngineDevice& device, const std::string& filepath);
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        // Retorna o ImageView — usado para criar o descriptor
        VkImageView getImageView() const { return imageView; }

        // Retorna o Sampler — define filtro e wrap mode
        VkSampler getSampler() const { return sampler; }

        // Retorna as infos necessarias para escrever no descriptor set
        VkDescriptorImageInfo getDescriptorInfo() const;

    private:
        void createTextureImage(const std::string& filepath);
        void createImageView();
        void createSampler();
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);

        EngineDevice& device;
        VkImage image;
        VkDeviceMemory imageMemory;
        VkImageView imageView;
        VkSampler sampler;
        uint32_t width, height;
    };

} // namespace app