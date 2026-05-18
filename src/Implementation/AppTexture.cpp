#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "AppTexture.h"
#include <stdexcept>

namespace app {

    Texture::Texture(EngineDevice& device, const std::string& filepath, VkFormat format, VkSamplerAddressMode addressMode) : device{ device }, format{ format }, addressMode { addressMode } {
    createTextureImage(filepath);
    createImageView();
    createSampler();
}

Texture::~Texture() {
    vkDestroyImage(device.device(), image, nullptr);
    vkFreeMemory(device.device(), imageMemory, nullptr);
    vkDestroyImageView(device.device(), imageView, nullptr);
    vkDestroySampler(device.device(), sampler, nullptr);
}

void Texture::createTextureImage(const std::string& filepath) {
    // 1. Carrega pixels do disco em CPU
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) throw std::runtime_error("Falha ao carregar textura: " + filepath);

    width  = static_cast<uint32_t>(texWidth);
    height = static_cast<uint32_t>(texHeight);
    VkDeviceSize imageSize = width * height * 4; // 4 bytes por pixel (RGBA)

    // 2. Cria staging buffer em CPU e copia pixels
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    device.createBuffer(imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(device.device(), stagingMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device.device(), stagingMemory);
    // Copia antes para usarmos no raytracing
    this->texWidth = static_cast<int>(width);
    this->texHeight = static_cast<int>(height);
    cpuPixels.resize(width * height * 4);
    memcpy(cpuPixels.data(), pixels, width * height * 4);

    stbi_image_free(pixels); // pixels em CPU nao sao mais necessarios

    // 3. Cria VkImage na GPU
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = mipLevels;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = this->format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL; // melhor performance na GPU
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

    // 4. Transiciona layout: UNDEFINED -> TRANSFER_DST (pronto para receber dados)
    transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // 5. Copia staging buffer para a imagem na GPU
    device.copyBufferToImage(stagingBuffer, image, width, height, 1);

    // 6. Transiciona layout: TRANSFER_DST -> SHADER_READ_ONLY (pronto para o shader ler)
    //transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // 7. Limpa staging buffer
    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingMemory, nullptr);

    generateMipmaps();
}


void Texture::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer cmd = device.beginSingleTimeCommands();

    // Uma barrier de pipeline que sincroniza acesso a imagem e muda o layout
    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags srcStage, dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        // Antes do upload: nao precisa esperar nada, destino e o transfer
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // Apos upload: espera o transfer terminar, destino e o fragment shader
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    } else {
        throw std::runtime_error("Transicao de layout nao suportada");
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    device.endSingleTimeCommands(cmd);
}

void Texture::createImageView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format   = this->format;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        throw std::runtime_error("Falha ao criar image view da textura");
}

void Texture::createSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter    = VK_FILTER_LINEAR; // suaviza quando ampliado
    samplerInfo.minFilter    = VK_FILTER_LINEAR; // suaviza quando reduzido
    samplerInfo.addressModeU = addressMode; // Trava no limite em U
    samplerInfo.addressModeV = addressMode; // Trava no limite em V
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable        = VK_TRUE;
    samplerInfo.maxAnisotropy           = device.properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE; // UVs em [0,1] nao em pixels
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;                          // Começa no Mip 0 (máxima resolução)
    samplerInfo.maxLod = static_cast<float>(mipLevels); // Vai até o último nível (mínima resolução)
    samplerInfo.mipLodBias = 0.0f;                          // Ajuste fino opcional de nitidez

    if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("Falha ao criar sampler da textura");
}

VkDescriptorImageInfo Texture::getDescriptorInfo() const {
    VkDescriptorImageInfo info{};
    info.sampler     = sampler;
    info.imageView   = imageView;
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return info;
}

glm::vec3 Texture::sampleUV(float u, float v) const {
    if (cpuPixels.empty() || texWidth <= 0 || texHeight <= 0) return glm::vec3(1.f);

    float u_repeat = u - std::floor(u);
    float v_repeat = v - std::floor(v);
    v_repeat = 1.f - v_repeat;
    // Converte a fração [0, 1) para o índice inteiro de pixels da imagem
    int x = static_cast<int>(u_repeat * texWidth);
    int y = static_cast<int>(v_repeat * texHeight);

    // Garante por segurança que arredondamentos de ponto flutuante não estourem os limites do vetor
    x = std::max(0, std::min(x, texWidth - 1));
    y = std::max(0, std::min(y, texHeight - 1));

    // Cada pixel tem 4 canais (RGBA)
    int idx = (y * texWidth + x) * 4;
    return glm::vec3(
        cpuPixels[idx + 0] / 255.f,
        cpuPixels[idx + 1] / 255.f,
        cpuPixels[idx + 2] / 255.f
    );
}
void Texture::generateMipmaps() {
    // Checa se o hardware suporta filtragem linear para blit
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(device.getPhysicalDevice(), this->format, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("O formato da textura nao suporta filtragem linear para blitting!");
    }

    VkCommandBuffer cmd = device.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1; // Processa um nível por vez

    int32_t mipWidth = width;
    int32_t mipHeight = height;

    for (uint32_t i = 1; i < mipLevels; i++) {
        // Transiciona o nível anterior (i-1) de DST para SRC (Pronto para ser lido)
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr, 0, nullptr, 1, &barrier);

        // Define a operação de redimensionamento (Blit)
        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;

        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        // Executa a redução na GPU
        vkCmdBlitImage(cmd,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, VK_FILTER_LINEAR);

        // Transiciona o nível anterior (i-1) para seu estado final de leitura pelos shaders
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // Transiciona o último nível (Mip final) que sobrou pendente em TRANSFER_DST
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);

    device.endSingleTimeCommands(cmd);
}

} // namespace app