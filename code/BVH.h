#pragma once

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include <vector>
#include <algorithm>

#include "GPUStructs.h"
#include "ModelInstance.h"
#include "AABB.h"

struct BVHNode {
    AABB bounds;
    int start = 0, count = 0; // section of primitives this node contains
    int numNodes = 1; // #nodes beneath this one (including itself)

    int childCount = 0; // 0 for leaf, 1-4 for internal
    BVHNode* children[4] = { nullptr, nullptr, nullptr, nullptr };

    bool isLeaf() const { return childCount == 0; }
};


// For Triangles (BLAS)
inline AABB getBounds(const Triangle& t) {
    return { glm::min(t.a, glm::min(t.b, t.c)), glm::max(t.a, glm::max(t.b, t.c)) };
}
inline glm::vec3 getCentroid(const Triangle& t) {
    return (t.a + t.b + t.c) / 3.f;
}
inline float getLeafOffset(const std::vector<Triangle>& prims, int start) {
    return (float)start;
}

// For ModelInstances (TLAS)
inline AABB getBounds(const ModelInstance& i) {
    return i.worldBounds;
}
inline glm::vec3 getCentroid(const ModelInstance& i) {
    return (i.worldBounds.min + i.worldBounds.max) * .5f;
}
inline float getLeafOffset(const std::vector<ModelInstance>& prims, int start) {
    return (float)prims[start].gpuInstanceIndex;
}

template <typename T>
class BVH {
public:
    std::vector<T> primitives;
    BVHNode* root;

    BVH(const std::vector<T>& primitives, int maxDepth, int maxPrims) {
        this->primitives = primitives;
        this->MAX_PRIMS = std::min(maxPrims, 127); // Only have 7 bits for primitive count, so hard maximum is 2^7 - 1
        this->root = build(0, (int)primitives.size(), maxDepth); // Build BVH immediately
    }

    ~BVH() { deleteNode(root); }

    BVHNode* build(int start, int count, unsigned int depth);

    AABB makeAABB(int start, int count);

    std::vector<GPUNode> flattenNodes(int globalIndex);

    std::vector<GPUTriangle> prepareTriangles();

private:
    int MAX_PRIMS;

    int splitMTLA(AABB bounds, int start, int count);

    int splitSAH(AABB bounds, int start, int count);

    int flatten(BVHNode* node, std::vector<GPUNode>& flatNodes, int indexOffset);

    void collapseToQuad(BVHNode* node);

    void deleteNode(BVHNode* node) {
        if (!node) return;
        for (int i = 0; i < node->childCount; ++i) {
            deleteNode(node->children[i]);
        }
        delete node;
    }
};