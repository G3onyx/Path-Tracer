#include "Interaction.h"

Interaction::Interaction(Scene* scene, Camera* camera, const std::vector<Curve*>& curves,
    ModelInstance* sun, ModelInstance* shadow)
    : scene(scene), camera(camera), curves(curves), sun(sun), shadow(shadow) {
}

bool Interaction::IsHolding() const {
    return heldPointIndex != -1 || heldSphere;
}

bool Interaction::IsHoldingCelestial() const {
    return heldSphere == sun || heldSphere == shadow;
}

glm::vec3 Interaction::GetHeldObjectPos() const {
    if (heldPointIndex != -1 && heldPointCurve)
        return heldPointCurve->controlPoints[heldPointIndex];
    if (heldSphere) return heldSphere->position;
    return glm::vec3(0.f);
}

bool Interaction::RaySphereIntersect(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 center, 
    float radius, float& t) {
    glm::vec3 oc = rayOrigin - center;
    float b = glm::dot(oc, rayDir);
    float c = glm::dot(oc, oc) - radius * radius;
    float h = b * b - c;

    if (h < 0.f) return false; // Missed the sphere

    h = std::sqrt(h);
    float t1 = -b - h;
    if (t1 > .001f) {
        t = t1;
        return true;
    }

    float t2 = -b + h;
    if (t2 > .001f) {
        t = t2;
        return true;
    }

    return false;
}

HitResult Interaction::RaycastScene(glm::vec3 origin, glm::vec3 dir) {
    HitResult result;

    // Check sphere models
    for (ModelInstance& instance : scene->instances) {
        float t;
        if (instance.model->isSphere && RaySphereIntersect(origin, dir, instance.position, instance.scalevs.x, t)) {
            if (t < result.t) {
                result.t = t;
                result.type = HIT_SPHERE;
                result.sphere = &instance;
                result.center = instance.position;
            }
        }
    }

    // Check curve points
    for (Curve* c : curves) {
        if (!c->editable) continue;
        for (int i = 0; i < c->controlPoints.size(); i++) {
            float t;

            float dist = glm::distance(origin, c->controlPoints[i]);
            float dynamicRadius = Config::POINT_RADIUS * std::max(1.f, dist / Config::POINT_VISIBILITY_RANGE);

            if (RaySphereIntersect(origin, dir, c->controlPoints[i], dynamicRadius, t)) {
                if (t < result.t) {
                    result.t = t;
                    result.type = HIT_POINT;
                    result.pointIndex = i;
                    result.curve = c;
                    result.center = c->controlPoints[i];
                }
            }
        }
    }

    // Check the sun/shadow!
    float t;
    if (result.t == Config::INF) {
        if (RaySphereIntersect(origin, dir, sun->position, sun->scalevs.x, t)) {
            result.type = HIT_SUN;
            result.t = t;
            result.center = sun->position;
            result.sphere = sun;
        }
        else if (RaySphereIntersect(origin, dir, shadow->position, shadow->scalevs.x, t)) {
            result.type = HIT_SUN;
            result.t = t;
            result.center = shadow->position;
            result.sphere = shadow;
        }
    }

    return result;
}


bool Interaction::Grab(bool isIndirect) {
    if (IsHolding()) return true;

    HitResult hit = RaycastScene(camera->Position, camera->Front);

    if (hit.type == HIT_SUN) {
        heldSphere = hit.sphere;
        holdDistance = targetHoldDistance = Config::SUN_DIST;
        camera->LookAt(heldSphere->position);
        return true;
    }

    if (hit.type != HIT_NONE) {
        if (hit.type == HIT_POINT) {
            heldPointIndex = hit.pointIndex;
            heldPointCurve = hit.curve;
        }
        if (hit.type == HIT_SPHERE) {
            heldSphere = hit.sphere;

            // Only detach on direct grabs (so you can orbit balls on curves)
            if (!isIndirect) {
                for (Curve* c : curves)
                    c->detach(heldSphere);
            }
        }

        holdDistance = std::max(hit.t, Config::MIN_HOLD_DISTANCE);
        targetHoldDistance = holdDistance;
        camera->LookAt(hit.center);
        return true;
    }

    return false;
}

void Interaction::Drop() {
    if (heldSphere && highlightedCurve)
        highlightedCurve->attach(heldSphere);

    heldPointIndex = -1;
    heldPointCurve = nullptr;
    heldSphere = nullptr;
    holdDistance = Config::INF;
    targetHoldDistance = Config::INF;
    indirectGrab = false;
}


void Interaction::ToggleMode(GLFWwindow* window) {
    // Force free-fly mode if holding the sun/shadow
    if (IsHoldingCelestial()) {
        if (!camera->mouseCaptured) {
            camera->mouseCaptured = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            camera->firstMouse = true;
        }
        return;
    }

    // Toggle mouse state
    camera->mouseCaptured = !camera->mouseCaptured;
    glfwSetInputMode(window, GLFW_CURSOR, camera->mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // Entering free-fly mode
    if (camera->mouseCaptured) {
        camera->firstMouse = true;
        if (indirectGrab) Drop();
        else if (IsHolding()) holdDistance = targetHoldDistance;
    }
    // Entering orbit mode (auto-grab)
    else if (!IsHolding()) {

        // Indirect grab, succeed if grabbed something other than the sun/shadow
        if (Grab(true) && !IsHoldingCelestial()) {
            indirectGrab = true;
        }
        // Orbit the origin
        else {
            if (IsHoldingCelestial()) Drop(); // release accidental sun/shadow grab

            camera->LookAt(glm::vec3(0.f));
            targetHoldDistance = std::max(glm::length(camera->Position), Config::MIN_HOLD_DISTANCE);
            holdDistance = targetHoldDistance;
        }
    }
}

void Interaction::DeletePoint(Curve* personalCurve) {
    if (heldPointCurve == personalCurve) {
        personalCurve->removePoint(heldPointIndex, camera);
        Drop();
    }
}

void Interaction::OnLeftClick(int action) {
    // Left clicking
    if (action == GLFW_PRESS) {
        // Grabbing/dropping logic
        if (camera->mouseCaptured) {
            if (IsHolding()) Drop();
            else {
                if (Grab(false)) indirectGrab = false;
            }
        }
        // Start panning
        else if (!IsHoldingCelestial()) {
            camera->panning = true;
            targetHoldDistance = std::max(glm::distance(camera->Position, GetHeldObjectPos()), Config::MIN_HOLD_DISTANCE);
        }
    }
    // Stop panning
    else if (action == GLFW_RELEASE && !camera->mouseCaptured)
        camera->panning = false;
}

void Interaction::OnScroll(double yoffset) {
    if (IsHoldingCelestial()) return;

    if (camera->mouseCaptured) {

        // Pull/push selected sphere away from camera
        if (IsHolding()) {
            targetHoldDistance += (float)yoffset * targetHoldDistance * camera->ZoomStrength;
            targetHoldDistance = std::max(targetHoldDistance, Config::MIN_HOLD_DISTANCE); // don't pull through your face
        }

        // Teleport
        else {
            float dir = yoffset > 0 ? 1.f : -1.f;
            camera->Position += dir * camera->Front * Config::DASH_DISTANCE;
            camera->needsUpdate = true;
        }
    }
    // Pull/push camera away from orbit origin
    else {
        targetHoldDistance -= (float)yoffset * targetHoldDistance * camera->ZoomStrength;
        targetHoldDistance = std::max(targetHoldDistance, Config::MIN_HOLD_DISTANCE);
    }
}


void Interaction::AdjustHoldDistance(double yoffset, float zoomStrength) {
    targetHoldDistance += (float)yoffset * targetHoldDistance * zoomStrength;
    targetHoldDistance = std::max(targetHoldDistance, Config::MIN_HOLD_DISTANCE);
}


glm::vec3 Interaction::Update(float dt, glm::vec3 sunDir, bool canProcessMouse) {
    // Smooth hold distance
    if (holdDistance != Config::INF && !IsHoldingCelestial())
        holdDistance = glm::mix(holdDistance, targetHoldDistance, 1.f - glm::exp(-10.f * dt));

    if (!camera->mouseCaptured && !IsHoldingCelestial()) {
        glm::vec3 newPos = GetHeldObjectPos() - camera->Front * GetHoldDistance();
        if (glm::distance(camera->Position, newPos) > 0.0001f) {
            camera->Position = newPos;
            camera->needsUpdate = true;
        }
    }

    highlightedCurve = nullptr;

    if (canProcessMouse) {
        if (IsHoldingCelestial()) {
            sunDir = (heldSphere == sun) ? camera->Front : -camera->Front;
            camera->needsUpdate = true;
        }
        else if (IsHolding()) {
            glm::vec3 targetPos = camera->Position + camera->Front * holdDistance;
            glm::vec3 diff = targetPos - GetHeldObjectPos();

            // Move held object (sphere or point)
            if (glm::length(diff) > 0.001f) {
                if (heldSphere)
                    heldSphere->translate(diff);
                else if (heldPointIndex != -1 && heldPointCurve)
                    heldPointCurve->setPoint(heldPointIndex, targetPos, camera);
            }

            // Curve snapping highlight (sphere only)
            if (heldSphere) {
                glm::vec3 objPos = GetHeldObjectPos();
                float minDistSq = Config::CURVE_SNAP_DISTANCE_SQ;
                for (Curve* c : curves) {
                    glm::vec3 snapDiff = objPos - c->getPositionAtT(c->getClosestT(objPos));
                    float distSq = glm::dot(snapDiff, snapDiff);
                    if (distSq < minDistSq) {
                        minDistSq = distSq;
                        highlightedCurve = c;
                    }
                }
            }
        }
    }

    return sunDir;
}
