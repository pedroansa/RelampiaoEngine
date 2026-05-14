#pragma once
#include "AppTexture.h"
#include <array>

namespace app {

    class CubemapTexture : public Texture {
    public:
        // faces order: +X, -X, +Y, -Y, +Z, -Z
        CubemapTexture(EngineDevice& device, const std::array<std::string, 6>& facePaths);

        CubemapTexture(const CubemapTexture&) = delete;
        CubemapTexture& operator=(const CubemapTexture&) = delete;

    private:
        void createCubemapImage(const std::array<std::string, 6>& facePaths);
        void createImageView();
        void createSampler();
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
    };

} // namespace app