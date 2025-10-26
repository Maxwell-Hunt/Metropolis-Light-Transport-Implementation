#pragma once

#include <string>
#include <optional>

#include "math.h"
#include "path.h"
#include "types.h"
#include "random.h"


struct MaterialData {
    std::string name;

    Vec4 baseColorFactor = Vec4(1.0f);
    std::optional<std::size_t> baseColorTextureIdx;

    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    std::optional<std::size_t> metallicRoughnessTextureIdx;

    Vec3 emissiveFactor = Vec3(0.0f);
    float emissiveStrength = 1.0f;
    std::optional<std::size_t> emissiveTextureIdx;

    float transmissionFactor = 0.0f;
    std::optional<std::size_t> transmissionTextureIdx;

    float ior = 1.5f;

    Path::Vertex::BounceType getType() const {
        if (transmissionFactor > 0.5f && metallicFactor < 0.5f)
            return Path::Vertex::BounceType::Refractive;
        if (metallicFactor > 0.5f && roughnessFactor < 0.5f)
            return Path::Vertex::BounceType::Reflective;
        return Path::Vertex::BounceType::Diffuse;
    }    
};

class Material {
public:
    Material(const Scene& scene, const MaterialData& data)
        : _scene(scene), _data(data) {}
    Vec3 bsdf(const Path::Vertex& vertex) const;
    Vec3 expectedContribution(const Path::Vertex& vertex, const Vec3& inDir) const;
    Vec3 emission(const Path::Vertex& vertex) const;
    Path::Vertex::BounceType getType() const { return _data.getType(); }

    // inRay is meant to point away from the surface normal
    std::pair<Ray, Path::Vertex::BounceType> sampleDirection(
        Vec3 inDir, const Path::Vertex& vertex) const;
private:
    const Scene& _scene;
    const MaterialData& _data;
};

