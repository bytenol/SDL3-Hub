/*
* @file Archimedes.cpp
* @date 19th Jan, 2025
* Refactored 15th Feb, 2026
*/
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <chrono>

#include <painter2/core.hpp>

#include "./include/phy/vec2.h"
#include "./include/phy/geometry.h"

using namespace pnt;

struct {
	phy::vec3 pos, vel, acc;
	float mass = 1.0f;
	float radius = 1.0f;
} ball;


class Window: public Painter2
{
	const float fixedTimeStep = 1.0f / 60.0f;
	float fixedTimeAccumulator = 0.0f;
	const float g = 100.0f;
	const float ballDensity = 0.1f;
	const float pondDensity = 0.2f;
	phy::Rect2D pond;

	std::chrono::high_resolution_clock::duration t0;/*  */

public:
    Window(const std::string& t, const int& w, const int& h): Painter2(t, w, h){}

protected:
    bool onReady() override
    {

		pond.pos = { 0.0f, getHeight() * 0.5f };
		pond.size = { (float)getWidth(), getHeight() - pond.pos.y };

		ball.pos = { 300.0f, 0.0f };
		ball.radius = 20.0f;
		ball.acc = {0, 0};
		ball.vel = { randRange(-50, 50), 0 };

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
        return true;
    }

	void physicsProcess(const float& dt)
	{
		ball.pos += ball.vel * dt + ball.acc * (dt * dt * 0.5f);
		auto lastAcc = ball.acc;

		// calc acceleration
		float ballVolume = 3.1415f * ball.radius * ball.radius;
		float ballMass = ballDensity * ballVolume;
		float dragCoeff = 0.4f;

		float submergedArea = 0.0f;

		float waterLine = pond.pos.y;
		float bottom = ball.pos.y + ball.radius;
		float top = ball.pos.y - ball.radius;

		if (bottom > waterLine)
		{
			float h = bottom - waterLine;

			if (h >= 2 * ball.radius)
			{
				submergedArea = ballVolume; // fully submerged
			}
			else
			{
				float r = ball.radius;
				float segmentHeight = h;

				submergedArea =
					r * r * std::acos((r - segmentHeight) / r)
					- (r - segmentHeight) * sqrt(2 * r * segmentHeight - segmentHeight * segmentHeight);
			}
		}

		float u = pondDensity * g * submergedArea;

		phy::vec3 weight{ 0, ballMass * g };
		phy::vec3 upthrust { 0, -u };
		phy::vec3 drag;

		if(submergedArea > 0.0f) drag = ball.vel * -(dragCoeff * ball.vel.length());

		auto force = weight + upthrust + drag;
		ball.acc = force * (1 / ballMass );
	
		// update acceleration
		ball.vel += (ball.acc + lastAcc) * (0.5f * dt);
	}

	void process(const float& dt)
	{
	
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


		setRenderColor(64/255.0f, 224/255.0f, 208/255.0f, 50/255.0f);
		renderFillRect(pond.pos.x, pond.pos.y, pond.size.x, pond.size.y);

		setRenderColor(1.0f, 0.0f, 0.0f);
		renderFillArc(ball.pos.x, ball.pos.y, ball.radius);

        return true;
    }


	float randRange(const float& min, const float& max)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dist(min, max);
		return dist(gen);
	}

};



int main(int argc, char const *argv[])
{
    Window w{"Archimedes", 620, 480};
    if(!w.start()) {
        std::cout << w.getError() << std::endl;
        return -1;
    }
    return 0;
}