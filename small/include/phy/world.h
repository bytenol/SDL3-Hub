#ifndef __PHY_WORLD_H__
#define __PHY_WORLD_H__

#include <vector>
#include <memory>
#include <cmath>
#include <iostream>

#include "vec2.h"
#include "geometry.h"


namespace phy
{

    struct collision;

    enum class RbBodyType
    {
        DEFAULT,
        CIRCLE,
        POLYGON
    };


    struct RigidBody
    {
        private:
            float mass = 1.0f;
            float invMass = 1.0f;

        public:
        phy::vec3 pos, vel, acc, force;
        bool isStatic = false;
        std::vector<phy::vec3> vertices;
        RbBodyType type = RbBodyType::DEFAULT;

        float rotation = 0.0f;
        float torque, angularVelocity;
        float inertia, invInertia;

        virtual ~RigidBody() = default;

        inline float getRotation() const 
        {
            return rotation;
        }

        inline void setRotation(const float& angleInRadians)
        {
            rotation = angleInRadians;
        }

        void setInertia(const float& i) {
            inertia = i;
            invInertia = 1.0f / i;
        }

        float& getInertia() { return inertia; }

        float& getInvInertia() { return invInertia; }

        void setMass(const float& m) {
            mass = m;
            invMass = 1.0f / m;
        }

        float& getMass() { return mass; }

        const float& getInvMass() const { return invMass; }
    };

    
    struct PolygonRb: public RigidBody
    {
        PolygonRb(): RigidBody()
        {
            type = RbBodyType::POLYGON;
        }
    };


    struct CircleRb: public RigidBody
    {

        CircleRb(): RigidBody() 
        {
            type = RbBodyType::CIRCLE;
            initVertices();
        }

        void setRadius(const float& r)
        { 
            radius = r; 
            initVertices();
        }

        const float& getRadius() const { return radius; }

        private:
            float radius = 1.0f;

            void initVertices()
            {
                vertices.clear();
                const int segments = 32;
                float thetaStep = 2.0f * 3.14159f / segments;
                float prevX = radius;
                float prevY = 0.0f;

                for(int i = 1; i <= segments; i++) {
                    float theta = i * thetaStep;
                    float x = radius * std::cos(theta);
                    float y = radius * std::sin(theta);
                    vertices.push_back({ prevX, prevY });
                    prevX = x;
                    prevY = y;
                }
            }
    };


    struct collision
    {

        static bool circleToCircle(RigidBody* body1, RigidBody* body2)
        {
            if(!(body1->type == RbBodyType::CIRCLE) || !(body2->type == RbBodyType::CIRCLE))
                return false;

            auto circ1 = dynamic_cast<CircleRb*>(&(*body1));
            auto circ2 = dynamic_cast<CircleRb*>(&(*body2));

            auto dist = circ2->pos - circ1->pos;
            const float distLen = dist.length();
            const float radSum = circ1->getRadius() + circ2->getRadius();

            if(distLen < radSum)
            {
                auto normal = dist * (1/distLen);
                const float sumInvMass = circ1->getInvMass() + circ2->getInvMass();
                auto vRel = circ2->vel - circ1->vel;
                auto normalVel = vRel.dotProduct(normal);
                if(normalVel > 0.0f) return false;

                float restitution = 0.4f;
                float j = -(1 + restitution) * normalVel / (sumInvMass);
                phy::vec3 impulse{ normal * j };
                circ1->vel -= impulse * circ1->getInvMass();
                circ2->vel += impulse * circ2->getInvMass();

                float slop = 0.01f;
                float penetration = radSum - distLen;
                float percent = 0.8f;
                float correction = (std::max(penetration - slop, 0.0f) / sumInvMass) * percent;
                circ1->pos -= normal * (correction * circ1->getInvMass());
                circ2->pos += normal * (correction * circ2->getInvMass());
                return true;
            }

            return false;
        }

        static bool circleToPolygon(RigidBody* body1, RigidBody* body2)
        {
            phy::CircleRb* circ = nullptr;
            phy::PolygonRb* poly = nullptr;
            
            if(body1->type == RbBodyType::CIRCLE && body2->type == RbBodyType::POLYGON)
            {
                circ = dynamic_cast<CircleRb*>(&(*body1));
                poly = dynamic_cast<PolygonRb*>(&(*body2));
            } else if(body1->type == RbBodyType::POLYGON && body2->type == RbBodyType::CIRCLE)
            {
                circ = dynamic_cast<CircleRb*>(&(*body2));
                poly = dynamic_cast<PolygonRb*>(&(*body1));
            } else {
                return false;
            }

            auto vsz = poly->vertices.size();
            auto rotation = poly->getRotation();
            float depth = INFINITY;
            phy::vec3 axis;

            float minDistPt = INFINITY;
            phy::vec3 vClosest;

            for(int i = 0; i < vsz; i++)
            {
                auto v1 = poly->vertices[i].rotate(rotation);
                auto v2 = poly->vertices[(i+1)%vsz].rotate(rotation);
                auto edge = (v2 - v1);
                auto normal = edge.perp(1).normalize();

                auto pv1 = poly->pos + v1;
                float distq = (pv1 - circ->pos).length();
                if(distq < minDistPt) {
                    minDistPt = distq;
                    vClosest = pv1;
                }

                float minA = INFINITY, maxA = -INFINITY;
                for(int i = 0; i < vsz; i++)
                {
                    auto pos = poly->pos + poly->vertices[i].rotate(rotation);
                    auto dp = pos.dotProduct(normal);
                    minA = std::min(minA, dp);
                    maxA = std::max(maxA, dp);
                }

                auto c = circ->pos.dotProduct(normal);
                float minB = c - circ->getRadius();
                float maxB = c + circ->getRadius();

                bool isColliding = maxA >= minB && maxB >= minA;
                if(!isColliding) {
                    return false;
                }

                float overlap = std::min(maxA, maxB) - std::max(minA, minB);
                if(overlap < depth)
                {
                    depth = overlap;
                    axis = normal;
                }
            }

            vec3 cNormal = (vClosest - circ->pos).normalize();
            float minA = INFINITY, maxA = -INFINITY;
            for(int i = 0; i < poly->vertices.size(); i++)
            {
                auto p = poly->pos + poly->vertices[i].rotate(rotation);
                auto dp = p.dotProduct(cNormal);
                minA = std::min(minA, dp);
                maxA = std::max(maxA, dp);
            }

            auto c = circ->pos.dotProduct(cNormal);
            float minB = c - circ->getRadius();
            float maxB = c + circ->getRadius();

            bool isColliding = maxA >= minB && maxB >= minA;
            if(!isColliding) {
                return false;
            }

            float overlap = std::min(maxA, maxB) - std::max(minA, minB);
            if(overlap < depth)
            {
                depth = overlap;
                axis = cNormal;
            }

            circ->pos -= axis * -depth;
            std::cout << "rel" << std::endl;

            return true;
        }

        bool sat(RigidBody& r1, RigidBody& r2)
        {
            RigidBody* body1 = &r1;
            RigidBody* body2 = &r2;

            for(int i = 0; i < 2; i++)
            {
                if(i > 0) {
                    body1 = &r2;
                    body2 = &r1;
                }

                auto vsz = body1->vertices.size();
                for(int i = 0; i < vsz; i++)
                {
                    auto rotation = body1->getRotation();
                    auto v1 = body1->vertices[i].rotate(rotation);
                    auto v2 = body1->vertices[(i+1)%vsz].rotate(rotation);
                    auto edge = (v2 - v1).normalize();
                    auto normal = edge.perp(1);
                }

            }

            return false;

        }
    };


    class PhysicsWorld
    {
        using body_type = std::unique_ptr<RigidBody>;
        std::vector<body_type> bodies;

        float frameAccumulator = 0.0f;
        float fixedTimeStep = 1.0f / 60.0f;
        float g = 10.0f;
        float restitution = 0.4f;

        Rect2D boundary{ {0, 0}, {100, 100} };

        public:

            template<typename T, typename... Args>
            T& createObject(Args&&... args) {
                static_assert(std::is_base_of_v<RigidBody, T>);
                auto body = std::make_unique<T>(std::forward<Args>(args)...);
                T& ref = *body;
                bodies.push_back(std::move(body));
                return ref;
            }

            void update(const float& dt)
            {
                accumulateForce();
                integrateVelocity(dt);
                solveCollision();
                // solve impulses
                integratePosition(dt);
            }

            const Rect2D& getBoundary() const {
                return boundary;
            }

            void setBoundary(const phy::vec2& pos, const phy::vec2& size)
            {
                boundary.pos = pos;
                boundary.size = size;
            }

            void process(const float& dt)
            {
                frameAccumulator += dt;
                while(frameAccumulator >= fixedTimeStep)
                {
                    update(fixedTimeStep);
                    frameAccumulator -= fixedTimeStep;
                }
            }

            auto begin() const { return bodies.begin(); }
            auto end() const { return bodies.end(); }

            void setFps(const float& fps) {
                fixedTimeStep = 1.0f / fps;
            }

        private:
            void accumulateForce()
            {
                for(auto& body: bodies) {
                    if(body->isStatic) continue;
                    phy::vec3 weight{ 0.0f, body->getMass() * g };
                    body->force = weight;
                    body->torque = 0.0f;
                }
            }

            void integrateVelocity(const float& dt)
            {
                for(auto& body: bodies)
                {
                    if(body->isStatic) continue;
                    auto acc = body->force * body->getInvMass();
                    body->vel += acc * dt;
                    auto alph = body->torque * body->getInvInertia();
                    body->angularVelocity += alph * dt;
                }
            }

            void integratePosition(const float& dt)
            {
                for(auto& body: bodies)
                {
                    if(body->isStatic) continue;
                    body->pos += body->vel * dt;
                    body->rotation += body->angularVelocity * dt;
                }
            }
            
            void solveCollision()
            {
                for(auto& body: bodies)
                {
                    for(auto& body2: bodies)
                    {
                        if(&body >= &body2) continue;
                        collision::circleToCircle(&(*body), &(*body2));
                        collision::circleToPolygon(&(*body), &(*body2));
                        // if(collision::circleToPolygon(&(*body), &(*body2)))
                        // {
                        //     std::cout << "Colliding" << std::endl;
                        // }
                    }   
                }
            }
    };

}

#endif 