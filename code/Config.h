#pragma once
#define NOMINMAX
#include <glm/glm.hpp>
#include <cmath>

namespace Config {
    // Initial window size
    constexpr unsigned int SCREEN_W = 900;
    constexpr unsigned int SCREEN_H = 900;

    // Materials
    enum Material {
        LIGHT,
        GLASS,
        MIRROR,
        RED,
        GREEN,
        BLUE,
        WHITE,
        SEAGLASS
    };

    // Sun visuals
    constexpr float SUN_DIST = 3.f;
    inline const float SUN_ANGLE = 1.f / SUN_DIST;
    inline const float SUN_THRESHOLD = std::sqrt(1.f - (SUN_ANGLE * SUN_ANGLE));

    // Bezier curves
    constexpr float POINT_RADIUS = 0.5f;
    constexpr float POINT_VISIBILITY_RANGE = 30.f;
    constexpr float CURVE_SNAP_DISTANCE_SQ = 2.f;

    // Scene settings
    const glm::vec3 INITIAL_CAMERA_POS = glm::vec3(0.f, 0.f, 7.5f);
    const glm::vec3 INITIAL_SUN_DIR = glm::vec3(0.f, 0.5f, -1.f);

    constexpr float CORNELL_WIDTH = 8.f; // DO NOT CHANGE
    constexpr float CORNELL_HEIGHT = 6.f; // DO NOT CHANGE
    constexpr float CORNELL_DEPTH = 16.f; // DO NOT CHANGE

    constexpr int FRACTAL_DEPTH = 5; // DO NOT INCREASE PAST 6
    constexpr float FRACTAL_SIDE_LENGTH = 300.f; // DO NOT CHANGE
    constexpr int FRACTAL_MATERIAL = RED;

    constexpr int OBJ_MODEL_MATERIAL = GREEN;

    // Misc.
    constexpr float MIN_HOLD_DISTANCE = 2.f;
    constexpr float DASH_DISTANCE = 5.f;
    constexpr float STOPPED_DELAY = 1.f;
    constexpr float INF = 1e30f;

    // BVH settings
    constexpr int SCENE_MAX_NODES = 100000;

    constexpr int BLAS_MAX_DEPTH = 10;
    constexpr int BLAS_MIN_PRIMS = 3;

    constexpr int TLAS_MAX_DEPTH = 16;
    constexpr int TLAS_MIN_PRIMS = 1;
}