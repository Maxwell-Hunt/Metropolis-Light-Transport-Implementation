// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#pragma once

#include <chrono>
#include <functional>
#include <string_view>

#include "GL/glew.h"
#define NOMINMAX
#include "GLFW/glfw3.h"

#include "image.h"
#include "renderer.h"
#include "scene.h"
#include "types.h"
#include "threadpool.h"

class IEventHandler {
public:
    virtual ~IEventHandler() = default;
    virtual void onKey(int key, int scancode, int action, int mods) = 0;
    virtual void onMouseMove(double xpos, double ypos) = 0;
    virtual void onMouseButton(int button, int action, int mods) = 0;
};

class Window {
public:
    Window(int width, int height, std::string_view title);
    ~Window();

    GLFWwindow* handle() const { return _handle; }
    int width() const { return _width; }
    int height() const { return _height; }
    void pollEvents();
    void swapBuffers();
    bool shouldClose() const;
    void setEventHandler(IEventHandler* handler);
    float getDeltaTime() const { return _deltaTime; }
    void setTitle(std::string_view title);

private:
    GLFWwindow* _handle = nullptr;
    int _width;
    int _height;
    IEventHandler* _eventHandler = nullptr;
    std::chrono::high_resolution_clock::time_point _lastTime;
    float _deltaTime = 0.0f;
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};

class GraphicsContext {
public:
    explicit GraphicsContext(Window& window);
    ~GraphicsContext();
    void drawImage(const Image& frameBuffer);

private:
    Window& _window;
    GLuint _frameBufferTexture = 0;
    GLuint _fragmentShaderProgram = 0;
};

class RenderProcess {
public:
    RenderProcess(
        IRenderer& renderer, Scene& scene, int width, int height, int numJobs);
    ~RenderProcess();

    /// Live converging frame buffer for presentation.
    /// @warning
    ///     This frame buffer may be invalidated at any point in the future;
    ///     there is no lock for access.
    const Image& frameBuffer() const { return *_frontBuffer; }

    /// Should be called when the scene changes.
    void reset();

private:
    void renderLoop();

    IRenderer& _renderer;
    Scene& _scene;

    std::array<Image, 2> _frameBuffers;
    Image* _frontBuffer;
    Image* _backBuffer;

    std::thread _thread;
    std::optional<ThreadPool> _threadPool;
};

class Application : public IEventHandler {
public:
    static constexpr float MouseSensitivity = 0.005f;
    static constexpr float MovementSpeed = 2.0f;

    Application(Window& window, GraphicsContext& graphicsContext, Scene& scene);
    void run(IRenderer& renderer, int numJobs);

    void onKey(int key, int scancode, int action, int mods) override;
    void onMouseMove(double xpos, double ypos) override;
    void onMouseButton(int button, int action, int mods) override;

private:
    Window& _window;
    GraphicsContext& _graphicsContext;
    Scene& _scene;
    int _frameCount;

    Vec2 _movementDirection;
    Vec2 _mouseDelta;
    bool _isMousePressed;
    bool _saveNextFrameToDisk;
    std::optional<Vec2> _lastMousePosition;
};
