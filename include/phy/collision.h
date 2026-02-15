#ifndef __COLLISION_RB_H__
#define __COLLISION_RB_H__

#include "vec2.h"

namespace phy {
    
    struct collisionInfo {
        vec2 vertex, intersection, edge, normal, rp1, rp2;

        vec2 getDir() const {
            return intersection - vertex;
        }

        float length() const {
            return getDir().length();
        }
    };

    struct collision {

        static bool lineToLineIntersect(const vec2& l1, const vec2& l2, const vec2& l3, const vec2& l4, collisionInfo& info)
        {
            float denom = (l1.x - l2.x) * (l3.y - l4.y) - (l1.y - l2.y) * (l3.x - l4.x);
            float t = ((l1.x - l3.x) * (l3.y - l4.y) - (l1.y - l3.y) * (l3.x - l4.x)) / denom;
            float u = -((l1.x - l2.x) * (l1.y - l3.y) - (l1.y - l2.y) * (l1.x - l3.x)) / denom;

            if(t >= 0 && t <= 1 && u >= 0 && u <= 1) {
                info.vertex = l2;
                info.intersection.x = l1.x + t * (l2.x - l1.x);
                info.intersection.y = l1.y + t * (l2.y - l1.y);
                info.edge = (l4 - l3).normalize();
                info.normal = info.edge.perp(1);
                return true;
            }

            return false;
        }

    };

    

}

#endif 