#pragma once

#include <GL/gl3w.h>
#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min, max;
};

static AABB calculateWorldAABB(AABB localBox, glm::mat4 transform) {
    glm::vec3 corners[8] = {
        localBox.min,
        {localBox.max.x, localBox.min.y, localBox.min.z},
        {localBox.min.x, localBox.max.y, localBox.min.z},
        {localBox.min.x, localBox.min.y, localBox.max.z},
        {localBox.max.x, localBox.max.y, localBox.min.z},
        {localBox.max.x, localBox.min.y, localBox.max.z},
        {localBox.min.x, localBox.max.y, localBox.max.z},
        localBox.max
    };

    glm::vec3 worldMin(FLT_MAX);
    glm::vec3 worldMax(-FLT_MAX);

    for (int i = 0; i < 8; i++) {
        glm::vec3 worldCorner = glm::vec3(transform * glm::vec4(corners[i], 1.f));
        worldMin = glm::min(worldMin, worldCorner);
        worldMax = glm::max(worldMax, worldCorner);
    }
    return { worldMin, worldMax };
}

struct Triangle {
    glm::vec3 a{}, b{}, c{};
    int materialID = 0;
};