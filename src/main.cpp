// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#include <exception>
#include <print>
#include <thread>
#include <string>
#include <sstream>
#include <string_view>

#include "argparse/argparse.hpp"

#include "application.h"
#include "path_tracer.h"
#include "scene.h"
#include "mesh.h"
#include "mlt.h"

constexpr const char* ApplicationName = "MLT";
constexpr const char* WindowTitleMLT = "Metropolis Light Transport";
constexpr const char* WindowTitlePathTracer = "Path Tracer";

namespace {

bool matches(const std::string_view token, const std::string_view ref) {
    if (token.size() > ref.size())
        return false;
    for (size_t i = 0; i < token.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(token[i])) !=
                std::tolower(static_cast<unsigned char>(ref[i])))
            return false;
    }
    return true;
};

MLT::EnabledMutations getEnabledMutationsFromString(
        const std::string& string) {
    std::stringstream ss(string);
    std::string token;
    MLT::EnabledMutations result{};
    while (std::getline(ss, token, ',')) {
        if (token.empty())
            continue;
        if (matches(token, "newPathMutation"))
            result.newPathMutation = true;
        else if (matches(token, "lensPerturbation"))
            result.lensPerturbation = true;
        else if (matches(token, "multiChainPerturbation"))
            result.multiChainPerturbation = true;
        else if (matches(token, "bidirectionalMutation"))
            result.bidirectionalMutation = true;
        else
            throw std::runtime_error(
                std::format("Unknown mutation type: {}", token));
    }
    return result;
}

std::pair<std::size_t, std::size_t> getWindowSizeFromString(const std::string& sizeString) {
    if (sizeString == "small")  return {512, 384};
    if (sizeString == "med")    return {1024, 768};
    if (sizeString == "large")  return {1920, 1440};
    
    auto formatErrorMessage = 
        std::format("Invalid window size format: '{}'. Use 'small', 'med', 'large', or 'WIDTHxHEIGHT'", sizeString);
    
    // Parse custom dimensions as WIDTHxHEIGHT
    size_t xPos = sizeString.find('x');
    if (xPos == std::string::npos || xPos == 0 || xPos == sizeString.size() - 1)
        throw std::runtime_error(formatErrorMessage);
    
    try {
        std::size_t width = std::stoul(sizeString.substr(0, xPos));
        std::size_t height = std::stoul(sizeString.substr(xPos + 1));
        
        return {width, height};
    } catch (const std::invalid_argument& e) {
        throw std::runtime_error(formatErrorMessage);
    }
}

} // namespace

int main(int argc, const char* argv[]) {
    argparse::ArgumentParser parser(
        ApplicationName, "", argparse::default_arguments::help);

    std::filesystem::path glbFile;
    parser.add_argument("glb-file")
        .help("The .glb file to load into the scene.")
        .required()
        .store_into(glbFile);

    int numJobs = std::thread::hardware_concurrency();
    parser.add_argument("-j", "--jobs")
        .metavar("NUM_JOBS")
        .help("The size of the thread pool. By default, the hardware "
            "concurrency is used. A value less than 2 disables the thread pool.")
        .store_into(numJobs);

    bool usePathTracer = false;
    parser.add_argument("--pt", "--use-path-tracer")
        .help("Use regular path tracing instead of MLT.")
        .store_into(usePathTracer);

    MLT::EnabledMutations enabledMutations{
        .newPathMutation = true,
        .lensPerturbation = true,
        .multiChainPerturbation = true,
        .bidirectionalMutation = true};
    std::string enabledMutationsString;
    parser.add_argument("-m", "--mutations")
        .metavar("MUTATIONS")
        .help("Specifies a custom set of enabled mutators for MLT. The set "
            "should be passed as a comma-separated list of the enabled "
            "mutators from the set {newPathMutation, lensPerturbation, "
            "multiChainPerturbation, bidirectionalMutation}, with no spaces. "
            "The full name does not need to be provided; the closest match "
            "will be used.")
        .store_into(enabledMutationsString);

    std::string windowSizeString = "med";
    parser.add_argument("-w", "--window-size")
        .metavar("SIZE")
        .help("Window size as a preset ('small', 'med', 'large') or custom dimensions (WIDTHxHEIGHT). Default is 'med'.")
        .store_into(windowSizeString);

    parser.add_epilog(std::format(
        "Example usage: {} ../media/room_far.glb -m new,lens -j 8 -w large",
        ApplicationName));

    try {
        parser.parse_args(argc, argv);
        if (!enabledMutationsString.empty())
            enabledMutations =
                getEnabledMutationsFromString(enabledMutationsString);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }

    auto [windowWidth, windowHeight] = getWindowSizeFromString(windowSizeString);

    Camera camera(
        windowWidth, windowHeight, 45.0f, 0.032f,
        Vec3(0.0f, 0.0f, 1.5f),
        Vec3(0.0f, 0.0f, -1.0f),
        Vec3(0.0f, 1.0f, 0.0f));
    Scene scene(camera);
    bool isSceneLoaded = scene.loadGltf(glbFile);
    if (!isSceneLoaded)
        std::exit(1);

    Window window(windowWidth, windowHeight, WindowTitleMLT);
    GraphicsContext graphicsContext(window);
    Application application(window, graphicsContext, scene);
    if (usePathTracer) {
        window.setTitle(WindowTitlePathTracer);
        PathTracer pathTracer(window.width(), window.height());
        application.run(pathTracer, numJobs);
    } else {
        constexpr MLT::EnabledMutations DefaultConfig;
        MLT mlt(enabledMutations, window.width(), window.height(), numJobs);
        application.run(mlt, numJobs);
    }
}
