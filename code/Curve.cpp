#include "Curve.h"

Curve::Curve() {
    editable = true;
}

Curve::Curve(std::initializer_list<glm::vec3> data, Camera* camera, bool closed, bool editable, float speed) {
    controlPoints = data;
    this->closed = closed;
    this->editable = editable;
    moveSpeed = speed;
    update(camera);
}

Curve::~Curve() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
}

void Curve::addPoint(Camera* camera, float minHoldDistance) {
    controlPoints.push_back(camera->Position + minHoldDistance * camera->Front);
    update(camera);
}

void Curve::removePoint(int index, Camera* camera) {
    if (index >= 0 && index < controlPoints.size()) {
        controlPoints.erase(controlPoints.begin() + index);
        update(camera);
    }
}

void Curve::toggleClosed(Camera* camera) {
    closed = !closed;
    update(camera);
}

// Set the position of an existing curve point
void Curve::setPoint(int index, glm::vec3 p, Camera* camera) {
    if (index >= 0 && index < controlPoints.size()) {
        controlPoints[index] = p;
        update(camera);
    }
}

void Curve::update(Camera* camera) {
    if (controlPoints.size() < 2) {
        // Detach spheres if the curve is broken
        attachedSpheres.clear();
        cookedVertices.clear();
        return;
    }

    std::vector<glm::vec3> vertices;
    size_t n = controlPoints.size();

    size_t limit = closed ? n : n - 1;
    for (size_t i = 0; i < limit; ++i) {
        glm::vec3 p0, p1, p2, p3; // prev, current, next, next-next

        if (closed) {
            p0 = controlPoints[(i - 1 + n) % n];
            p1 = controlPoints[i];
            p2 = controlPoints[(i + 1) % n];
            p3 = controlPoints[(i + 2) % n];
        }
        else {
            p0 = i == 0 ? controlPoints[0] : controlPoints[i - 1];
            p1 = controlPoints[i];
            p2 = controlPoints[i + 1];
            p3 = i + 2 >= n ? controlPoints[n - 1] : controlPoints[i + 2];
        }

        // Create bezier control points
        glm::vec3 b0 = p1; // bezier starts at current
        glm::vec3 b1 = p1 + (p2 - p0) / 6.f;
        glm::vec3 b2 = p2 - (p3 - p1) / 6.f;
        glm::vec3 b3 = p2; // bezier ends at next

        std::vector<glm::vec3> segmentPoints = { b0, b1, b2, b3 };

        // Calculate points on curve segment (using De Casteljau's algorithm)
        for (int j = 0; j < pointsPerSegment; ++j) {
            float t = (float)j / (float)pointsPerSegment;

            std::vector<glm::vec3> temp = segmentPoints;
            for (int k = 1; k < temp.size(); ++k) {
                for (size_t m = 0; m < temp.size() - k; ++m) {
                    temp[m] = glm::mix(temp[m], temp[m + 1], t);
                }
            }
            vertices.push_back(temp[0]);
        }
    }

    if (!closed) vertices.push_back(controlPoints.back());
    else if (vertices.size() > 0) vertices.push_back(vertices[0]);

    cookedVertices = vertices;

    // Build arc-length table (distances between each pair of points)
    arcLengths.clear();
    arcLengths.push_back(0.f);
    totalLength = 0.f;
    for (size_t i = 1; i < cookedVertices.size(); ++i) {
        totalLength += glm::distance(cookedVertices[i], cookedVertices[i - 1]);
        arcLengths.push_back(totalLength);
    }

    // Ensure attached spheres stay attached
    for (AttachedSphere& as : attachedSpheres)
        as.t = getClosestT(as.instance->position);
    
    // On first update, generate the buffers
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }

    // Update data on GPU
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    vNum = (int)vertices.size();
    glBufferData(GL_ARRAY_BUFFER, vNum * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // Flag camera update
    if (camera) camera->needsUpdate = true;
}

void Curve::draw(Shader& shader, Camera* camera, glm::vec2 renderScale, glm::vec2 resolution, GLuint accumTexture, 
    bool isHighlighted, GLuint pointVAO, int circleVertexNum, GLuint crossVAO, int crossVertexNum) {
    if (controlPoints.empty()) return;

    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    shader.use();
    shader.setMat4("PV", camera->GetPV());
    shader.setMat4("model", glm::mat4(1.0f));

    shader.setVec3("camPos", camera->Position);
    shader.setVec2("resolution", resolution);
    shader.setVec2("renderScale", renderScale);
    shader.setBool("isHighlighted", isHighlighted);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, accumTexture);
    shader.setInt("accumTexture", 0);

    shader.setBool("isEndPoint", false);

    // Only draw the curve if there are enough points
    if (controlPoints.size() >= 2) {
        shader.setMat4("model", glm::mat4(1.0f));
        glBindVertexArray(vao);
        glDrawArrays(GL_LINE_STRIP, 0, vNum);
    }

    glBindVertexArray(editable ? pointVAO : crossVAO);
    int currentVNum = editable ? circleVertexNum : crossVertexNum;

    // Draw the points
    for (int i = 0; i < controlPoints.size(); i++) {
        shader.setBool("isEndPoint", editable && i == controlPoints.size() - 1);

        float dist = glm::distance(camera->Position, controlPoints[i]);
        float dynamicRadius = Config::POINT_RADIUS * glm::max(1.f, dist / Config::POINT_VISIBILITY_RANGE);

        // Make points face the camera (billboarding)
        glm::mat4 model = glm::mat4(1.0f);
        model[0] = glm::vec4(camera->Right * dynamicRadius, 0.f);
        model[1] = glm::vec4(camera->Up * dynamicRadius, 0.f);
        model[2] = glm::vec4(-camera->Front * dynamicRadius, 0.f);
        model[3] = glm::vec4(controlPoints[i], 1.f);

        shader.setMat4("model", model);
        glDrawArrays(GL_LINES, 0, currentVNum);
    }

    glDisable(GL_DEPTH_TEST);
}

// Get the real position of the given t position on the curve
glm::vec3 Curve::getPositionAtT(float t) {
    if (cookedVertices.empty()) return glm::vec3(0.f);
    if (t <= 0.f) return cookedVertices.front();
    if (t >= 1.f) return cookedVertices.back();

    float exactI = t * (cookedVertices.size() - 1);
    int i1 = (int)exactI;
    int i2 = std::min(i1 + 1, (int)cookedVertices.size() - 1);
    float fract = exactI - (float)i1;

    return glm::mix(cookedVertices[i1], cookedVertices[i2], fract);
}

// Get the t position (on the curve) closest to the given real position
float Curve::getClosestT(glm::vec3 pos) {
    if (cookedVertices.empty()) return 0.f;

    float closestDistSq = 1e30f;
    float closestT = 0.f;

    for (int i = 0; i < cookedVertices.size(); i++) {
        glm::vec3 diff = pos - cookedVertices[i];
        float distSq = glm::dot(diff, diff);
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closestT = (float)i / (cookedVertices.size() - 1);
        }
    }
    return closestT;
}

// Attach a sphere to the nearest point on the curve
void Curve::attach(ModelInstance* sphere) {
    AttachedSphere as{};
    as.instance = sphere;
    as.t = getClosestT(sphere->position);
    as.direction = 1;
    attachedSpheres.push_back(as);
}

// Detach a sphere from the curve
void Curve::detach(ModelInstance* sphere) {
    attachedSpheres.erase(
        std::remove_if(attachedSpheres.begin(), attachedSpheres.end(),
            [sphere](const AttachedSphere& as) { return as.instance == sphere; }),
        attachedSpheres.end());
}

// Attach a sphere to a specific point on the curve
void Curve::forceAttach(ModelInstance* sphere, float startT) {
    if (cookedVertices.empty()) return;

    AttachedSphere as{};
    as.instance = sphere;
    as.t = glm::clamp(startT, 0.f, 1.f);
    as.direction = 1;
    attachedSpheres.push_back(as);

    glm::vec3 targetPos = getPositionAtT(as.t);
    glm::vec3 diff = targetPos - sphere->position;
    sphere->translate(diff);
}

bool Curve::animateSpheres(float dt) {
    if (attachedSpheres.empty() || cookedVertices.empty() || totalLength <= 0.f) return false;

    float realDist = moveSpeed * dt; // how far spheres moved this frame

    for (AttachedSphere& as : attachedSpheres) {

        // Move the sphere a real (not t) amount along the curve
        float currentDist = getDistanceAtT(as.t);
        currentDist += as.direction * realDist;

        if (closed) {
            // Wrap around if curve is closed
            if (currentDist > totalLength) currentDist -= totalLength;
            if (currentDist < 0.f) currentDist += totalLength;
        }
        else {
            // Bounce off the ends if curve is open
            if (currentDist > totalLength) {
                currentDist = totalLength;
                as.direction = -1;
            }
            else if (currentDist < 0.f) {
                currentDist = 0.f;
                as.direction = 1;
            }
        }

        // Update the sphere's t distance from real distance
        as.t = getTFromDistance(currentDist);

        // Move the sphere to the correct position
        glm::vec3 newPos = getPositionAtT(as.t);
        glm::vec3 diff = newPos - as.instance->position;
        as.instance->translate(diff);
    }
    return true;
}

// Convert t-distance to real distance
float Curve::getDistanceAtT(float t) {
    if (arcLengths.empty()) return 0.f;
    if (t <= 0.f) return 0.f;
    if (t >= 1.f) return totalLength;

    float exactI = t * (arcLengths.size() - 1);
    int i1 = (int)exactI;
    int i2 = std::min(i1 + 1, (int)arcLengths.size() - 1);
    float fract = exactI - (float)i1;

    return glm::mix(arcLengths[i1], arcLengths[i2], fract);
}

// Convert real distance to t-distance
float Curve::getTFromDistance(float dist) {
    if (arcLengths.empty() || totalLength <= 0.f) return 0.f;
    if (dist <= 0.f) return 0.f;
    if (dist >= totalLength) return 1.f;

    for (size_t i = 0; i < arcLengths.size() - 1; ++i) {
        if (dist >= arcLengths[i] && dist <= arcLengths[i + 1]) {
            float segmentLen = arcLengths[i + 1] - arcLengths[i];
            float fract = (segmentLen > 0.0001f) ? (dist - arcLengths[i]) / segmentLen : 0.f;
            return (i + fract) / (float)(arcLengths.size() - 1);
        }
    }
    return 1.f;
}