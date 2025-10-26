#include "mesh.h"
#include "image.h"

void Mesh::addPrimitive(
        std::size_t startIdx, std::size_t count,
        std::optional<std::size_t> materialIdx) {
    primitives.emplace_back(
        startIdx, count, materialIdx, BVH(*this, startIdx, count));
}

float Mesh::Triangle::computeArea() const {
    const Vec3 edge1 = positions[1] - positions[0];
    const Vec3 edge2 = positions[2] - positions[0];
    return length(cross(edge1, edge2));
}
