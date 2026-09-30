#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

#include "Config.h"
#include "Scene.h"
#include "Camera.h"
#include "Curve.h"
#include "ModelInstance.h"

enum HitType { HIT_NONE, HIT_POINT, HIT_SPHERE, HIT_SUN };

struct HitResult {
    HitType type = HIT_NONE;
    float t = Config::INF;
    int pointIndex = -1;
    Curve* curve = nullptr;
    ModelInstance* sphere = nullptr;
    glm::vec3 center = glm::vec3(0.f);
};


class Interaction {
public:
    int heldPointIndex = -1;
    Curve* heldPointCurve = nullptr;
    ModelInstance* heldSphere = nullptr;
    bool indirectGrab = false;

    Interaction(Scene* scene, Camera* camera, const std::vector<Curve*>& curves, 
        ModelInstance* sun, ModelInstance* shadow);

    bool Grab(bool isIndirect);
    void Drop();

    glm::vec3 Update(float dt, glm::vec3 sunDir, bool canProcessMouse);

    void AdjustHoldDistance(double yoffset, float zoomStrength);

    // Input handling helpers
    void ToggleMode(GLFWwindow* window);
    void DeletePoint(Curve* personalCurve);
    void OnLeftClick(int action);
    void OnScroll(double yoffset);

    // Getters for app state
    bool IsHolding() const;
    bool IsHoldingCelestial() const;
    glm::vec3 GetHeldObjectPos() const;
    Curve* GetHighlightedCurve() const { return highlightedCurve; }
    float GetHoldDistance() const { return holdDistance; }

private:
    Scene* scene;
    Camera* camera;
    const std::vector<Curve*>& curves{};
    ModelInstance* sun, *shadow;

    Curve* highlightedCurve = nullptr;
    float holdDistance = Config::INF;
    float targetHoldDistance = Config::INF;

    HitResult RaycastScene(glm::vec3 origin, glm::vec3 dir);
    bool RaySphereIntersect(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 center, float radius, float& t);
};