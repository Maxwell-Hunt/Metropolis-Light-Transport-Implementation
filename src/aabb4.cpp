// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#include "aabb4.h"

#include <algorithm>
#include <immintrin.h>

#include "types.h"

namespace {

__m128 loadVec4(const Vec4& value) {
    return _mm_setr_ps(value.x, value.y, value.z, value.w);
}

Vec4 storeVec4(__m128 value) {
    alignas(16) float components[4];
    _mm_storeu_ps(components, value);
    return Vec4(components[0], components[1], components[2], components[3]);
}

glm::bvec4 storeMask(__m128 mask) {
    const int bits = _mm_movemask_ps(mask);
    return glm::bvec4(
        (bits & 0x1) != 0,
        (bits & 0x2) != 0,
        (bits & 0x4) != 0,
        (bits & 0x8) != 0);
}

}

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
    const __m128 minX = loadVec4(_minX);
    const __m128 minY = loadVec4(_minY);
    const __m128 minZ = loadVec4(_minZ);
    const __m128 maxX = loadVec4(_maxX);
    const __m128 maxY = loadVec4(_maxY);
    const __m128 maxZ = loadVec4(_maxZ);

    const __m128 originX = _mm_set1_ps(ray.o.x);
    const __m128 originY = _mm_set1_ps(ray.o.y);
    const __m128 originZ = _mm_set1_ps(ray.o.z);
    const __m128 directionX = _mm_set1_ps(ray.d.x);
    const __m128 directionY = _mm_set1_ps(ray.d.y);
    const __m128 directionZ = _mm_set1_ps(ray.d.z);

    const __m128 tminX = _mm_div_ps(_mm_sub_ps(minX, originX), directionX);
    const __m128 tmaxX = _mm_div_ps(_mm_sub_ps(maxX, originX), directionX);
    const __m128 tminY = _mm_div_ps(_mm_sub_ps(minY, originY), directionY);
    const __m128 tmaxY = _mm_div_ps(_mm_sub_ps(maxY, originY), directionY);
    const __m128 tminZ = _mm_div_ps(_mm_sub_ps(minZ, originZ), directionZ);
    const __m128 tmaxZ = _mm_div_ps(_mm_sub_ps(maxZ, originZ), directionZ);

    const __m128 tx1 = _mm_min_ps(tminX, tmaxX);
    const __m128 tx2 = _mm_max_ps(tminX, tmaxX);
    const __m128 ty1 = _mm_min_ps(tminY, tmaxY);
    const __m128 ty2 = _mm_max_ps(tminY, tmaxY);
    const __m128 tz1 = _mm_min_ps(tminZ, tmaxZ);
    const __m128 tz2 = _mm_max_ps(tminZ, tmaxZ);

    const __m128 t1 = _mm_max_ps(tx1, _mm_max_ps(ty1, tz1));
    const __m128 t2 = _mm_min_ps(tx2, _mm_min_ps(ty2, tz2));

    const __m128 zero = _mm_setzero_ps();
    const __m128 isHit = _mm_andnot_ps(
        _mm_and_ps(_mm_cmplt_ps(t1, zero), _mm_cmplt_ps(t2, zero)),
        _mm_cmple_ps(t1, t2));

    return {storeMask(isHit), storeVec4(t1)};
}


