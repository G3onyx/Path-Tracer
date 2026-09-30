#pragma once
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <chrono>
#include <format>

#include "Scene.h"
#include "Camera.h"
#include "Renderer.h"
#include "Interaction.h"
#include "Curve.h"
#include "ModelInstance.h"

class App {
public:
    App();
    ~App();

    void Run();

private:
    bool InitWindow();
    void SetupScene();
    void UpdateWorld(float dt);
    void HandleResize();

    // Main systems
    GLFWwindow* window;
    Scene* scene;
    Camera* camera;
    Renderer* renderer;
    Interaction* interaction;
    std::vector<Model*> loadedModels{};

    // App state
    bool frozen = false;
    bool needsSave = false;
    bool seeBVH = false;
    bool windowResized = false;
    glm::vec2 resolution = glm::vec2(Config::SCREEN_W, Config::SCREEN_H);
    glm::vec3 sunDir = glm::normalize(Config::INITIAL_SUN_DIR);

    // Scene state
    std::vector<Curve*> curves{};
    Curve* personalCurve;
    ModelInstance* sun;
    ModelInstance* shadow;

    // GLFW callbacks
    static void KeyCallback(GLFWwindow* w, int k, int s, int a, int m);
    static void MouseCallback(GLFWwindow* w, double x, double y);
    static void MouseButtonCallback(GLFWwindow* w, int b, int a, int m);
    static void ScrollCallback(GLFWwindow* w, double xoffset, double yoffset);
    static void ResizeCallback(GLFWwindow* w, int width, int height);
    static void WindowFocusCallback(GLFWwindow* w, int focused);
    static void DebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

    // Event handlers
    void OnKey(int key, int action);
    void OnMousePosition(double x, double y);
    void OnResize(int width, int height);
    void OnFocus(int focused);
};