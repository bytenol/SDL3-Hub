#ifndef __PHY_GEOMETRY_H__
#define __PHY_GEOMETRY_H__

#include "vec2.h"

namespace phy {

    template<typename T>
    concept RectangularObjectConcept = requires(T t) 
    {
        t.pos;
        t.size;
    };

    struct Point2D {
        float x = 0, y = 0;
    };


    struct Rect2D {
        vec3 pos, size;

        explicit Rect2D(const vec3& p = { 0, 0 }, const vec3& s = { 0, 0 }): pos(p), size(s) {}

        inline float getArea() {
            return size.x * size.y;
        }
    };

}

#endif 