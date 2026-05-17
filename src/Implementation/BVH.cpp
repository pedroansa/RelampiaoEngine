#include "BVH.h"
#include <algorithm>
#include <cmath>

namespace app {

    void BVH::build(std::vector<BVHTri>& inputTris) {
        tris = &inputTris;
        triIndices.resize(tris->size());
        for (int i = 0; i < triIndices.size(); i++) triIndices[i] = i;

        // Precompute bounds and centroids
        for (auto& tri : *tris) {
            tri.bounds = AABB{};
            tri.bounds.expand(tri.v0);
            tri.bounds.expand(tri.v1);
            tri.bounds.expand(tri.v2);
            tri.centroid = tri.bounds.centroid();
        }

        nodes.clear();
        nodes.reserve(tris->size() * 2);
        buildRecursive(0, static_cast<int>(tris->size()));
    }

    int BVH::buildRecursive(int start, int end) {
        int nodeIdx = static_cast<int>(nodes.size());
        nodes.emplace_back();
        BVHNode& node = nodes[nodeIdx];

        // Compute bounds for all tris in range
        for (int i = start; i < end; i++)
            node.bounds.expand((*tris)[triIndices[i]].bounds);

        int count = end - start;

        // Leaf if few enough triangles
        if (count <= 4) {
            node.triStart = start;
            node.triCount = count;
            return nodeIdx;
        }

        // Split along longest axis at centroid midpoint
        glm::vec3 extent = node.bounds.max - node.bounds.min;
        int axis = 0;
        if (extent.y > extent.x) axis = 1;
        if (extent.z > extent[axis]) axis = 2;

        float mid = node.bounds.centroid()[axis];

        // Partition triangles around midpoint
        auto it = std::partition(triIndices.begin() + start, triIndices.begin() + end,
            [&](int idx) { return (*tris)[idx].centroid[axis] < mid; });

        int splitIdx = static_cast<int>(it - triIndices.begin());

        // Fallback: split in half if partition degenerate
        if (splitIdx == start || splitIdx == end)
            splitIdx = (start + end) / 2;

        // Build children — must re-fetch node ref after recursive calls (vector may reallocate)
        int left = buildRecursive(start, splitIdx);
        int right = buildRecursive(splitIdx, end);
        nodes[nodeIdx].leftChild = left;
        nodes[nodeIdx].rightChild = right;

        return nodeIdx;
    }

    BVH::HitResult BVH::intersect(const glm::vec3& origin, const glm::vec3& direction) const {
        HitResult result;
        glm::vec3 invDir = 1.f / direction;

        // Iterative traversal with stack
        int stack[64];
        int stackPtr = 0;
        stack[stackPtr++] = 0; // root

        while (stackPtr > 0) {
            int idx = stack[--stackPtr];
            const BVHNode& node = nodes[idx];

            float tMin;
            if (!node.bounds.intersect(origin, invDir, tMin)) continue;
            if (tMin > result.t) continue; // already found closer hit

            if (node.isLeaf()) {
                // Test all triangles in leaf
                for (int i = node.triStart; i < node.triStart + node.triCount; i++) {
                    const BVHTri& tri = (*tris)[triIndices[i]];

                    // Moller-Trumbore
                    const float EPSILON = 1e-7f;
                    glm::vec3 edge1 = tri.v1 - tri.v0;
                    glm::vec3 edge2 = tri.v2 - tri.v0;
                    glm::vec3 h = glm::cross(direction, edge2);
                    float a = glm::dot(edge1, h);
                    if (std::abs(a) < EPSILON) continue;

                    float f = 1.f / a;
                    glm::vec3 s = origin - tri.v0;
                    float u = f * glm::dot(s, h);
                    if (u < 0.f || u > 1.f) continue;

                    glm::vec3 q = glm::cross(s, edge1);
                    float v = f * glm::dot(direction, q);
                    if (v < 0.f || u + v > 1.f) continue;

                    float t = f * glm::dot(edge2, q);
                    if (t > EPSILON && t < result.t) {
                        result.hit = true;
                        result.t = t;
                        result.u = u;
                        result.v = v;
                        result.triIndex = triIndices[i];
                    }
                }
            }
            else {
                stack[stackPtr++] = node.leftChild;
                stack[stackPtr++] = node.rightChild;
            }
        }

        return result;
    }

} // namespace app