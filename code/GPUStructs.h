#pragma once

#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include <vector>


struct alignas(16) GPUNode {
    glm::vec3 min;
    uint32_t qminx;

    glm::vec3 scaledDims;
    uint32_t qmaxx;

    uint32_t qminy;
    uint32_t qmaxy;
    uint32_t qminz;
    uint32_t qmaxz;

    // If Leaf: Bits 0-23 = primitive offset, Bits 24-30 = primitive count
    // If Internal: Bits 0-30 = child node index in GPUNodes array
    // Bit 31: isLeaf flag
    uint32_t childData[4];
};

struct alignas(16) GPUTriangle {
    glm::vec4 a; // xyz=p0, w=?
    glm::vec4 b; // xyz=p1, w=?
    glm::vec4 c; // xyz=p2, w=matID
};

struct alignas(16) GPUInstance {
    glm::mat4 modelMatrix;
    glm::mat4 invModelMatrix;
    int blasRoot; // Index in globalNodes where the model's BVH starts (materialID if sphere)
    int meshOffset; // Index in globalTris where the model's mesh data starts (-1 if sphere)
    float padding[2];
};

struct alignas(16) GPUMaterial {
    // xyz: albedo, w: roughness
    glm::vec4 albedo_rough = glm::vec4(1.f);
    // xyz: transmissionTint, w: metallic
    glm::vec4 tint_metal = glm::vec4(1.f, 1.f, 1.f, 0.f);
    // x: emissive, y: ior, z: type (0=PBR, 1=Glass), w: ao
    glm::vec4 params = glm::vec4(0.f, 1.f, 0.f, 1.f);
};