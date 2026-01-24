#ifndef __PHY_GEOMETRY_H__
#define __PHY_GEOMETRY_H__

#include "vec2.h"

namespace phy {

    struct Point2D {
        float x = 0, y = 0;
    };


    struct Rect2D {
        vec2 pos, size;

        explicit Rect2D(const vec2& p = { 0, 0 }, const vec2& s = { 0, 0 }): pos(p), size(s) {}

        inline float getArea() {
            return size.x * size.y;
        }

        constexpr bool containsPoint(const Point2D& other) {
            return false;
        }

        constexpr bool intersectRect(const Rect2D& other) {
            return false;
        }
    };

}

#endif 