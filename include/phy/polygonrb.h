#ifndef __PHY_POLYGON_RB__
#define __PHY_POLYGON_RB__

#include <vector>
#include "vec2.h"

namespace phy {

    enum class RbShapeType
    {
        DEFAULT,
        POLYGON,
        CIRCLE
    };

    struct RigidShape
    { 
        phy::vec2 pos, vel, acc, force;
        float mass = 1;
        float im = 1;
        float angVelo = 0;
        float torque = 0;
        float theta = 0;
        RbShapeType type = RbShapeType::DEFAULT;  
        RigidShape() = default;
        virtual ~RigidShape() = default;
    };

    struct PolygonRb: public RigidShape {
        std::vector<phy::vec2> vertices;
        std::vector<unsigned int> indices;

        struct { float r, g, b; } color;

        void setRotation(const float& angle) {
            theta = angle;
            for(auto& vert: vertices) {
                vert = vert.rotate(angle);
            }
        } 

        float getRotation() const  {
            return theta;
        }
    };


    struct CircleRb: public RigidShape
    {
        float radius;
        CircleRb(const float& r): RigidShape()
        {
            radius = r;
            type = RbShapeType::CIRCLE;
        }
    };


}

#endif 