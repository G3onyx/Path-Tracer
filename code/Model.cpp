#define TINYOBJLOADER_IMPLEMENTATION
#include "Model.h"


static const std::string RES_PATH = "res\\";


Model::Model(std::string text) {
    if (text == "BALL") {
        isSphere = true;
        BuildUnitCube();
    }
    else if (text == "CUBE") {
        BuildUnitCube();
    }
    else if (text == "CORNELL") {
        BuildCornellBox();
    }
    else if (text == "FRACTAL") {
        BuildSierpinski();
    }
    else {
        LoadObj(text);
    }

    UpdateGPU();
}


void Model::UpdateGPU() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(triangles.size() * 9);
    indices.reserve(triangles.size() * 3);

    for (unsigned int i = 0; i < triangles.size(); ++i) {
        const auto& t = triangles[i];
        // Positions
        vertices.insert(vertices.end(), { t.a.x, t.a.y, t.a.z, t.b.x, t.b.y, t.b.z, t.c.x, t.c.y, t.c.z });
        // Indices
        unsigned int base = i * 3;
        indices.insert(indices.end(), { base, base + 1, base + 2 });
    }

    indexCount = static_cast<unsigned int>(indices.size());

    glGenVertexArrays(1, &vao);
    unsigned int vbo, ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}


void Model::BuildUnitCube() {
    bounds = AABB{ glm::vec3(-1.f), glm::vec3(1.f) };

    if (isSphere) return;

    glm::vec3 v[8] = {
        {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}
    };

    unsigned int indices[] = {
        4, 5, 6, 4, 6, 7,
        1, 0, 3, 1, 3, 2,
        5, 1, 2, 5, 2, 6,
        0, 4, 7, 0, 7, 3,
        7, 6, 2, 7, 2, 3,
        0, 1, 5, 0, 5, 4
    };

    for (int i = 0; i < 36; i += 3) {
        Triangle tri;
        tri.a = v[indices[i]];
        tri.b = v[indices[i + 1]];
        tri.c = v[indices[i + 2]];
        tri.materialID = 0;
        triangles.push_back(tri);
    }
}


void Model::BuildCornellBox() {

    float w = Config::CORNELL_WIDTH;
    float h = Config::CORNELL_HEIGHT;
    float d = Config::CORNELL_DEPTH;

    bounds = AABB{ glm::vec3(-w, -h, -d), glm::vec3(w, h, d) };

    auto addQuad = [&](glm::vec3 bl, glm::vec3 br, glm::vec3 tr, glm::vec3 tl, int matID) {
        Triangle t1, t2;

        t1.a = bl; t1.b = br; t1.c = tr; t1.materialID = matID;
        t2.a = bl; t2.b = tr; t2.c = tl; t2.materialID = matID;

        triangles.push_back(t1);
        triangles.push_back(t2);
    };

    // Front
    //addQuad(glm::vec3(w, -h, d), glm::vec3(-w, -h, d), glm::vec3(-w, h, d), glm::vec3(w, h, d), WHITE);
    
    // Back
    addQuad(glm::vec3(-w, -h, -d), glm::vec3(w, -h, -d), glm::vec3(w, h, -d), glm::vec3(-w, h, -d), Config::WHITE);
    // Left
    addQuad(glm::vec3(-w, -h, d), glm::vec3(-w, -h, -d), glm::vec3(-w, h, -d), glm::vec3(-w, h, d), Config::RED);
    // Right
    addQuad(glm::vec3(w, -h, -d), glm::vec3(w, -h, d), glm::vec3(w, h, d), glm::vec3(w, h, -d), Config::GREEN);
    // Floor
    addQuad(glm::vec3(-w, -h, d), glm::vec3(w, -h, d), glm::vec3(w, -h, -d), glm::vec3(-w, -h, -d), Config::WHITE);
    // Ceiling
    addQuad(glm::vec3(-w, h, -d), glm::vec3(w, h, -d), glm::vec3(w, h, d), glm::vec3(-w, h, d), Config::WHITE);
}


void Model::BuildSierpinski() {
    const int depth = Config::FRACTAL_DEPTH;
    const float sideLength = Config::FRACTAL_SIDE_LENGTH;
    const int matID = Config::FRACTAL_MATERIAL;

    float height = sideLength * sqrt(2.f / 3.f);
    float baseY = -height * .25f;
    float apexY = height * .75f;
    float radius = sideLength / sqrt(3.f);

    glm::vec3 v0 = glm::vec3(0.f, apexY, 0.f);
    glm::vec3 v1 = glm::vec3(0.f, baseY, radius);
    glm::vec3 v2 = glm::vec3(-sideLength / 2.f, baseY, -radius / 2.f);
    glm::vec3 v3 = glm::vec3(sideLength / 2.f, baseY, -radius / 2.f);

    // Set bounding box
    glm::vec3 minBound = glm::min(v0, glm::min(v1, glm::min(v2, v3)));
    glm::vec3 maxBound = glm::max(v0, glm::max(v1, glm::max(v2, v3)));
    bounds = AABB{ minBound, maxBound };

    size_t numTetrahedrons = 1ULL << (depth * 2);
    triangles.reserve(numTetrahedrons * 4);

    std::function<void(glm::vec3, glm::vec3, glm::vec3, glm::vec3, int)> subdivide;
    subdivide = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int currentDepth) {
        if (currentDepth == 0) {
            // Base case, build 4 faces of tetrahedron
            Triangle t1, t2, t3, t4;

            t1.a = a; t1.b = c; t1.c = b; t1.materialID = matID;
            t2.a = a; t2.b = d; t2.c = c; t2.materialID = matID;
            t3.a = a; t3.b = b; t3.c = d; t3.materialID = matID;
            t4.a = b; t4.b = c; t4.c = d; t4.materialID = matID;

            triangles.push_back(t1);
            triangles.push_back(t2);
            triangles.push_back(t3);
            triangles.push_back(t4);
        }
        else {
            // Find midpoints of edges
            glm::vec3 ab = (a + b) * .5f;
            glm::vec3 ac = (a + c) * .5f;
            glm::vec3 ad = (a + d) * .5f;
            glm::vec3 bc = (b + c) * .5f;
            glm::vec3 bd = (b + d) * .5f;
            glm::vec3 cd = (c + d) * .5f;

            // Form 4 smaller corner tetrahedrons (hollow center)
            subdivide(a, ab, ac, ad, currentDepth - 1);
            subdivide(ab, b, bc, bd, currentDepth - 1);
            subdivide(ac, bc, c, cd, currentDepth - 1);
            subdivide(ad, bd, cd, d, currentDepth - 1);
        }
    };

    subdivide(v0, v1, v2, v3, depth);
}


void Model::LoadObj(std::string objPath) {
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(RES_PATH + objPath, reader_config)) return;

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    size_t totalTriangles = 0;
    for (const auto& shape : shapes) totalTriangles += shape.mesh.num_face_vertices.size();
    triangles.reserve(totalTriangles);

    unsigned int idx = 0;
    glm::vec3 bMin(FLT_MAX);
    glm::vec3 bMax(-FLT_MAX);

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            Triangle tri;
            tri.materialID = Config::OBJ_MODEL_MATERIAL;

            for (size_t v = 0; v < 3; v++) {
                tinyobj::index_t idx_t = shapes[s].mesh.indices[index_offset + v];

                glm::vec3 p(
                    attrib.vertices[3 * size_t(idx_t.vertex_index) + 0],
                    attrib.vertices[3 * size_t(idx_t.vertex_index) + 1],
                    attrib.vertices[3 * size_t(idx_t.vertex_index) + 2]
                );

                if (v == 0) tri.a = p;
                else if (v == 1) tri.b = p;
                else tri.c = p;

                bMin = glm::min(bMin, p);
                bMax = glm::max(bMax, p);
            }

            triangles.push_back(tri);
            index_offset += 3;
        }
    }

    bounds = AABB{ bMin, bMax };
}