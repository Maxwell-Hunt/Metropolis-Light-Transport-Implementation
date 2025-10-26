#include "bvh.h"

#include "tracy/Tracy.hpp"

#include "aabb.h"
#include "aabb4.h"
#include "mesh.h"
#include "types.h"

namespace {

struct SplitInfo {
    int axis;
    float position;
    AABB leftBBox, rightBBox;
    std::size_t numLeft, numRight;
    float leftCost, rightCost;
};

SplitInfo evaluateSplit(
        std::span<const BVH::Triangle> triangles,
        std::span<const Vec3> triangleCenters,
        int axis,
        float splitPosition) {
    SplitInfo info{axis, splitPosition};
    for (std::size_t i = 0; i < triangles.size(); ++i) {
        if (triangleCenters[i][axis] < splitPosition) {
            // grow left by triangle
            for (Vec3 position : triangles[i].positions)
                info.leftBBox.fit(position);
            ++info.numLeft;
        } else {
            // grow right by triangle
            for (Vec3 position : triangles[i].positions)
                info.rightBBox.fit(position);
        }
    }
    info.numRight = triangles.size() - info.numLeft;
    info.leftCost = info.numLeft * info.leftBBox.halfArea();
    info.rightCost = info.numRight * info.rightBBox.halfArea();
    return info;
}

std::optional<BVH::HitInfo> doesRayIntersectTriangle(
        const Ray& ray, const BVH::Triangle& triangle,
        float minDistance, float maxDistance) {
    constexpr float Epsilon = 5e-7f;
    const Vec3 ab = triangle.positions[0] - triangle.positions[1];
    const Vec3 ac = triangle.positions[0] - triangle.positions[2];
    const Vec3 ao = triangle.positions[0] - ray.o;
    const Vec3 geometricNormal = cross(ab, ac);
    const float determinant = dot(geometricNormal, ray.d);

    if (std::abs(determinant) < Epsilon)
        return std::nullopt; // The ray is parallel to the triangle.

    const float invDeterminant = 1 / determinant;

    const float beta = dot(cross(ao, ac), ray.d) * invDeterminant;
    if (beta < 0 || beta > 1)
        return std::nullopt;

    const float gamma = dot(cross(ab, ao), ray.d) * invDeterminant;
    if (gamma < 0 || beta + gamma > 1)
        return std::nullopt;

    const float alpha = 1 - beta - gamma;

    const float t = dot(geometricNormal, ao) * invDeterminant;
    if (t < minDistance || t > maxDistance)
        return std::nullopt;

    return BVH::HitInfo {
        triangle.idx, t, ray.o + ray.d * t, {alpha, beta, gamma}};
}

/// Try to find an optimal two-way split using SAH.
template<int NumSplits, typename GetBoundsSizeFn, typename GetBoundsMinFn>
std::optional<SplitInfo> trySplitAndPartition(
        const GetBoundsSizeFn getBoundsSize,
        const GetBoundsMinFn getBoundsMin,
        const std::uint32_t firstTriangleIdx,
        const std::uint32_t numTriangles,
        const std::span<BVH::Triangle> triangles,
        const std::span<Vec3> triangleCenters,
        float bestCost) {
    const auto firstTriangleInBounds = triangles.begin() + firstTriangleIdx;
    const auto lastTriangleInBounds = firstTriangleInBounds + numTriangles;
    const std::span<BVH::Triangle> trianglesInBounds(firstTriangleInBounds, lastTriangleInBounds);

    const auto firstTriangleCenterInBounds = triangleCenters.begin() + firstTriangleIdx;
    const auto lastTriangleCenterInBounds = firstTriangleCenterInBounds + numTriangles;
    const std::span<Vec3> triangleCentersInBounds(firstTriangleCenterInBounds, lastTriangleCenterInBounds);

    std::optional<SplitInfo> bestSplit;
    for (int axis = 0; axis < 3; ++axis) {
        const float splitSeparation = getBoundsSize(axis) / (NumSplits + 1);
        for (int split = 0; split < NumSplits; ++split) {
            const float splitPosition = getBoundsMin(axis) + (split + 1) * splitSeparation;
            const SplitInfo splitInfo = evaluateSplit(
                trianglesInBounds, triangleCentersInBounds, axis, splitPosition);
            const float cost = splitInfo.leftCost + splitInfo.rightCost;
            if (cost < bestCost) {
                bestCost = cost;
                bestSplit = splitInfo;
            }
        }
    }

    if (!bestSplit)
        return std::nullopt; // We did not find a split that beats `bestCost`.

    // We found an optimal split, now partition the triangles.
    std::size_t numLeft = 0;
    for (std::size_t i = 0; i < trianglesInBounds.size(); ++i) {
        if (triangleCentersInBounds[i][bestSplit->axis] < bestSplit->position) {
            std::swap(trianglesInBounds[i], trianglesInBounds[numLeft]);
            std::swap(triangleCentersInBounds[i], triangleCentersInBounds[numLeft]);
            ++numLeft;
        }
    }

    return bestSplit;
}

class TraversalStack {
public:
    struct StackInfo {
        std::uint32_t idx;
        float distance;
    };

    void push(StackInfo info) {
        if(_size < _data.size()) {
            _data[_size] = info;
        } else {
            _data.push_back(info);
        }
        
        ++_size;
    }

    StackInfo top() { return _data[_size-1]; }
    bool empty() { return _size == 0; }
    void pop() { --_size; };

private:
    std::vector<StackInfo> _data;
    std::size_t _size = 0;
};

} // namespace


BVH::BVH(const Mesh& mesh, const std::size_t startIdx, const std::size_t count) {
    ZoneScopedN("Building BVH");
    std::vector<Vec3> triangleCenters;
    triangles.reserve(count);
    triangleCenters.reserve(count);
    Node& rootNode = nodes.emplace_back(AABB4(), 0, count);
    for (std::size_t i = startIdx; i < startIdx + count; ++i) {
        Triangle& triangle = triangles.emplace_back(mesh.triangles[i].positions, i);
        triangleCenters.push_back(triangle.center());
        for (Vec3 position : triangle.positions)
            rootBounds.fit(position);
    }
    split(
        std::nullopt, 0, rootNode.numTriangles * rootBounds.halfArea(),
        triangleCenters);
}

std::optional<BVH::HitInfo> BVH::intersect(
        const Ray& ray,
        float minDistance,
        float maxDistance) const {
    constexpr std::uint32_t rootNodeIdx = 0;
    std::optional<float> rootIntersection = rootBounds.intersect(ray);
    if (!rootIntersection)
        return std::nullopt;

    std::optional<HitInfo> closestHit;
    thread_local TraversalStack stack;
    stack.push(TraversalStack::StackInfo(rootNodeIdx, *rootIntersection));
    while (!stack.empty()) {
        const auto [index, dist] = stack.top();
        stack.pop();
        if(closestHit && closestHit->distance < dist)
            continue;
        const Node& node = nodes[index];
        if (node.isLeaf()) {
            for (std::uint32_t i = node.idx; i < node.idx + node.numTriangles; ++i) {
                std::optional<HitInfo> hitInfo = doesRayIntersectTriangle(
                    ray, triangles[i], minDistance, maxDistance);
                if (hitInfo && (!closestHit || hitInfo->distance < closestHit->distance))
                    closestHit = hitInfo;
            }
        } else {
            AABB4::HitInfo hitInfo = node.childBounds.intersect(ray);
            for (int i = 0; i < 4; ++i) {
                std::optional<int> bestIdx;
                float bestDist = std::numeric_limits<float>::infinity();
                // Find the next closest hit
                for (int i = 0; i < 4; ++i) {
                    if (hitInfo.isHit[i] && hitInfo.distances[i] < bestDist) {
                        bestDist = hitInfo.distances[i];
                        bestIdx = i;
                    }
                }
                if (!bestIdx)
                    break; // No more hits
                const std::uint32_t childIdx = node.idx + *bestIdx;
                stack.push({childIdx, bestDist});
                // Mark as used so we don't push it again
                hitInfo.isHit[*bestIdx] = false;
            }
        }
    }

    return closestHit;
}

void BVH::split(
        std::optional<std::uint32_t> parentNodeIdx, int childIdx,
         float nodeCost, std::span<Vec3> triangleCenters) {
    std::uint32_t nodeIdx =
        parentNodeIdx
            ? nodes[*parentNodeIdx].idx + childIdx
            : 0; // Special case: root node has no parent node.
    if (nodes[nodeIdx].numTriangles <= MaxNumTrianglesInLeaf)
        return; // The leaf node is small enough to not warrant splitting.

    // First, we create an initial split, splitting into 2 regions.
    std::optional<SplitInfo> bestInitialSplit;
    if (parentNodeIdx) {
        bestInitialSplit = trySplitAndPartition<NumSplits>(
            [&](int axis) { return nodes[*parentNodeIdx].childBounds.getSize(childIdx, axis); },
            [&](int axis) { return nodes[*parentNodeIdx].childBounds.getMin(childIdx, axis); },
            nodes[nodeIdx].idx, nodes[nodeIdx].numTriangles, triangles, triangleCenters, nodeCost);
    } else {
        bestInitialSplit = trySplitAndPartition<NumSplits>(
            [&](int axis) { return rootBounds.getSize(axis); },
            [&](int axis) { return rootBounds.getMin(axis); },
            nodes[nodeIdx].idx, nodes[nodeIdx].numTriangles, triangles, triangleCenters, nodeCost);
    }
    if (!bestInitialSplit)
        return;

    // Now, for the left and right bounds created by the initial split, we try
    // split them again to get a total of 4 child regions.

    std::optional<SplitInfo> bestLeftSplit = trySplitAndPartition<NumSplits>(
        [&](int axis) { return bestInitialSplit->leftBBox.getSize(axis); },
        [&](int axis) { return bestInitialSplit->leftBBox.getMin(axis); },
        nodes[nodeIdx].idx, bestInitialSplit->numLeft, triangles, triangleCenters, nodeCost);
    if (!bestLeftSplit)
        return;

    std::optional<SplitInfo> bestRightSplit = trySplitAndPartition<NumSplits>(
        [&](int axis) { return bestInitialSplit->rightBBox.getSize(axis); },
        [&](int axis) { return bestInitialSplit->rightBBox.getMin(axis); },
        nodes[nodeIdx].idx + bestInitialSplit->numLeft,
        bestInitialSplit->numRight, triangles, triangleCenters, nodeCost);
    if (!bestRightSplit)
        return;

    const float totalCost =
        bestLeftSplit->leftCost + bestLeftSplit->rightCost +
        bestRightSplit->leftCost + bestRightSplit->rightCost;

    if (totalCost > nodeCost)
        // Unfortunately, all of this work was all for nothing; despite the
        // individual splits looking good, the 4-way split is not worth it.
        return;

    // We must use indices as we are still writing to the vector.
    const std::uint32_t firstChildIdx = nodes.size();

    // Construct child nodes...
    nodes[nodeIdx].childBounds = AABB4(
        bestLeftSplit->leftBBox, bestLeftSplit->rightBBox,
        bestRightSplit->leftBBox, bestRightSplit->rightBBox);

    std::uint32_t trianglesIdx = nodes[nodeIdx].idx;
    std::uint32_t numTriangles = bestLeftSplit->numLeft;
    nodes.emplace_back(Node{.idx = trianglesIdx, .numTriangles = numTriangles});

    trianglesIdx += numTriangles;
    numTriangles = bestLeftSplit->numRight;
    nodes.emplace_back(Node{.idx = trianglesIdx, .numTriangles = numTriangles});

    trianglesIdx += numTriangles;
    numTriangles = bestRightSplit->numLeft;
    nodes.emplace_back(Node{.idx = trianglesIdx, .numTriangles = numTriangles});

    trianglesIdx += numTriangles;
    numTriangles = bestRightSplit->numRight;
    nodes.emplace_back(Node{.idx = trianglesIdx, .numTriangles = numTriangles});

    // Mark this node as internal and update index.
    nodes[nodeIdx].numTriangles = 0;
    nodes[nodeIdx].idx = firstChildIdx;

    split(nodeIdx, 0, bestLeftSplit->leftCost,   triangleCenters);
    split(nodeIdx, 1, bestLeftSplit->rightCost,  triangleCenters);
    split(nodeIdx, 2, bestRightSplit->leftCost,  triangleCenters);
    split(nodeIdx, 3, bestRightSplit->rightCost, triangleCenters);
}

Vec3 BVH::Triangle::center() const {
    return (positions[0] + positions[1] + positions[2]) / 3.0f;
}
