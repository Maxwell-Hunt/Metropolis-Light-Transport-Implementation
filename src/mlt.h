// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#pragma once

#include <random>

#include "image.h"
#include "scene.h"
#include "threadpool.h"
#include "renderer.h"
#include "path.h"

class MLT;

class MLTProcess {
public:
    MLTProcess(const MLT& renderer, int width, int height);

    MLTProcess(const MLTProcess&) = delete;
    MLTProcess& operator=(const MLTProcess&) = delete;
    MLTProcess(MLTProcess&&) = default;
    MLTProcess& operator=(MLTProcess&&) = delete;

    void accumulate(const Scene& scene, int numMutations);
    const Image& accumulationBuffer() const { return _accumulationBuffer; }
    float accumulatedLuminance() const { return _accumulatedLuminance; }
    int numNewPathMutations() const { return _numNewPathMutations; }
    float averageSamplesPerPixel() const { return _averageSamplesPerPixel; }
    void reset();

private:
    struct State {
        Path path;
        Vec2 pixel;
        EvaluationResult evaluation;
    };

    struct MutationInfo {
        enum class Type : int {
            NewPath = 0,
            Lens = 1,
            MultiChain = 2,
            Bidirectional = 3
        };
        State proposal;
        float acceptance;
        Type type;
    };

    // Bidirectional mutations involve taking the current light path,
    // deleting a subpath and replacing it with a newly generated subpath.
    // Note that our implementation differs slightly from that of Veach and
    // Guibas since we currently only generate paths from the eye rather than
    // bidirectionally. Still, the spirit of this mutation should remain the
    // same.
    std::optional<MutationInfo> bidirectionalMutation(const Scene& scene);

    // Eye path perturbations involve slightly adjusting the outgoing direction
    // of the eye ray, propagating through the same number of specular bounces
    // as the original path, and then connecting back to the original path.
    // Multichain perturbations are the same with the exception that diffuse
    // bounces are also slightly perturbed reconnecting
    std::optional<MutationInfo> eyePathPerturbation(const Scene& scene, bool multiChain);

    // New path mutations generate a new path independent of the current path
    // based on Russian Roulette.
    std::optional<MutationInfo> computeNewPathMutation(const Scene& scene);

    std::optional<MutationInfo> computeRandomMutation(const Scene& scene);

    const MLT& _renderer;
    Image _accumulationBuffer;
    float _accumulatedLuminance = 0.0f;
    int _numNewPathMutations = 0;
    float _averageSamplesPerPixel = 0.0f;
    std::optional<State> _currentState;
    std::discrete_distribution<> _mutationDistribution;
};

class MLT : public IRenderer {
public:
    struct EnabledMutations {
        bool newPathMutation = false;
        bool lensPerturbation = false;
        bool multiChainPerturbation = false;
        bool bidirectionalMutation = false;
    };

    MLT(const EnabledMutations& config, int width, int height, int numProcesses = 1);

    MLT(const MLT&) = delete;
    MLT& operator=(const MLT&) = delete;
    MLT(MLT&&) = delete;
    MLT& operator=(MLT&&) = delete;

    virtual void accumulate(
        const Scene& scene,
        int numSamples,
        ThreadPool* pool = nullptr) override;
    virtual void updateFrameBuffer(Image& frameBuffer) const override; 
    virtual int numSamplesPerPixel() const override { return _averageSamplesPerPixel; }
    virtual void reset() override;

    const EnabledMutations& getConfig() const { return _config; }

private:
    /// Compute the scaling factor needed to make the histogram approximate the image.
    float computeScaleFactor() const;

    EnabledMutations _config;
    int _width;
    int _height;
    std::vector<MLTProcess> _processes;
    int _averageSamplesPerPixel = 0;
};
