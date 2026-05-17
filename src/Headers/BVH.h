#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace app {

    struct AABB {
        glm::vec3 min{ 1e9f };
        glm::vec3 max{ -1e9f };

        // Expand to include point
        void expand(const glm::vec3& p) {
            min = glm::min(min, p);
            max = glm::max(max, p);
        }

        // Expand to include another AABB
        void expand(const AABB& other) {
            min = glm::min(min, other.min);
            max = glm::max(max, other.max);
        }

        glm::vec3 centroid() const { return (min + max) * 0.5f; }

        // Ray-AABB intersection — returns true and fills tMin
        bool intersect(const glm::vec3& origin, const glm::vec3& invDir, float& tMin) const {
            glm::vec3 t0 = (min - origin) * invDir;
            glm::vec3 t1 = (max - origin) * invDir;
            glm::vec3 tSmall = glm::min(t0, t1);
            glm::vec3 tLarge = glm::max(t0, t1);
            tMin = glm::max(glm::max(tSmall.x, tSmall.y), tSmall.z);
            float tMax = glm::min(glm::min(tLarge.x, tLarge.y), tLarge.z);
            return tMax >= tMin && tMax > 0.f;
        }
    };

    // BVH node — either internal (has children) or leaf (has triangles)
    struct BVHNode {
        AABB bounds;
        int leftChild = -1; // index into nodes array, -1 if leaf
        int rightChild = -1;
        int triStart = -1; // index into triIndices, if leaf
        int triCount = 0;

        bool isLeaf() const { return triCount > 0; }
    };

    // Triangle data needed by BVH
    struct BVHTri {
        glm::vec3 v0, v1, v2;
        glm::vec3 n0, n1, n2;
        glm::vec3 c0, c1, c2;
        glm::vec2 uv0, uv1, uv2;
        AABB bounds;
        glm::vec3 centroid;

        const uint8_t* texPixels = nullptr;
        int texWidth = 0;
        int texHeight = 0;
    };

    class BVH {
    public:
        // Build BVH from triangles
        void build(std::vector<BVHTri>& tris);

        struct HitResult {
            bool hit = false;
            float t = 1e9f;
            float u, v;
            int triIndex = -1;
        };

        // Traverse BVH with a ray
        HitResult intersect(const glm::vec3& origin, const glm::vec3& direction) const;

        const std::vector<BVHTri>& getTris() const { return *tris; }

    private:
        int buildRecursive(int start, int end);

        std::vector<BVHNode> nodes;
        std::vector<int>     triIndices; // sorted triangle indices
        std::vector<BVHTri>* tris = nullptr;
    };

} // namespace app