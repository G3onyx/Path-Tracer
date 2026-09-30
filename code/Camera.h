#pragma once

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#include "Config.h"

class Camera {
public:
    glm::vec3 Position = glm::normalize(Config::INITIAL_CAMERA_POS);
    glm::vec3 Front = glm::vec3(0.f, 0.f, -1.f);
    glm::vec3 Up, Right;

    const glm::vec3 WorldUp = glm::vec3(0.f, 1.f, 0.f);

    const float FOV = glm::radians(70.f);
    const float Scale = std::tan(FOV * 0.5f);
    float AspectRatio;

    float Yaw = -90.f;
    float Pitch = 0.f;

    const float MovementSpeed = 3.f;
    const float MouseSensitivity = .1f;
    const float ZoomStrength = .3f;

    bool needsUpdate = true;
    bool mouseCaptured = true;
    bool panning = false;
    bool firstMouse = true;
    double lastX, lastY;

    Camera(GLFWwindow* window);
    
    glm::mat4 GetPV() const;

    void LookAt(glm::vec3 target);
    void TeleportTo(glm::vec3 target);

    void Process(GLFWwindow* w, float dt);

    void updateAspectRatio(int w, int h);
    void updateCameraVectors();

private:
    glm::mat4 Perspective;
    const float ZNear = .01f;
    const float ZFar = 10000.f;
};