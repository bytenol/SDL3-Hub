#ifndef __PHY_POLYGON_RB__
#define __PHY_POLYGON_RB__

#include <vector>
#include <cmath>
#include <memory>
#include "vec2.h"
#include "./geometry.h"

namespace phy {

    class RigidShape;
    using rigid_t = std::unique_ptr<RigidShape>;

    enum class RbShapeType
    {
        DEFAULT,
        POLYGON,
        CIRCLE
    };

    struct RigidShape
    { 
        phy::vec3 pos, vel, acc, force;
        float mass = 1;
        float im = 1;
        float angVelo = 0;
        float torque = 0;
        float angularAcc = 0.0f;
        std::vector<phy::vec3> vertices;
        struct { float r, g, b; } color;
        float theta = 0;
        bool isStatic = false;

        RbShapeType type = RbShapeType::DEFAULT;  
        RigidShape() = default;
        
        virtual phy::vec3 findSupportPoint(const phy::vec3& dir) 
        {
            float maxDot = -INFINITY;
			phy::vec3 best;

			for(auto v: vertices) {
				auto vp = pos + v.rotate(getRotation());
				auto dp = vp.dotProduct(dir);
				if(dp > maxDot) {
					maxDot = dp;
					best = vp;
				}
			}

			return best;
        }

        virtual ~RigidShape() = default;

        void setRotation(const float& angle) {
            theta = angle;
        } 

        float getRotation() const  {
            return theta * 3.14159f / 180.0f;
        }

    private:
    };


    struct PolygonRb: public RigidShape {

        PolygonRb()
        {
            type = RbShapeType::POLYGON;
        }

        Rect2D getBoundary()
        {
            float minX = INFINITY, maxX = -INFINITY;
            float minY = INFINITY, maxY = -INFINITY;
            for(int i = 0; i < vertices.size(); i++)
            {
                auto rotation = getRotation();
                auto p = pos + vertices[i].rotate(rotation);
                minX = std::min(minX, p.x);
                maxX = std::max(maxX, p.x);
                minY = std::min(minY, p.y);
                maxY = std::max(maxY, p.y);
            }

            return Rect2D{ { minX, minY }, { maxX - minX, maxY - minY } };
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

        phy::vec3 findSupportPoint(const phy::vec3& dir) override
        {
            return pos + dir.normalize() * radius;
        }

    };

    struct CollisionInfo {
        phy::vec3 start, end, normal;
        float depth;
    };


    bool satCollision(PolygonRb* poly1, PolygonRb* poly2, CollisionInfo& info)
    {
        PolygonRb* polygon1 = poly1;
        PolygonRb* polygon2 = poly2;

        float depth = INFINITY;
        PolygonRb* refPolygon = polygon1;
        PolygonRb* incPolygon = polygon2;

        vec3 axisOfColl, refV1, refV2, incV1, incV2;
    

        for(int i = 0; i < 2; i++) {
            if(i > 0) {
                polygon1 = poly2;
                polygon2 = poly1;
            }

            float rotation1 = polygon1->getRotation();
            float rotation2 = polygon2->getRotation();

            for(int i = 0; i < polygon1->vertices.size(); i++)
            {
                auto p1 = polygon1->pos + polygon1->vertices[i].rotate(rotation1);
                auto p2 = polygon1->pos + polygon1->vertices[(i + 1) % polygon1->vertices.size()].rotate(rotation1);
                auto edge = (p2 - p1);
                auto normal = edge.perp(1).normalize();

                // for shape A
                float minA = INFINITY, maxA = -INFINITY;
                for(int i = 0; i < polygon1->vertices.size(); i++)
                {
                    auto pos = polygon1->pos + polygon1->vertices[i].rotate(rotation1);
                    auto dp = pos.dotProduct(normal);
                    minA = std::min(dp, minA);
                    maxA = std::max(dp, maxA);
                }

                // for shape B
                float minB = INFINITY, maxB = -INFINITY;
                for(int i = 0; i < polygon2->vertices.size(); i++)
                {
                    auto pos = polygon2->pos + polygon2->vertices[i].rotate(rotation2);
                    auto dp = pos.dotProduct(normal);
                    minB = std::min(dp, minB);
                    maxB = std::max(dp, maxB);
                }

                bool isColliding = maxA >= minB && maxB >= minA;
                if(!isColliding) {
                    return false;
                }

                float overlap = std::min(maxA, maxB) - std::max(minA, minB);
                
                if(overlap < depth) {
                    depth = overlap;
                    refPolygon = polygon1;
                    incPolygon = polygon2;
                    axisOfColl = normal;

                    refV1 = p1;
                    refV2 = p2;
                }
            }
        }

        vec3 refNormal = (refV2 - refV1).perp(1).normalize();
        if(refNormal.dotProduct(axisOfColl) < 0){
             refNormal *= -1;
             std::cout << "Normal flipped" << std::endl;
        }

        // find the incident edge
        float incNormal = INFINITY;
        auto rotation = incPolygon->getRotation();
        for(int i = 0; i < incPolygon->vertices.size(); i++)
        {
            auto p1 = incPolygon->pos + incPolygon->vertices[i].rotate(rotation);
            auto p2 = incPolygon->pos + incPolygon->vertices[(i + 1) % incPolygon->vertices.size()].rotate(rotation);
            auto edge = p2 - p1;
            auto normal = edge.perp(1).normalize();
            auto dp = normal.dotProduct(axisOfColl);
            if(dp < incNormal) {
                incNormal = dp;
                incV1 = p1;
                incV2 = p2;
            }
        }

        auto clipEdgeToPlane = [](vec3 in1, vec3 in2,
            vec3 planeNormal, float planeOffset, vec3& out1, vec3& out2)
        {
            vec3 out[2];
            int count = 0;

            float d1 = in1.dotProduct(planeNormal) - planeOffset;
            float d2 = in2.dotProduct(planeNormal) - planeOffset;

            // If inside, keep
            if (d1 <= 0) out[count++] = in1;
            if (d2 <= 0) out[count++] = in2;

            // If segment crosses plane
            if (d1 * d2 < 0)
            {
                float t = d1 / (d1 - d2);
                vec3 intersection = in1 + (in2 - in1) * t;
                out[count++] = intersection;
            }

            if (count > 0) out1 = out[0];
            if (count > 1) out2 = out[1];

            return count;
        };

        vec3 cp1, cp2;

        auto refEdge = (refV2 - refV1).normalize();
        vec3 sideNormal1 =  refEdge;
        float offset1 = sideNormal1.dotProduct(refV1);

        // clip first plane
        int count = clipEdgeToPlane( incV1, incV2, sideNormal1, offset1, cp1, cp2 );
        if (count < 2) return false;

        // clip second
        vec3 sideNormal2 = refEdge * -1.0f;
        float offset2 = sideNormal2.dotProduct(refV2);

        count = clipEdgeToPlane(
            cp1, cp2,
            sideNormal2,
            offset2,
            cp1, cp2
        );

        if (count < 2) return false;

        // keep behind
        float faceOffset = refNormal.dotProduct(refV1);

        vec3 contacts[2];
        int contactCount = 0;

        float separation1 = refNormal.dotProduct(cp1) - faceOffset;
        if (separation1 <= 0)
            contacts[contactCount++] = cp1;

        float separation2 = refNormal.dotProduct(cp2) - faceOffset;
        if(separation2 <= 0)
              contacts[contactCount++] = cp2;


        std::cout << contacts[1].y << std::endl;
        
        return true;
    }


    bool circleCollisionTest(const rigid_t& body1, const rigid_t& body2, CollisionInfo& info)
    {   
        if(body1->type != RbShapeType::CIRCLE) 
            return false;

        auto c1 = dynamic_cast<CircleRb*>(&(*body1));

        // circle to circle
        if (body2->type == RbShapeType::CIRCLE)
        {
            auto c2 = dynamic_cast<CircleRb*>(&(*body2));
            auto maxRad = c1->radius + c2->radius;
            auto dist = c2->pos - c1->pos;
            auto dLength = dist.length();

            if(dLength != 0) {
                if(dLength < maxRad) {
                    float depth = maxRad - dLength;
                    auto normal = dist.normalize();
                    auto n = c1->pos + normal * c1->radius;
                    info.start = n + normal * -depth;
                    info.end = n;
                    info.normal = normal;
                    return true;
                }
            } else {
                if(c1->pos.x == c2->pos.x && c1->pos.y == c2->pos.y) {
                    auto normal = vec3{ 1, 0 };
                    float depth = std::max(c2->radius, c1->radius);
                    auto n = c1->pos + normal * c1->radius;
                    info.start = n + normal * -depth;
                    info.end = n;
                    info.normal = normal;
                    return true;
                }
            }
                
        }

        return false;
    }

}

#endif 