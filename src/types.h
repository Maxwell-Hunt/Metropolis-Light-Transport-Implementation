// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#pragma once

#include <cstdint>
#include <format>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/norm.hpp"

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;

using Mat4 = glm::mat4;

using Quat = glm::quat;

using glm::cross;
using glm::dot;
using glm::normalize;
using glm::length;
using glm::length2;

template<class... Ts>
struct Visitor : Ts... { using Ts::operator()...; };

template<>
struct std::formatter<Vec2> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const Vec2& v, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", v.x, v.y);
    }
};

template<>
struct std::formatter<Vec3> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const Vec3& v, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", v.x, v.y, v.z);
    }
};

template<>
struct std::formatter<Vec4> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const Vec4& v, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {}, {})", v.x, v.y, v.z, v.w);
    }
};
