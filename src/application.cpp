#include "application.h"

#include <iostream>
#include <vector>
#include <cfloat>
#include <optional>
#include <random>
#include <print>
#include <chrono>
#include <cmath>

#include "tracy/Tracy.hpp"

#include "image.h"
#include "random.h"
#include "types.h"

namespace {

constexpr const char* FragmentShaderSrc = R"(
    #version 120

    uniform sampler2D input_tex;
    uniform vec4 BufInfo;

    void main()
    {
        gl_FragColor = texture2D(input_tex, gl_FragCoord.st * BufInfo.zw);
    }
)";

} // namespace

Window::Window(int width, int height, std::string_view title)
        : _width(width), _height(height) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        std::exit(-1);
    }
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    _handle = glfwCreateWindow(_width, _height, title.data(), nullptr, nullptr);
    if (!_handle) {
        std::cerr << "Failed to open GLFW window." << std::endl;
        glfwTerminate();
        std::exit(-1);
    }
}

Window::~Window() {
    if (_handle) glfwDestroyWindow(_handle);
    glfwTerminate();
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::swapBuffers() {
    glfwSwapBuffers(_handle);
}

bool Window::shouldClose() const {
    return _handle == nullptr || glfwWindowShouldClose(_handle) == GL_TRUE;
}

void Window::setEventHandler(IEventHandler* handler) {
    _eventHandler = handler;
    glfwSetWindowUserPointer(_handle, this);
    glfwSetKeyCallback(_handle, Window::keyCallback);
    glfwSetCursorPosCallback(_handle, Window::cursorPosCallback);
    glfwSetMouseButtonCallback(_handle, Window::mouseButtonCallback);
}

void Window::setTitle(std::string_view title) {
    if (_handle) {
        glfwSetWindowTitle(_handle, title.data());
    }
}

void Window::keyCallback(GLFWwindow* handle, int key, int scancode, int action, int mods) {
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window && window->_eventHandler) {
        window->_eventHandler->onKey(key, scancode, action, mods);
    }
}

void Window::cursorPosCallback(GLFWwindow* handle, double xpos, double ypos) {
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window && window->_eventHandler) {
        window->_eventHandler->onMouseMove(xpos, ypos);
    }
}

void Window::mouseButtonCallback(GLFWwindow* handle, int button, int action, int mods) {
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window && window->_eventHandler) {
        window->_eventHandler->onMouseButton(button, action, mods);
    }
}

GraphicsContext::GraphicsContext(Window& window) : _window(window) {
    glfwMakeContextCurrent(window.handle());

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW." << std::endl;
        glfwTerminate();
        std::exit(-1);
    }
    // create shader
    _fragmentShaderProgram = glCreateProgram();
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &FragmentShaderSrc, 0);
    glCompileShader(fragmentShader);
    glAttachShader(_fragmentShaderProgram, fragmentShader);
    glLinkProgram(_fragmentShaderProgram);
    glDeleteShader(fragmentShader);
    // create texture
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &_frameBufferTexture);
    glBindTexture(GL_TEXTURE_2D, _frameBufferTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB32F_ARB, _window.width(), _window.height(), 0,
        GL_LUMINANCE, GL_FLOAT, 0);

    glDisable(GL_DEPTH_TEST);

    glUseProgram(_fragmentShaderProgram);
    glUniform1i(glGetUniformLocation(_fragmentShaderProgram, "input_tex"), 0);

    GLint dims[4];
    glGetIntegerv(GL_VIEWPORT, dims);
    const float BufInfo[4] = {
        float(dims[2]),
        float(dims[3]),
        1.0f / float(dims[2]),
        1.0f / float(dims[3])};
    glUniform4fv(
        glGetUniformLocation(_fragmentShaderProgram, "BufInfo"), 1, BufInfo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _frameBufferTexture);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

GraphicsContext::~GraphicsContext() {
    glfwMakeContextCurrent(_window.handle());

    if (_fragmentShaderProgram) {
        glDeleteProgram(_fragmentShaderProgram);
        _fragmentShaderProgram = 0;
    }

    if (_frameBufferTexture) {
        glDeleteTextures(1, &_frameBufferTexture);
        _frameBufferTexture = 0;
    }
}

void GraphicsContext::drawImage(const Image& image) {
    glTexSubImage2D(
        GL_TEXTURE_2D, 0, 0, 0, image.width(), image.height(),
        GL_RGB, GL_FLOAT, image.pixels());
    glRecti(1, 1, -1, -1);
}

Application::Application(
        Window& window, GraphicsContext& graphicsContext, Scene& scene)
    : _window(window),
      _graphicsContext(graphicsContext),
      _scene(scene),
      _frameCount(0),
      _movementDirection(0.0f),
      _mouseDelta(0.0f),
      _isMousePressed(false),
      _saveNextFrameToDisk(false) {}

void Application::run(IRenderer& renderer, int numJobs) {
    RenderProcess renderProcess(
        renderer, _scene, _window.width(), _window.height(), numJobs);
    _window.setEventHandler(this);
    auto lastTime = std::chrono::high_resolution_clock::now();
    constexpr auto FrameTime = std::chrono::duration<float>(std::chrono::seconds(1)) / 20;
    while (!_window.shouldClose()) {
        const auto startTime = std::chrono::high_resolution_clock::now();
        const Image& frameBuffer = renderProcess.frameBuffer();

        if (_saveNextFrameToDisk) {
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            std::tm* tm = std::localtime(&t);
            if (tm) {
                std::ostringstream oss;
                oss << "screenshot_" << std::put_time(tm, "%Y_%m_%d_%H_%M_%S") << ".png";
                frameBuffer.save(oss.str());
            }
            _saveNextFrameToDisk = false;
        }

        _graphicsContext.drawImage(frameBuffer);
        _window.swapBuffers();

        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        _window.pollEvents();
        bool renderNeedsReset = false;
        if (glm::length(_movementDirection) > Epsilon) {
            _scene.camera.move(deltaTime * MovementSpeed * (
                _scene.camera.forward * _movementDirection.y +
                _scene.camera.right   * _movementDirection.x));
            renderNeedsReset = true;
        }
        if (glm::length(_mouseDelta) > Epsilon) {
            _scene.camera.rotate(
                _mouseDelta.x * MouseSensitivity,
                -_mouseDelta.y * MouseSensitivity);
            _mouseDelta = Vec2(0.0f);
            renderNeedsReset = true;
        }

        if (renderNeedsReset)
            renderProcess.reset();

        ++_frameCount;

        const auto endTime = std::chrono::high_resolution_clock::now();
        const auto timeTaken = endTime - startTime;
        if (timeTaken < FrameTime)
            std::this_thread::sleep_for(FrameTime - timeTaken);
    }
}

void Application::onKey(int key, int scancode, int action, int mods) {
    float multiplier = 0.0f;
    if (action == GLFW_PRESS)
        multiplier = 1.0f;
    else if (action == GLFW_RELEASE)
        multiplier = -1.0f;

    if (key == GLFW_KEY_W)
        _movementDirection += Vec2(0.0f, 1.0f) * multiplier;
    else if (key == GLFW_KEY_S)
        _movementDirection += Vec2(0.0f, -1.0f) * multiplier;
    else if (key == GLFW_KEY_A)
        _movementDirection += Vec2(-1.0f, 0.0f) * multiplier;
    else if (key == GLFW_KEY_D)
        _movementDirection += Vec2(1.0f, 0.0f) * multiplier;

    if (key == GLFW_KEY_I && action == GLFW_PRESS)
        _saveNextFrameToDisk = true;
}

void Application::onMouseMove(double xpos, double ypos) {
    if (!_lastMousePosition) {
        _lastMousePosition = {xpos, ypos};
    }
    if (_isMousePressed)
        _mouseDelta += Vec2(
            xpos - _lastMousePosition->x,
            ypos - _lastMousePosition->y);
    _lastMousePosition = {xpos, ypos};
}

void Application::onMouseButton(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS)
            _isMousePressed = true;
        else if (action == GLFW_RELEASE)
            _isMousePressed = false;
    }
}

RenderProcess::RenderProcess(
        IRenderer& renderer, Scene& scene, int width, int height, int numJobs)
    : _renderer(renderer),
      _scene(scene),
      _frameBuffers{Image(width, height, 3), Image(width, height, 3)},
      _frontBuffer(&_frameBuffers[0]),
      _backBuffer(&_frameBuffers[1]) {
    if (numJobs > 1)
        _threadPool.emplace(numJobs);
    _thread = std::thread(std::bind_front(&RenderProcess::renderLoop, this));
}

RenderProcess::~RenderProcess() {
    _renderer.stop();
    _thread.join();
}

void RenderProcess::reset() {
    _renderer.stop();
    _thread.join();
    _renderer.reset();
    _thread = std::thread(std::bind_front(&RenderProcess::renderLoop, this));
}

void RenderProcess::renderLoop() {
    tracy::SetThreadName("Render Thread");
    constexpr int NumSamplesToTake = 16384;
    constexpr int MaxNumSamplesPerStep = 128;
    int sampleStepSize = 1;
    const auto startTime = std::chrono::high_resolution_clock::now();
    while (_renderer.numSamplesPerPixel() < NumSamplesToTake) {
        FrameMark;
        _renderer.accumulate(
            _scene, sampleStepSize,
            _threadPool ? &_threadPool.value() : nullptr);
        if (_renderer.isStopping())
            break;
        if (sampleStepSize < MaxNumSamplesPerStep) {
            sampleStepSize *= 2;
        } else {
            const auto currentTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = currentTime - startTime;
            std::println("Samples per pixel: {}, Time: {:.3f}s",
                _renderer.numSamplesPerPixel(), elapsed.count());
        }
        _renderer.updateFrameBuffer(*_backBuffer);
        std::swap(_frontBuffer, _backBuffer);
    }
}
