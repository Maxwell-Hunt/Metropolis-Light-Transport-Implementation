// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#pragma once

#include <optional>

#include "types.h"
#include "ray.h"

/// Axis-aligned bounding box.
class AABB {
public:
    float getMin(int axis) const { return _min[axis]; };
    float getMax(int axis) const { return _max[axis]; };
    float getSize(int axis) const { return _max[axis] - _min[axis]; };

    Vec3 getMin() const { return _min; };
    Vec3 getMax() const { return _max; };
    Vec3 getSize() const { return _max - _min; };

    int getLargestAxis() const;

    void fit(Vec3 v);

    float halfArea() const {
        const Vec3 size = getSize();
        return size.x * (size.y + size.z) + size.y * size.z;
    }

    float area() const { return 2.0f * halfArea(); }

    /// Tests intersection of a ray against the bounding box.
    /// @returns
    ///     The distance to the intersection point if there is an intersection,
    ///     `std::nullopt` otherwise.
    std::optional<float> intersect(const Ray& ray) const;

private:
    static constexpr float Infinity = std::numeric_limits<float>::infinity();

    Vec3 _min{Infinity};
    Vec3 _max{-Infinity};
};


