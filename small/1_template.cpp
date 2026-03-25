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


class Window: public Painter2
{
	const float fixedTimeStep = 1.0f / 60.0f;
	float fixedTimeAccumulator = 0.0f;
	std::chrono::high_resolution_clock::duration t0;/*  */

public:
    Window(const std::string& t, const int& w, const int& h): Painter2(t, w, h){}

protected:
    bool onReady() override
    {

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
        return true;
    }

	void physicsProcess(const float& dt)
	{
	
	}

	void process(const float& dt)
	{
	
	}

	void render()
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

		render();

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
    Window w{"Untitled", 620, 480};
    if(!w.start()) {
        std::cout << w.getError() << std::endl;
        return -1;
    }
    return 0;
}