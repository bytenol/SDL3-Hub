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
#include <SDL3/SDL.h>

#include "./include/phy/vec2.h"

constexpr int W = 640;
constexpr int H = 480;
constexpr float fixedTimeStep = 1.0f / 60.0f;
float fixedTimeAccumulator = 0.0f;

std::chrono::high_resolution_clock::duration t0;/*  */


bool init();
void process(const float& dt);
void physicsProcess(const float& dt);
void render(SDL_Renderer* renderer);
void pollEvent(SDL_Event& evt);
void animate();
float randRange(const float& min, const float& max);
void drawFilledCircle(SDL_Renderer* renderer, const float& x, const float& y, const float& radius);

struct
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	bool windowShouldClose = false;
	SDL_Event evt;
} canvas;


constexpr float g = 100.0f;
SDL_FRect pond;

struct {
	phy::vec2 pos, vel, acc;
	float mass = 1.0f;
	float radius = 1.0f;
} ball;

float ballDensity = 0.1f;
float pondDensity = 0.2f;


void physicsProcess(const float& dt)
{
	ball.pos += ball.vel * dt + ball.acc * (dt * dt * 0.5f);
	auto lastAcc = ball.acc;

	// calc acceleration
	float ballVolume = 3.1415f * ball.radius * ball.radius;
	float ballMass = ballDensity * ballVolume;
	float dragCoeff = 0.4f;

	float submergedArea = 0.0f;

	float waterLine = pond.y;
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

	phy::vec2 weight{ 0, ballMass * g };
	phy::vec2 upthrust { 0, -u };
	phy::vec2 drag;

	if(submergedArea > 0.0f) drag = ball.vel * -(dragCoeff * ball.vel.length());

	auto force = weight + upthrust + drag;
	ball.acc = force * (1 / ballMass );
 
	// update acceleration
	ball.vel += (ball.acc + lastAcc) * (0.5f * dt);
}


void process(const float& dt){}


void render(SDL_Renderer* renderer)
{
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);

	SDL_SetRenderDrawColor(renderer, 64, 224, 208, 50);
	SDL_RenderFillRect(renderer, &pond);

	SDL_SetRenderDrawColor(renderer, 195, 32, 18, 255);
	drawFilledCircle(renderer, ball.pos.x, ball.pos.y, ball.radius);
}


bool init()
{
	pond.x = 0.0f;
	pond.y = H * 0.5f;
	pond.w = W;
	pond.h = H - pond.y;

	ball.pos = { 300.0f, 0.0f };
	ball.radius = 20.0f;
	ball.acc = {0, 0};
	ball.vel = { randRange(-50, 50), 0 };
	return true;
}


int main()
{
	canvas.window = SDL_CreateWindow("Archimedes Principle", W, H, 0);
	canvas.renderer = SDL_CreateRenderer(canvas.window, nullptr);

	if (!canvas.window || !canvas.renderer)
	{
		SDL_Log("Error creating canvas window or renderer: %s", SDL_GetError());
		return -1;
	}

	init();

	animate();
	SDL_DestroyWindow(canvas.window);
	SDL_Quit();
	return 0;
}


void pollEvent(SDL_Event& evt)
{
	while (SDL_PollEvent(&evt))
	{
		if (evt.type == SDL_EVENT_QUIT)
		{
			canvas.windowShouldClose = true;
			return;
		}
	}
}

void animate()
{
	t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
	while (!canvas.windowShouldClose)
	{
		auto t1 = std::chrono::high_resolution_clock::now().time_since_epoch();
		std::chrono::duration<float> delta = t1 - t0;
		float dt = delta.count();
		t0 = t1;
		fixedTimeAccumulator += dt;
		pollEvent(canvas.evt);
		process(dt);

		while(fixedTimeAccumulator > fixedTimeStep) {
			physicsProcess(fixedTimeStep);
			fixedTimeAccumulator -= fixedTimeStep;
		}

		SDL_SetRenderDrawColor(canvas.renderer, 0, 0, 0, 255);
		SDL_RenderClear(canvas.renderer);
		render(canvas.renderer);
		SDL_RenderPresent(canvas.renderer);
	}
}

void drawFilledCircle(SDL_Renderer* renderer, const float& px, const float& py, const float& radius)
{
    auto drawHorizontalLine = [](SDL_Renderer* renderer, int x1, int x2, int y) -> void {
        for (int x = x1; x <= x2; x++)
            SDL_RenderPoint(renderer, x, y);
        };

    int x = 0;
    int y = radius;
    int d = 3 - (int(radius) << 1);

    while (y >= x)
    {
        drawHorizontalLine(renderer, px - x, px + x, py - y);
        drawHorizontalLine(renderer, px - x, px + x, py + y);
        drawHorizontalLine(renderer, px - y, px + y, py - x);
        drawHorizontalLine(renderer, px - y, px + y, py + x);

        if (d < 0)
            d = d + (x << 2) + 6;
        else {
            d = d + ((x - y) << 2) + 10;
            y--;
        }
        x++;
    }

}


float randRange(const float& min, const float& max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}