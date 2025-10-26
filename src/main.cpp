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

    parser.add_epilog(std::format(
        "Example usage: {} ../media/room_far.glb -m new,lens -j 8",
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

    Camera camera(
        512, 384, 45.0f, 0.032f,
        Vec3(0.0f, 0.0f, 1.5f),
        Vec3(0.0f, 0.0f, -1.0f),
        Vec3(0.0f, 1.0f, 0.0f));
    Scene scene(camera);
    bool isSceneLoaded = scene.loadGltf(glbFile);
    if (!isSceneLoaded)
        std::exit(1);

    Window window(512, 384, WindowTitleMLT);
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
