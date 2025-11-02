// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#include "aabb.h"

int AABB::getLargestAxis() const {
    const Vec3 size = getSize();
    if ((size.x > size.y) && (size.x > size.z)) {
        return 0;
    } else if (size.y > size.z) {
        return 1;
    } else {
        return 2;
    }
}

void AABB::fit(const Vec3 v) {
    if (_min.x > v.x) _min.x = v.x;
    if (_min.y > v.y) _min.y = v.y;
    if (_min.z > v.z) _min.z = v.z;

    if (_max.x < v.x) _max.x = v.x;
    if (_max.y < v.y) _max.y = v.y;
    if (_max.z < v.z) _max.z = v.z;
}

std::optional<float> AABB::intersect(const Ray& ray) const {
    float tx1 = (_min.x - ray.o.x) / ray.d.x;
    float ty1 = (_min.y - ray.o.y) / ray.d.y;
    float tz1 = (_min.z - ray.o.z) / ray.d.z;

    float tx2 = (_max.x - ray.o.x) / ray.d.x;
    float ty2 = (_max.y - ray.o.y) / ray.d.y;
    float tz2 = (_max.z - ray.o.z) / ray.d.z;

    if (tx1 > tx2) {
        const float temp = tx1;
        tx1 = tx2;
        tx2 = temp;
    }

    if (ty1 > ty2) {
        const float temp = ty1;
        ty1 = ty2;
        ty2 = temp;
    }

    if (tz1 > tz2) {
        const float temp = tz1;
        tz1 = tz2;
        tz2 = temp;
    }

    float t1 = tx1; if (t1 < ty1) t1 = ty1; if (t1 < tz1) t1 = tz1;
    float t2 = tx2; if (t2 > ty2) t2 = ty2; if (t2 > tz2) t2 = tz2;

    if (t1 > t2) return std::nullopt;
    if ((t1 < 0.0) && (t2 < 0.0)) return std::nullopt;

    return t1;
}
