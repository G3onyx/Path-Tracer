#include "ModelInstance.h"


ModelInstance::ModelInstance(Model* model, unsigned int gpuInstanceIndex) {
    this->model = model;
    this->gpuInstanceIndex = gpuInstanceIndex;
    worldBounds = model->bounds;
}

void ModelInstance::translate(glm::vec3 amount) {
    needsUpdate = true;
    position += amount;
}

void ModelInstance::rotate(glm::vec3 angles) {
    needsUpdate = true;
    rotation += angles;
}

void ModelInstance::scale(float extent) {
    needsUpdate = true;
    scalevs *= extent;
}

void ModelInstance::scale(glm::vec3 extents) {
    bool equal = extents.x == extents.y && extents.y == extents.z;
    if (model->isSphere && !equal) return;

    needsUpdate = true;
    scalevs *= extents;
}

glm::mat4 ModelInstance::getTransform() {
    if (needsUpdate) {
        transform = glm::translate(glm::mat4(1.0f), position);

        transform = glm::rotate(transform, rotation.x, glm::vec3(1, 0, 0));
        transform = glm::rotate(transform, rotation.y, glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, rotation.z, glm::vec3(0, 0, 1));

        transform = glm::scale(transform, scalevs);

        // Update world bounds for the TLAS
        worldBounds = calculateWorldAABB(model->bounds, transform);
    }
    return transform;
};