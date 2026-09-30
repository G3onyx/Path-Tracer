#include "App.h"

static void printControls() {
    const char* controls = R"(
.------------------------------------------------------------------.
|                             CONTROLS                             |
| NOTE: High quality rendering starts when nothing is moving       |
|--- ALWAYS -------------------------------------------------------|
| Escape          : Exit the program                               |
| E               : Toggle between Flying and Orbiting modes       |
| F               : Toggle fullscreen                              |
| Tab             : Toggle no interruptions (for long renders)     |
| V               : Toggle BVH view                                |
| P               : Save screenshot (to `renders` folder)          |
| R               : Print debug information                        |
|--- FLYING MODE (MOUSE CAPTURED) ---------------------------------|
| WASD/Arrow Keys : Move forward, left, right or backwards         |
| Space/Page Up   : Move up                                        |
| Shift/Page Down : Move down                                      |
| Left click      : Grab or drop interactive objects               |
| Scroll wheel    : Dash forwards/backwards (if nothing grabbed)   |
|                OR Control grabbed object's distance from camera  |
| C               : Create a new curve control point               |
| X               : Delete the grabbed control point               |
| Z               : Toggle curve closure                           |
|--- ORBITING MODE (MOUSE FREE) -----------------------------------|
| Click and drag  : Orbit grabbed object/world origin              |
| Scroll wheel    : Control camera's distance from orbit origin    |
'------------------------------------------------------------------'

)";
    
    //std::cout << "\033[2J\033[1;1H"; // clear terminal
    std::cout << controls;
}

App::App() {
    std::cout << "Loading scene... This should take less than 10 seconds\n";

	if (!InitWindow())
        throw std::runtime_error("Failed to initialize window/OpenGL!");

	camera = new Camera(window);

    scene = new Scene();
    SetupScene();

	renderer = new Renderer((int)resolution.x, (int)resolution.y, scene, camera, curves);

	interaction = new Interaction(scene, camera, curves, sun, shadow);

    printControls();
}


App::~App() {
	delete interaction;
	delete renderer;
	delete camera;
	delete scene;

    delete sun;
    delete shadow;

	for (Curve* c : curves) delete c;
	
    for (Model* m : loadedModels) delete m;

	glfwDestroyWindow(window);
	glfwTerminate();
}


bool App::InitWindow() {

    if (!glfwInit()) return false;

    window = glfwCreateWindow((int)resolution.x, (int)resolution.y, "Path tracer", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return false;
    }

    // Move window to center of display
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (mode->width - (int)resolution.x) / 2, (mode->height - (int)resolution.y) / 2);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (gl3wInit()) return false;

    glfwSetWindowUserPointer(window, this);

    // Static callbacks
    glfwSetFramebufferSizeCallback(window, ResizeCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetWindowFocusCallback(window, WindowFocusCallback);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // Debug
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugMessageCallback, nullptr);

    // Force a resize immediately
    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    OnResize(fbW, fbH);

    return true;
}


void App::SetupScene() {

    // --- Bezier curves ---

    personalCurve = new Curve();

    Curve* track = new Curve({
        glm::vec3(2.56625f, 2.91868f, -11.9825f),
        glm::vec3(-4.18621f, 1.64879f, -10.8143f),
        glm::vec3(3.84536f, -0.963625f, -9.12116f),
        glm::vec3(0.0799711f, -2.52742f, -3.62002f),
        glm::vec3(-2.84852f, -1.7509f, -9.5803f)
    }, camera, true, true, 5.f);

    Curve* bigTrack = new Curve({
        glm::vec3(-20.033, 75.7334, 10.4928),
        glm::vec3(-87.6395, -28.7555, 52.9378),
        glm::vec3(-17.9369, 8.26352, 44.3355),
        glm::vec3(42.0908, -8.68718, 30.447),
        glm::vec3(69.948, 10.2897, 39.2291),
        glm::vec3(13.9637, 79.1383, 7.66443)
    }, camera, true, false, 100.f);

    curves.push_back(personalCurve);
    curves.push_back(track);
    curves.push_back(bigTrack);


    // --- Load model data ---

    Model* roomModel = new Model("CORNELL");
    Model* fractalModel = new Model("FRACTAL");
    Model* dragonModel = new Model("dragon.obj");
    Model* sphere = new Model("BALL");

    loadedModels.push_back(roomModel);
    loadedModels.push_back(fractalModel);
    loadedModels.push_back(dragonModel);
    loadedModels.push_back(sphere);

    // --- Add instances of models to the scene ---
    
    ModelInstance* room = scene->AddInstance(roomModel);

    ModelInstance* dragon = scene->AddInstance(dragonModel);
    dragon->translate(glm::vec3(-20.f, -10.f, 30.f));
    dragon->rotate(glm::vec3(0.f, glm::radians(90.f), 0.f));
    dragon->scale(1.f / 4.f);

    ModelInstance* fractal = scene->AddInstance(fractalModel);
    fractal->rotate(glm::vec3(0.f, glm::radians(60.f), 0.f));
    
    // Balls
    sun = new ModelInstance(sphere, -1);
    sun->position = camera->Position + sunDir * Config::SUN_DIST;

    shadow = new ModelInstance(sphere, -1);
    shadow->position = camera->Position + -sunDir * Config::SUN_DIST;

    ModelInstance* light = scene->AddSphereInstance(sphere, Config::LIGHT);
    light->translate(glm::vec3(0.f, 0.f, -6.f));
    light->ignoresMovement = true;
    //track->forceAttach(light);
    scene->lightBallIndex = light->gpuInstanceIndex;

    ModelInstance* glass = scene->AddSphereInstance(sphere, Config::GLASS);
    glass->translate(glm::vec3(-2.5f, 0.f, -2.f));

    ModelInstance* mirror = scene->AddSphereInstance(sphere, Config::MIRROR);
    mirror->translate(glm::vec3(2.5f, 0.f, -2.f));

    ModelInstance* blue = scene->AddSphereInstance(sphere, Config::BLUE);
    blue->translate(glm::vec3(-4.f, 0.f, 0.f));

    ModelInstance* seaglass = scene->AddSphereInstance(sphere, Config::SEAGLASS);
    seaglass->translate(glm::vec3(4.f, 0.f, 0.f));


    scene->UploadToGPU();
}


void App::UpdateWorld(float dt) {
    // Animate spheres on curves
    for (Curve* c : curves) c->animateSpheres(dt);

    // Update
    bool canProcessMouse = camera->mouseCaptured && !frozen;
    if (canProcessMouse) camera->Process(window, dt);
    sunDir = interaction->Update(dt, sunDir, canProcessMouse);

    // Lock sun/shadow to camera
    sun->position = camera->Position + sunDir * Config::SUN_DIST;
    shadow->position = camera->Position - sunDir * Config::SUN_DIST;
}


void App::HandleResize() {
    renderer->Resize((int)resolution.x, (int)resolution.y);
    camera->updateAspectRatio((int)resolution.x, (int)resolution.y);
    camera->needsUpdate = true;
    windowResized = false;
}


void App::Run() {

    glfwFocusWindow(window);
    double lastFrameTime = glfwGetTime();

    int framesThisInterval = 0;
    float fpsTimer = 0.f;

    while (!glfwWindowShouldClose(window)) {
        renderer->WaitForGPU();

        // Delayed resize handling
        if (windowResized) HandleResize();
        glfwPollEvents();

        // Timing
        double currentFrameTime = glfwGetTime();
        float dt = (float)(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;

        // FPS calculation
        fpsTimer += dt;
        framesThisInterval++;

        if (fpsTimer >= 0.2f) {
            float currentFps = (float)framesThisInterval / fpsTimer;

            float renderScale = renderer->getRenderScale();
            unsigned int maxBounces = renderer->getMaxBounces();
            float timeRendering = renderer->getRenderTimer();

            std::string timeStr = timeRendering > 0.f ? std::format(" | RENDERING: {:.2f}s", timeRendering) : "";

            std::string title = std::format("Path tracer | FPS: {:.1f} | Max bounces: {} | Resolution: {}%",
                currentFps, maxBounces, (int)(renderScale * 100.f));

            glfwSetWindowTitle(window, (title + timeStr).c_str());

            // Reset trackers
            fpsTimer = 0.f;
            framesThisInterval = 0;
        }

        UpdateWorld(dt);

        // Movement detection for TAA
        bool sceneChanged = scene->UpdateDynamicData() || camera->needsUpdate;
        camera->needsUpdate = false;

        // Render
        renderer->RenderFrame(sceneChanged, dt, seeBVH, sunDir, interaction->GetHighlightedCurve(), needsSave);

        glfwSwapBuffers(window);
    }
}


static void print(std::string name, glm::vec3 vec) {
    std::string text = name.empty() ? "" : name + ":\n";
    std::cout << text.c_str() << "glm::vec3(" << vec.x << "f, " << vec.y << "f, " << vec.z << "f),\n";
}

void App::KeyCallback(GLFWwindow* w, int k, int s, int a, int m) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    if (app) app->OnKey(k, a);
}

void App::MouseCallback(GLFWwindow* w, double x, double y) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    if (app) app->OnMousePosition(x, y);
}

void App::MouseButtonCallback(GLFWwindow* w, int b, int a, int m) {
    if (b != GLFW_MOUSE_BUTTON_LEFT || !glfwGetWindowAttrib(w, GLFW_FOCUSED)) return;

    App* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    if (app && !app->frozen) {
        app->interaction->OnLeftClick(a);
    }
}

void App::ScrollCallback(GLFWwindow* w, double xoffset, double yoffset) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    if (app && !app->frozen) {
        app->interaction->OnScroll(yoffset);
    }
}

void App::ResizeCallback(GLFWwindow* w, int width, int height) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    if (app) app->OnResize(width, height);
}

void App::WindowFocusCallback(GLFWwindow* w, int focused) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    if (app) app->OnFocus(focused);
}

void APIENTRY App::DebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    if (type == GL_DEBUG_TYPE_ERROR)
        throw std::runtime_error(std::string("GL ERROR: ") + message);
}


void App::OnKey(int k, int a) {
    if (a == GLFW_PRESS) {

        // Close the app
        if (k == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        }

        // Toggle frozen mode
        else if (k == GLFW_KEY_TAB) {
            frozen = !frozen;
            if (frozen) {
                std::cout << "Movement frozen (Press TAB to unfreeze)\n";
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            else {
                glfwSetInputMode(window, GLFW_CURSOR, camera->mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                camera->firstMouse = true;
            }
        }

        // Save render
        else if (k == GLFW_KEY_P) needsSave = true;

        if (frozen) return;

        // Toggle fullscreen
        else if (k == GLFW_KEY_F) {
            if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
                glfwRestoreWindow(window);
            else
                glfwMaximizeWindow(window);
        }

        // Toggle modes
        if (k == GLFW_KEY_E) {
            interaction->ToggleMode(window);
        }

        // Place/remove bezier curve point
        else if (k == GLFW_KEY_C) personalCurve->addPoint(camera, Config::MIN_HOLD_DISTANCE);
        else if (k == GLFW_KEY_X) interaction->DeletePoint(personalCurve);
        // Toggle curve connect
        else if (k == GLFW_KEY_Z) personalCurve->toggleClosed(camera);

        // Toggle BVH view
        else if (k == GLFW_KEY_V) {
            seeBVH = !seeBVH;
            camera->needsUpdate = true;
        }

        // Print useful info to console
        else if (k == GLFW_KEY_R) {
            std::cout << "PLAYER" << std::endl;
            print("Pos", camera->Position);
            print("Front", camera->Front);
            print("Up", camera->Up);

            std::cout << "CURVE" << std::endl;
            for (glm::vec3 cp : personalCurve->controlPoints)
                print("", cp);
            std::cout << std::endl;
        }
    }
}

void App::OnMousePosition(double x, double y) {
    if (frozen) return;

    if (camera->firstMouse) {
        camera->lastX = x;
        camera->lastY = y;
        camera->firstMouse = false;
    }

    float xoffset = (float)(x - camera->lastX);
    float yoffset = (float)(camera->lastY - y);

    camera->lastX = x;
    camera->lastY = y;

    // Standard looking around
    if (camera->mouseCaptured) {
        camera->Yaw += xoffset * camera->MouseSensitivity;
        camera->Pitch += yoffset * camera->MouseSensitivity;
        camera->Pitch = glm::clamp(camera->Pitch, -89.f, 89.f);

        camera->updateCameraVectors();
    }

    // Orbit interactables with click and drag panning
    else if (camera->panning) {
        camera->Yaw += xoffset * camera->MouseSensitivity;
        camera->Pitch += yoffset * camera->MouseSensitivity;
        camera->Pitch = glm::clamp(camera->Pitch, -89.f, 89.f);

        camera->updateCameraVectors();
    }
}

void App::OnResize(int width, int height) {
    resolution = glm::vec2((float)width, (float)height);
    windowResized = true;
}

void App::OnFocus(int focused) {
    if (focused && camera->mouseCaptured) camera->firstMouse = true;
}