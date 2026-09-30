#include "Camera.h"

Camera::Camera(GLFWwindow* w) {
    int width, height;
    glfwGetFramebufferSize(w, &width, &height);

    lastX = (double)width / 2.0;
    lastY = (double)height / 2.0;

    // Make the window capture the user's cursor
    glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);   

    updateAspectRatio(width, height);
    updateCameraVectors();
}

// Update aspect ratio and recalcuate perspective matrix
void Camera::updateAspectRatio(int w, int h) {
    AspectRatio = (float)w / (h > 0 ? (float)h : 1.f);
    Perspective = glm::perspective(FOV, AspectRatio, ZNear, ZFar);
}

// Calculate view matrix and return it with perspective
glm::mat4 Camera::GetPV() const {
    return Perspective * glm::lookAt(Position, Position + Front, Up);
}

void Camera::LookAt(glm::vec3 target) {
    glm::vec3 diff = target - Position;
    if (glm::dot(diff, diff) < 1e-5) return; // don't look at something you're inside of

    glm::vec3 direction = glm::normalize(diff);

    Yaw = glm::degrees(atan2(direction.z, direction.x));
    Pitch = glm::degrees(asin(direction.y));
    Pitch = glm::clamp(Pitch, -89.f, 89.f);

    updateCameraVectors();
}

void Camera::TeleportTo(glm::vec3 target) {
    Position = target;
    needsUpdate = true;
}

void Camera::updateCameraVectors() {
    glm::vec3 f{};
    f.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    f.y = sin(glm::radians(Pitch));
    f.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(f);

    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));

    needsUpdate = true;
}

void Camera::Process(GLFWwindow* w, float dt) {
    if (!mouseCaptured) return;

    // Minecraft free-flying controls
    glm::vec3 move(0.f);
    glm::vec3 flatFront = glm::normalize(glm::vec3(Front.x, 0.f, Front.z));

    if (glfwGetKey(w, GLFW_KEY_W) || glfwGetKey(w, GLFW_KEY_UP)) move += flatFront;
    if (glfwGetKey(w, GLFW_KEY_S) || glfwGetKey(w, GLFW_KEY_DOWN)) move -= flatFront;
    if (glfwGetKey(w, GLFW_KEY_A) || glfwGetKey(w, GLFW_KEY_LEFT)) move -= Right;
    if (glfwGetKey(w, GLFW_KEY_D) || glfwGetKey(w, GLFW_KEY_RIGHT)) move += Right;
    if (glfwGetKey(w, GLFW_KEY_SPACE) || glfwGetKey(w, GLFW_KEY_PAGE_UP)) move += WorldUp;
    if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(w, GLFW_KEY_PAGE_DOWN)) move -= WorldUp;

    if (glm::length(move) > 0.f) {
        Position += glm::normalize(move) * MovementSpeed * dt;;
        needsUpdate = true;
    }
}