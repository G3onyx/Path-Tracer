#pragma once
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <format>

#include "Config.h"
#include "Scene.h"
#include "Camera.h"
#include "Curve.h"
#include "Shader.h"
#include "Texture.h"

class Renderer {
public:
    Renderer(int width, int height, Scene* scene, Camera* camera, std::vector<Curve*>& curves);
    ~Renderer();

    // The main render pipeline
    void RenderFrame(bool sceneChanged, float dt, bool seeBVH, glm::vec3 sunDir, Curve* highlightedCurve, bool& needsSave);

    void Resize(int width, int height);

    void WaitForGPU();
    void InsertFence();

    bool SaveScreenshot(const std::string& filename) const;
    
    float getRenderScale() const { return q.renderScale; }
    unsigned int getMaxBounces() const { return q.maxBounces; }
    int getAccumCount() const { return accumCount; }
    float getRenderTimer() const { return renderTimer; }

private:
    // Setup
    void InitScreenQuad();
    void InitCurveCircle();
    void InitCurveCross();
    void LoadResources();

    // Dynamic quality
    void DynamicQuality(bool isMoving, float dt);

    static constexpr float MIN_RENDER_SCALE = 0.1f; // Never below 10% monitor resolution
    static constexpr float MAX_RENDER_SCALE = 1.f;
    static constexpr float MOVING_DELTA_LIMIT = 1.f / 20.f; // Never below 20 FPS
    static constexpr float STATIC_DELTA_LIMIT = 1.f / 2.f; // Never below 2 FPS
    static constexpr unsigned int MOVING_MIN_BOUNCES = 3u;
    static constexpr unsigned int STATIC_MIN_BOUNCES = 6u;
    static constexpr unsigned int MAX_BOUNCES = 30u;

    struct Quality {
        float renderScale = MIN_RENDER_SCALE;
        bool useCheckerboard = true;
        unsigned int maxBounces = MOVING_MIN_BOUNCES;
    } q, mq;

    // Main components
    glm::vec2 resolution;
    Scene* scene;
    Camera* camera;
    const std::vector<Curve*>& curves;

    enum class RenderState {
        MOVING,
        RAMPING,
        SETTLED
    };
    RenderState state = RenderState::MOVING;

    // OpenGL State
    GLuint accumTextures[2];
    GLuint noisyTexture;
    int writeIndex = 0;

    GLuint quadVAO = 0, quadVBO = 0;
    GLuint pointVAO = 0, pointVBO = 0;
    int circleVertexNum = 0;
    GLuint crossVAO = 0, crossVBO = 0;
    int crossVertexNum = 0;

    GLsync fence = nullptr;

    // TAA and frame State
    int frameCount = 1, accumCount = 1;
    glm::mat4 prevPV = glm::mat4(1.f);
    glm::vec2 prevRenderScale = glm::vec2(1.f);
    float stoppedTimer = 0.f;
    float renderTimer = 0.f;

    // Resources
    Shader* shader;
    Shader* taaShader;
    Shader* displayShader;
    Shader* bezierShader;

    Texture* blueNoise;
    Texture* marbleAlbedo;
    Texture* marbleRoughness;
    Texture* skybox;
};