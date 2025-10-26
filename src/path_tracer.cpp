#include "path_tracer.h"

#include <queue>
#include <mutex>
#include <vector>
#include <condition_variable>

#include "tracy/Tracy.hpp"

#include "path.h"
#include "random.h"

void PathTracer::accumulate(
        const Scene& scene, int numSamples, ThreadPool* pool) {
    if (pool) {
        const std::size_t blockWidth = 32;
        for (int j = 0; j < _accumulationBuffer.height(); j += blockWidth) {
            for (int i = 0; i < _accumulationBuffer.width(); i += blockWidth) {
                pool->assignWork([&, i, j]() {
                        accumulateBlock(scene, numSamples, i, j, blockWidth);
                    });
            }
        }
        pool->wait();
    } else {
        accumulateBlock(
            scene, numSamples, 0, 0,
            std::max(_accumulationBuffer.width(), _accumulationBuffer.height()));
    }

    _numSamplesPerPixel += numSamples;
}

void PathTracer::updateFrameBuffer(Image& frameBuffer) const {
    for (int y = 0; y < frameBuffer.height(); ++y) {
        for (int x = 0; x < frameBuffer.width(); ++x) {
            frameBuffer.rgb(x, y) = Image::applyCorrection(
                _accumulationBuffer.rgb(x, y) * (1.0f / _numSamplesPerPixel));
        }
    }
}

void PathTracer::accumulateBlock(
        const Scene& scene, int numSamples,
        std::size_t x, std::size_t y, std::size_t blockWidth) {
    ZoneScoped;
    for (int j = y; j < std::min(_accumulationBuffer.height(), y + blockWidth); ++j) {
        for (int i = x; i < std::min(_accumulationBuffer.width(), x + blockWidth); ++i) {
            Vec3 radiance(0.0f);
            for (int k = 0; k < numSamples; ++k) {
                if(_isStopping) return;
                const Ray ray = scene.eyeRay(Vec2(i + PCG32::rand(), j + PCG32::rand()));
                const auto eyePath = Path::createRandomEyePath(scene, ray);
                const auto lightPath = Path::createRandomLightPath(scene);
                
                Vec3 throughput(1.0f);
                for (std::size_t i = 1;i < eyePath.length(); ++i) {
                    const Path::Vertex& prevVertex = eyePath.vertex(i-1);
                    const Path::Vertex& vertex = eyePath.vertex(i);

                    if (i < eyePath.length() - 1 ) {
                        const Path::Vertex& nextVertex = eyePath.vertex(i+1);
                        EvaluationResult implicitEvaluation =
                            evaluateImplicit(scene, prevVertex, vertex, nextVertex);
                        throughput *= implicitEvaluation.russianRouletteRadiance;
                    }
                    
                    if (vertex.bounceType == Path::Vertex::BounceType::Diffuse &&
                        lightPath.length() > 0) {
                        radiance += 0.5f * throughput * evaluateExplicitLight(
                            scene, prevVertex, vertex, lightPath.vertex(0));
                    }
                    
                    const Material& material = scene.getMaterial(vertex.materialIdx);
                    radiance += 0.5f * throughput * material.emission(vertex);
                }

            }
            _accumulationBuffer.rgb(i, j) += radiance;
        }
    }
}

void PathTracer::reset() {
    IRenderer::reset();
    _accumulationBuffer.clear();
    _numSamplesPerPixel = 0;
}
