#pragma once
#include "AppTexture.h"
#include <memory>

namespace app {

    struct MaterialComponent {
        std::shared_ptr<Texture> albedo;
        std::shared_ptr<Texture> normalMap;
        std::shared_ptr<Texture> metallic;   // separado
        std::shared_ptr<Texture> roughness;  // separado
        std::shared_ptr<Texture> ao;
        float uvScale = 1.f;
        float metallicf = 0.f;   // fallback escalar
        float roughnessf = 0.5f;  // fallback escalar
    };

} // namespace app