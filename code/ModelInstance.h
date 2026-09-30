#pragma once

#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Model.h"
#include "AABB.h"

class ModelInstance {
public:
    Model* model; // in model space

    AABB worldBounds;
    unsigned int gpuInstanceIndex; // index into gpuInstances

    bool ignoresMovement = false;

    glm::mat4 transform = glm::mat4(1.f); // in world space
    glm::vec3 position = glm::vec3(0.f);
    glm::vec3 rotation = glm::vec3(0.f);
    glm::vec3 scalevs = glm::vec3(1.f);
    bool needsUpdate = true;

    ModelInstance(Model* model, unsigned int gpuInstanceIndex);

    void translate(glm::vec3 amount);

    void rotate(glm::vec3 angles);

    void scale(float extent);

    void scale(glm::vec3 extents);

    glm::mat4 getTransform();
};