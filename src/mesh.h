// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <random>

#include "bvh.h"
#include "image.h"
#include "types.h"
#include "random.h"

struct Mesh {
    struct Triangle {
        std::array<Vec3, 3> positions;
        std::array<Vec3, 3> normals;
        std::array<Vec2, 3> textureCoords;

        float computeArea() const;
    };

    struct Primitive {
        std::size_t startIdx;
        std::size_t count;
        std::optional<std::size_t> materialIdx;
        BVH bvh;
        float totalArea;
    };

    std::string name;
    std::vector<Triangle> triangles;
    std::vector<Primitive> primitives;
    std::vector<float> triangleAreas;
    /// Distribution over the triangles in a primitive weighted by area
    mutable std::vector<std::discrete_distribution<PCG32::Generator::result_type>>
        primitiveTriangleDistibutions;

    void addPrimitive(
        std::size_t startIdx, std::size_t count,
        std::optional<std::size_t> materialIdx);
};
