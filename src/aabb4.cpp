// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#include "aabb4.h"

#include <algorithm>

#include "types.h"

float AABB4::getMin(int idx, int axis) const {
    switch (axis) {
    case 0: return _minX[idx];
    case 1: return _minY[idx];
    case 2: return _minZ[idx];
    default: return 0.0f;
    }
}

float AABB4::getMax(int idx, int axis) const {
    switch (axis) {
    case 0: return _maxX[idx];
    case 1: return _maxY[idx];
    case 2: return _maxZ[idx];
    default: return 0.0f;
    }
}

float AABB4::getSize(int idx, int axis) const {
    return getMax(idx, axis) - getMin(idx, axis);
}

Vec3 AABB4::getMin(int idx) const {
    return Vec3(_minX[idx], _minY[idx], _minZ[idx]);
}

Vec3 AABB4::getMax(int idx) const {
    return Vec3(_maxX[idx], _maxY[idx], _maxZ[idx]);
}

Vec3 AABB4::getSize(int idx) const {
    return getMax(idx) - getMin(idx);
}

void AABB4::fit(int idx, const Vec3& point) {
    _minX[idx] = std::min(_minX[idx], point.x);
    _minY[idx] = std::min(_minY[idx], point.y);
    _minZ[idx] = std::min(_minZ[idx], point.z);
    _maxX[idx] = std::max(_maxX[idx], point.x);
    _maxY[idx] = std::max(_maxY[idx], point.y);
    _maxZ[idx] = std::max(_maxZ[idx], point.z);
}

AABB4::AABB4(const AABB& a, const AABB& b, const AABB& c, const AABB& d) {
    _minX = Vec4(a.getMin().x, b.getMin().x, c.getMin().x, d.getMin().x);
    _minY = Vec4(a.getMin().y, b.getMin().y, c.getMin().y, d.getMin().y);
    _minZ = Vec4(a.getMin().z, b.getMin().z, c.getMin().z, d.getMin().z);
    _maxX = Vec4(a.getMax().x, b.getMax().x, c.getMax().x, d.getMax().x);
    _maxY = Vec4(a.getMax().y, b.getMax().y, c.getMax().y, d.getMax().y);
    _maxZ = Vec4(a.getMax().z, b.getMax().z, c.getMax().z, d.getMax().z);
}

AABB4::HitInfo AABB4::intersect(const Ray& ray) const {
    const Vec4 tminX = (_minX - ray.o.x) / ray.d.x;
    const Vec4 tmaxX = (_maxX - ray.o.x) / ray.d.x;
    const Vec4 tminY = (_minY - ray.o.y) / ray.d.y;
    const Vec4 tmaxY = (_maxY - ray.o.y) / ray.d.y;
    const Vec4 tminZ = (_minZ - ray.o.z) / ray.d.z;
    const Vec4 tmaxZ = (_maxZ - ray.o.z) / ray.d.z;

    const Vec4 tx1 = glm::min(tminX, tmaxX);
    const Vec4 tx2 = glm::max(tminX, tmaxX);
    const Vec4 ty1 = glm::min(tminY, tmaxY);
    const Vec4 ty2 = glm::max(tminY, tmaxY);
    const Vec4 tz1 = glm::min(tminZ, tmaxZ);
    const Vec4 tz2 = glm::max(tminZ, tmaxZ);

    const Vec4 t1 = glm::max(tx1, glm::max(ty1, tz1));
    const Vec4 t2 = glm::min(tx2, glm::min(ty2, tz2));

    const glm::bvec4 isHit =
        glm::lessThanEqual(t1, t2) &
        glm::not_(
            glm::lessThan(t1, Vec4(0.0f)) &
            glm::lessThan(t2, Vec4(0.0f)));

    return {isHit, t1};
}


