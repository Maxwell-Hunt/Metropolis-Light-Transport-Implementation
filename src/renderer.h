// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#pragma once

#include "image.h"
#include "scene.h"
#include "threadpool.h"

/// Abstract base class for different rendering techniques to implement.
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void accumulate(
        const Scene& scene,
        int numSamples,
        ThreadPool* pool = nullptr) = 0;

    virtual void updateFrameBuffer(Image& frameBuffer) const = 0; 

    virtual void reset() { _isStopping = false; }
    virtual void stop() { _isStopping = true; }
    bool isStopping() const { return _isStopping; }

    virtual int numSamplesPerPixel() const = 0;

protected:
    std::atomic<bool> _isStopping = false;
};
