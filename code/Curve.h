#pragma once

#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <vector>
#include <initializer_list>

#include "Config.h"
#include "Shader.h"
#include "Camera.h"
#include "ModelInstance.h"


struct AttachedSphere {
    ModelInstance* instance;
    float t; // Progress (0-1)
    int direction = 1; // 1 (forward) or -1 (backward)
};

class Curve {
public:
    std::vector<glm::vec3> controlPoints{}; // Catmull–Rom control points
    int vNum = 0;
    GLuint vao = 0, vbo = 0;

    bool closed = false;
    bool editable = false;

    std::vector<glm::vec3> cookedVertices{}; // Points that define the curve (cached for drawing)
    std::vector<float> arcLengths{}; // Distances between each pair of points (cached for searching)
    float totalLength = 0.f;

    std::vector<AttachedSphere> attachedSpheres{};

    float moveSpeed = 10.f; // Distance a sphere moves along the curve per second

    const int pointsPerSegment = 20;

    Curve();
    Curve(std::initializer_list<glm::vec3> data, Camera* camera, bool closed, bool editable, float speed);
    ~Curve();

    // Curve creation
    void addPoint(Camera* camera, float minHoldDistance);
    void removePoint(int index, Camera* camera);
    void toggleClosed(Camera* camera);
    void setPoint(int index, glm::vec3 p, Camera* camera);

    // Sphere movement
    void attach(ModelInstance* sphere);
    void detach(ModelInstance* sphere);
    void forceAttach(ModelInstance* sphere, float startT = 0.f);
    bool animateSpheres(float dt); // returns whether anything moved

    glm::vec3 getPositionAtT(float t);
    float getClosestT(glm::vec3 pos);
    float getDistanceAtT(float t);
    float getTFromDistance(float dist);


    void update(Camera* camera);
    void draw(Shader& shader, Camera* camera, glm::vec2 renderScale, glm::vec2 resolution, GLuint accumTexture, 
        bool isHighlighted, GLuint pointVAO, int circleVertexNum, GLuint crossVAO, int crossVertexNum);
};