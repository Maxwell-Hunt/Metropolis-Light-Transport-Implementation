#pragma once

#include "types.h"

struct Ray {
    Vec3 o, d;
    Ray() : o(), d(0.0f, 0.0f, 1.0f) {}
    Ray(Vec3 o, Vec3 d) : o(o), d(d) {}
};
