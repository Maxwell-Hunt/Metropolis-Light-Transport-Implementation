// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#pragma once

#include <optional>
#include <limits>

#include "aabb.h"
#include "ray.h"
#include "types.h"

/// Four axis-aligned bounding boxes. Each box is represented by its min and
/// max points in 3D. For vectorization, we store the x, y, z components of
/// each box in `Vec4`, where each component is a different box.
class AABB4 {
public:
    AABB4() = default;

    /// Construct from 4 AABBs.
    AABB4(const AABB& a, const AABB& b, const AABB& c, const AABB& d);

    float getMin(int idx, int axis) const;
    float getMax(int idx, int axis) const;
    float getSize(int idx, int axis) const;

    Vec3 getMin(int idx) const;
    Vec3 getMax(int idx) const;
    Vec3 getSize(int idx) const;

    void fit(int idx, const Vec3& point);

    float halfArea(int idx) const {
        const Vec3 size = getSize(idx);
        return size.x * (size.y + size.z) + size.y * size.z;
    }

    float area(int idx) const { return 2.0f * halfArea(idx); }

    struct HitInfo {
        glm::bvec4 isHit;
        Vec4 distances;
    };

    /// Tests intersection of a ray against all 4 bounding boxes.
    HitInfo intersect(const Ray& ray) const;

private:
    static constexpr float Infinity = std::numeric_limits<float>::infinity();

    Vec4 _minX{Infinity};
    Vec4 _minY{Infinity};
    Vec4 _minZ{Infinity};

    Vec4 _maxX{-Infinity};
    Vec4 _maxY{-Infinity};
    Vec4 _maxZ{-Infinity};
};
