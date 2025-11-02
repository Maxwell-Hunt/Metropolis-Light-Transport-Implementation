// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#pragma once

#include "image.h"
#include "scene.h"
#include "threadpool.h"
#include "renderer.h"

class PathTracer : public IRenderer {
public:
    PathTracer(int width, int height)
        : _accumulationBuffer(width, height, 3) {}

    virtual void accumulate(
        const Scene& scene,
        int numSamples,
        ThreadPool* pool = nullptr) override;

    virtual void updateFrameBuffer(Image& frameBuffer) const override; 

    void accumulateBlock(
        const Scene& scene,
        int numSamples,
        std::size_t x,
        std::size_t y,
        std::size_t blockWidth);

    virtual int numSamplesPerPixel() const override { return _numSamplesPerPixel; }

    virtual void reset() override;

private:
    Image _accumulationBuffer;
    int _numSamplesPerPixel = 0;
};