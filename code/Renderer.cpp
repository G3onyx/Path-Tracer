#include "Renderer.h"

Renderer::Renderer(int width, int height, Scene* scene, Camera* camera, std::vector<Curve*>& curves)
    : scene(scene), camera(camera), curves(curves)
{
    std::vector<float> emptyData(width * height * 4, 0.f);

    // Accumulation buffers
    glGenTextures(2, accumTextures);
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, accumTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, emptyData.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Intermediate raw data buffer
    glGenTextures(1, &noisyTexture);
    glBindTexture(GL_TEXTURE_2D, noisyTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, emptyData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindImageTexture(0, accumTextures[writeIndex], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    
    LoadResources();
    
    InitScreenQuad();
    InitCurveCircle();
    InitCurveCross();

    // Set constant uniforms
    shader->use();
    shader->setFloat("fov", camera->FOV);
    shader->setInt("tlasRoot", scene->tlasRootIndex);
    shader->setFloat("SUN_THRESHOLD", Config::SUN_THRESHOLD);
    shader->setInt("lightBallIndex", scene->lightBallIndex);

    shader->setInt("blueNoise", 2);
    blueNoise->bind(2);

    taaShader->use();
    taaShader->setInt("noisyTexture", 0);
    taaShader->setInt("historyImage", 1);

    displayShader->use();
    displayShader->setInt("accumTexture", 0);
}

Renderer::~Renderer() {
    glDeleteTextures(2, accumTextures);
    glDeleteTextures(1, &noisyTexture);

    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &pointVAO);
    glDeleteBuffers(1, &pointVBO);
    glDeleteVertexArrays(1, &crossVAO);
    glDeleteBuffers(1, &crossVBO);

    delete shader;
    delete taaShader;
    delete displayShader;
    delete bezierShader;

    delete blueNoise;
    delete marbleAlbedo;
    delete marbleRoughness;
    delete skybox;
}

void Renderer::InitScreenQuad() {
    float quadVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        3.0f, -1.0f, 2.0f, 0.0f,
        -1.0f, 3.0f, 0.0f, 2.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void Renderer::InitCurveCircle() {
    std::vector<glm::vec3> verts;
    const float pi2 = 3.14159265359f * 2.f;

    for (int i = 0; i < 32; i++) {
        float t1 = (float)i / 32.f * pi2;
        float t2 = (float)(i + 1) / 32.f * pi2;

        verts.push_back(glm::vec3(cos(t1), sin(t1), 0.f));
        verts.push_back(glm::vec3(cos(t2), sin(t2), 0.f));
    }
    circleVertexNum = (int)verts.size();

    glGenVertexArrays(1, &pointVAO);
    glGenBuffers(1, &pointVBO);
    glBindVertexArray(pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
}

void Renderer::InitCurveCross() {
    const float r = cos(glm::radians(45.f));
    std::vector<glm::vec3> verts = {
        glm::vec3(-r, -r, 0.f), glm::vec3(r,  r, 0.f),
        glm::vec3(-r,  r, 0.f), glm::vec3(r, -r, 0.f)
    };
    crossVertexNum = (int)verts.size();

    glGenVertexArrays(1, &crossVAO);
    glGenBuffers(1, &crossVBO);
    glBindVertexArray(crossVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crossVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
}

void Renderer::LoadResources() {
    shader = new Shader("shader.comp");
    taaShader = new Shader("taa.comp");
    displayShader = new Shader("shader.vert", "display.frag");
    bezierShader = new Shader("bezier.vert", "bezier.frag");

    blueNoise = new Texture("LDR_RGBA_0.png", true);
    marbleAlbedo = new Texture("marble_albedo.png", false);
    marbleRoughness = new Texture("marble_roughness.png", false);
    skybox = new Texture("skybox.png", true);
}

void Renderer::Resize(int width, int height) {
    if (height == 0) height = 1;

    resolution = glm::vec2((float)width, (float)height);
    glViewport(0, 0, width, height);

    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, accumTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    }

    glBindTexture(GL_TEXTURE_2D, noisyTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
}

// https://en.wikipedia.org/wiki/Halton_sequence
static float getHalton(uint32_t index, uint32_t base) {
    float f = 1.f;
    float r = 0.f;
    uint32_t current = index;
    while (current > 0) {
        f = f / (float)base;
        r = r + f * (float)(current % base);
        current = current / base;
    }
    return r;
}

static glm::vec3 getSafeInvDir(glm::vec3 dir) {
    float x = std::abs(dir.x) < 1e-7f ? (dir.x < 0.f ? -1e-7f : 1e-7f) : dir.x;
    float y = std::abs(dir.y) < 1e-7f ? (dir.y < 0.f ? -1e-7f : 1e-7f) : dir.y;
    float z = std::abs(dir.z) < 1e-7f ? (dir.z < 0.f ? -1e-7f : 1e-7f) : dir.z;
    return glm::vec3(1.f / x, 1.f / y, 1.f / z);
}


void Renderer::DynamicQuality(bool isMoving, float dt) {

    // State transitioning
    if (isMoving) {
        if (state != RenderState::MOVING) {
            state = RenderState::MOVING;
            q = mq;
            q.useCheckerboard = true;
            return;
        }
    }
    else {
        if (state == RenderState::MOVING) {
            state = RenderState::RAMPING;
            mq = q; // Save current moving quality
            q.useCheckerboard = false;
        }
        else if (state == RenderState::SETTLED)
            return;
    }

    const float margin = 0.003f;
    bool moving = (state == RenderState::MOVING);
    float limit = moving ? MOVING_DELTA_LIMIT : STATIC_DELTA_LIMIT;
    unsigned int minBounces = moving ? MOVING_MIN_BOUNCES : STATIC_MIN_BOUNCES;
    float downDamp = moving ? 0.9f : 0.95f;
    float upDamp = moving ? 0.95f : 1.f;
    unsigned int upBump = moving ? 1u : 2u;

    // Downscaling (when GPU is stalling)
    if (dt > limit + margin) {
        if (q.maxBounces > minBounces) {
            unsigned int target = (unsigned int)(q.maxBounces * (limit / dt));
            q.maxBounces = std::max(minBounces, std::min(q.maxBounces - 1u, target));
        }
        else {
            float target = q.renderScale * std::sqrt(limit / dt);
            q.renderScale = std::max(MIN_RENDER_SCALE, target * downDamp);
        }

        if (!moving) state = RenderState::SETTLED;
    }
    // Upscaling (when GPU has headroom)
    else if (dt < limit - margin) {
        float speedup = limit / std::max(dt, 0.001f);

        // First, bounces to minimum
        if (q.maxBounces < minBounces) {
            unsigned int target = (unsigned int)(q.maxBounces * speedup);
            q.maxBounces = std::min(minBounces, std::max(q.maxBounces + 1u, target));
        }
        // Then, resolution as close as possible to 100%
        else if (q.renderScale < MAX_RENDER_SCALE) {
            float target = q.renderScale * std::sqrt(speedup);
            q.renderScale = std::min(MAX_RENDER_SCALE, std::max(q.renderScale + 0.01f, target * upDamp));
        }
        // Finally, bounces to maximum
        else if (q.maxBounces < MAX_BOUNCES) {
            unsigned int target = (unsigned int)(q.maxBounces * speedup);
            q.maxBounces = std::min(MAX_BOUNCES, std::max(q.maxBounces + upBump, (unsigned int)(target * upDamp)));
        }
        else if (!moving) state = RenderState::SETTLED;
    }
    else if (!moving) state = RenderState::SETTLED;
}

void Renderer::RenderFrame(bool sceneChanged, float dt, bool seeBVH, glm::vec3 sunDir, Curve* highlightedCurve, bool& needsSave) {

    if (sceneChanged)
        stoppedTimer = Config::STOPPED_DELAY;
    else if (stoppedTimer > 0.f)
        stoppedTimer -= dt;
    bool isMoving = stoppedTimer > 0.f;


    // Dynamic quality
    RenderState prevState = state;
    DynamicQuality(isMoving, dt);

    
    if (state != RenderState::SETTLED || prevState != RenderState::SETTLED) {
        accumCount = 1;
        renderTimer = 0.f;
    }
    else {
        accumCount++;
        renderTimer += dt;
    }


    glm::ivec2 virtualRes = glm::ivec2(resolution * q.renderScale);
    int dispatchX = (virtualRes.x + 7) / 8;
    int dispatchY = (virtualRes.y + 7) / 8;

    int currentFrame = frameCount++;
    glm::vec2 rayScale = glm::vec2(camera->Scale * camera->AspectRatio, camera->Scale);

    // PATH TRACER

    shader->use();
    shader->setIVec2("resolution", virtualRes);
    shader->setInt("frameCount", currentFrame);
    shader->setVec3("camPos", camera->Position);
    shader->setVec3("camForward", camera->Front);
    shader->setVec3("camRight", camera->Right);
    shader->setVec3("camUp", camera->Up);
    shader->setInt("maxBounces", q.maxBounces);
    shader->setBool("seeBVH", seeBVH);
    shader->setVec3("sunDir", sunDir);
    shader->setBool("useCheckerboard", q.useCheckerboard);
    shader->setVec2("rayScale", rayScale);
    shader->setVec3("sunInvDir", getSafeInvDir(sunDir));
    shader->setVec2("subpixelJitter", glm::vec2(getHalton(accumCount, 2), getHalton(accumCount, 3)));

    shader->setInt("marbleAlbedo", 3);
    shader->setInt("marbleRoughness", 4);
    shader->setInt("skybox", 5);

    marbleAlbedo->bind(3);
    marbleRoughness->bind(4);
    skybox->bind(5);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, scene->nodeSSBO[scene->currentBuffer]);

    glBindImageTexture(0, noisyTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(dispatchX, dispatchY, 1);

    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    // TAA

    writeIndex = 1 - writeIndex;
    int readIndex = 1 - writeIndex;

    taaShader->use();
    taaShader->setIVec2("resolution", virtualRes);
    taaShader->setVec2("rayScale", rayScale);
    taaShader->setBool("useTAA", state != RenderState::SETTLED);
    taaShader->setInt("accumCount", accumCount);
    taaShader->setInt("frameCount", currentFrame);
    taaShader->setMat4("prevPV", prevPV);
    taaShader->setVec2("prevRenderScale", prevRenderScale);
    taaShader->setBool("useCheckerboard", q.useCheckerboard);

    taaShader->setVec3("camPos", camera->Position);
    taaShader->setVec3("camForward", camera->Front);
    taaShader->setVec3("camRight", camera->Right);
    taaShader->setVec3("camUp", camera->Up);

    // Bind textures
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, noisyTexture);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, accumTextures[readIndex]);

    // Write smoothed result to accumTexture
    glBindImageTexture(0, accumTextures[writeIndex], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute(dispatchX, dispatchY, 1);

    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);


    // DISPLAY
    glm::vec2 renderScale = glm::vec2(virtualRes) / resolution;

    displayShader->use();

    displayShader->setVec2("renderScale", renderScale);
    displayShader->setBool("isMoving", isMoving);
    displayShader->setVec2("resolution", resolution);

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, accumTextures[writeIndex]);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // BEZIER CURVES
    if (isMoving) {
        for (Curve* c : curves)
            c->draw(*bezierShader, camera, renderScale, resolution, accumTextures[writeIndex], highlightedCurve == c, pointVAO, circleVertexNum, crossVAO, crossVertexNum);
    }
    

    // Take screenshots
    if (needsSave) {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::string filename = std::format("renders/render_took_{:.2f}s_at_{:%Yy%mm%dd%Hh%Mm%Ss}{:03d}ms.bmp",
            renderTimer, now, ms.count());

        SaveScreenshot(filename);
        needsSave = false;
    }

    InsertFence();

    prevPV = camera->GetPV();
    prevRenderScale = renderScale;
}

void Renderer::WaitForGPU() {
    if (fence) {
        GLenum waitReturn = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 5000000000);
        if (waitReturn == GL_ALREADY_SIGNALED || waitReturn == GL_CONDITION_SATISFIED) {
            glDeleteSync(fence);
            fence = nullptr;
        }
        else
            std::cerr << "GPU Sync Timeout or Error!" << std::endl;
    }
}

void Renderer::InsertFence() {
    fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

bool Renderer::SaveScreenshot(const std::string& filename) const {
    int width = (int)resolution.x;
    int height = (int)resolution.y;

    int rowSize = (width * 3 + 3) & ~3;
    int dataSize = rowSize * height;
    std::vector<unsigned char> pixels(dataSize);

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());

    // File Header
    unsigned char fileHeader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
    // Info Header
    unsigned char infoHeader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0 };

    int fileSize = 54 + dataSize;
    fileHeader[2] = (unsigned char)(fileSize);
    fileHeader[3] = (unsigned char)(fileSize >> 8);
    fileHeader[4] = (unsigned char)(fileSize >> 16);
    fileHeader[5] = (unsigned char)(fileSize >> 24);

    infoHeader[4] = (unsigned char)(width);
    infoHeader[5] = (unsigned char)(width >> 8);
    infoHeader[6] = (unsigned char)(width >> 16);
    infoHeader[7] = (unsigned char)(width >> 24);
    infoHeader[8] = (unsigned char)(height);
    infoHeader[9] = (unsigned char)(height >> 8);
    infoHeader[10] = (unsigned char)(height >> 16);
    infoHeader[11] = (unsigned char)(height >> 24);

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not create file: " << filename << std::endl;
        return false;
    }

    file.write((char*)fileHeader, 14);
    file.write((char*)infoHeader, 40);
    file.write((char*)pixels.data(), dataSize);
    file.close();

    std::cout << "Successfully saved: " << filename << std::endl;
    return true;
}