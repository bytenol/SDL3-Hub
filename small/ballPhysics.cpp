/*
* @file ballPhysics.cpp
* @date 19th Jan, 2026
*/
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <chrono>

#include <painter2/core.hpp>

#include "./include/phy/vec2.h"
#include "./include/phy/geometry.h"
#include "./include/phy/quadtree.h"

using namespace pnt;
using namespace phy;

constexpr int W = 1024;
constexpr int H = 640;

float randRange(const float& min, const float& max);

class Ball {
    float g = 900;

    public:
        vec3 pos{0, 0}, 
            vel{ 0, 0 }, 
            force{ 0, 0}, 
            acc{ 0, 0 },
            lastAcc { 0, 0 };

        float mass, radius;

        glm::vec4 color;

        Ball(const vec3& p, const float& r, const float& m) {
            pos = p;
            mass = m;
            radius = r;

            color.x = randRange(0, 255)/255;
            color.y = randRange(0, 255)/255;
            color.z = randRange(0, 255)/255;
            color.w = 1.0f;
        }

    public:
        void checkWallBounce() {
            if(pos.y + radius > H) {
                pos.y = H - radius;
                vel.y *= -0.8f;
            }

            if(pos.x - radius < 0) {
                pos.x = radius;
                vel.x *= -0.3f;
            }

            if(pos.x + radius > W) {
                pos.x = W - radius;
                vel.x *= -0.3f;
            }
        }

        public:
        void ballToBallCollision(Ball* ball) {
            if(ball != this) {
                auto dist = ball->pos - pos;
                const float maxRadius = radius + ball->radius;
                const float dl = dist.length();

                if(dl < maxRadius) {
                    auto normal = dist.normalize();
                    const float m1 = mass;
                    const float m2 = ball->mass;

                    float slop = 0.01f;
                    float percent = 0.8f;

                    float penetration = maxRadius - dl;
                    float correctionMag = std::max(penetration - slop, 0.0f) / (1/m1 + 1/m2) * percent;

                    vec3 correction = normal * correctionMag;

                    pos -= correction * (1/m1);
                    ball->pos += correction * (1/m2);

                    // impulse based resolution
                    float restitution = 0.7f;
                    float velAlongNormal = (ball->vel - vel).dotProduct(normal);
                    if(velAlongNormal > 0) return;
                    float j = -(1 + restitution) * velAlongNormal;
                    j /= (1/m1 + 1/m2);
                    phy::vec3 impulse = normal * j;
                    vel -= impulse * (1/m1);
                    ball->vel += impulse * (1/m2);
                }
            }                
        }
};


struct BallData
{
    phy::vec3 pos, size;
    Ball* ball;
};


class Window: public Painter2
{
	const float fixedTimeStep = 1.0f / 60.0f;
	float fixedTimeAccumulator = 0.0f;
	std::chrono::high_resolution_clock::duration t0;/*  */

    std::vector<BallData> ballData;
    phy::Quadtree<BallData> qtree;
    int selectedIndex = 0;
    Ball* selectedBall = nullptr;
    std::vector<Ball> balls;

public:
    Window(const std::string& t): Painter2(t, W, H){}

protected:
    bool onReady() override
    {
        balls.clear();

        for(int i = 0; i < 500; i++) {
            const float radius = randRange(10, 20);
            balls.push_back({ {randRange(50, W - 50), randRange(-200, H - 50)}, radius, radius * 0.5f });
            balls.back().mass = randRange(1.0f, 5.0f);
            // balls.push_back({ {randRange(0, W), randRange(50, H - 100)}, radius, radius * 0.5f });
        }
        balls.push_back({ {randRange(0, W), 0}, 20, 20 * 0.5f });

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
        return true;
    }

	void physicsProcess(const float& dt)
	{
        selectedBall = &(balls[selectedIndex % balls.size()]);
        
        ballData.clear();

        for(auto& ball: balls)
        {
            ball.vel += ball.acc * dt;
            ball.pos += ball.vel * dt;
            phy::vec3 sz{ ball.radius, ball.radius };
            ballData.push_back(BallData{ ball.pos - sz * 0.5f, sz, &ball });
        }

        qtree.resize(phy::Rect2D{ {0, -300}, {W, H + 300} }, 4, 1000);
        for(auto& data: ballData) qtree.insert(&data);

        for(int i = 0; i < 3; i++)
        for(int i = 0; i < balls.size(); i++) {
            auto& ball = balls[i];
            phy::Rect2D range{ { ball.pos.x - 50, ball.pos.y - 50 }, {100, 100} };
            std::vector<BallData*> queried;
            qtree.getRange(range, queried);
            for(auto& data: queried)
            {
                if(&ball >= data->ball) continue;
                ball.ballToBallCollision(data->ball);
            }
            ball.checkWallBounce();

        }

        for(auto& ball: balls) {
            ball.force = { 0, ball.mass * 10 };
            ball.acc = ball.force * (1/ball.mass);
        }
	}

	void process(const float& dt)
	{
	
	}

	void render()
	{
        beginUseBuffer(BATCHED_BUFFER);
        for(auto& ball: balls) {
            setRenderColor(ball.color.x, ball.color.y, ball.color.z, 1.0f);
            renderFillArc(ball.pos.x, ball.pos.y, ball.radius);
        }

        setRenderColor(1, 1, 1, 1);
        renderQuadtree(qtree);
        endUseBuffer();
	}

    template<typename T>
    void renderQuadtree(const phy::Quadtree<T>& qtree)
    {
        setRenderColor(1, 1, 1);
        auto& boundary = qtree.getBoundary();
        renderStrokeRect(boundary.pos.x, boundary.pos.y, boundary.size.x, boundary.size.y);
        for(auto& child: qtree.getChildren()) {
            renderQuadtree(*child);
        }
    }

    bool onRender() override
    {
		auto t1 = std::chrono::high_resolution_clock::now().time_since_epoch();
		std::chrono::duration<float> delta = t1 - t0;
		float dt = delta.count();
		t0 = t1;
		fixedTimeAccumulator += dt;
		process(dt);

		while(fixedTimeAccumulator > fixedTimeStep) {
			physicsProcess(fixedTimeStep);
			fixedTimeAccumulator -= fixedTimeStep;
		}

		render();

        return true;
    }

};



int main(int argc, char const *argv[])
{
    Window w{"Ball Physics"};
    if(!w.start()) {
        std::cout << w.getError() << std::endl;
        return -1;
    }
    return 0;
}


float randRange(const float& min, const float& max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}