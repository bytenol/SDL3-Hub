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



void physicsProcess(const float& dt)
{

}


void process(const float& dt)
{
	
}


void render(SDL_Renderer* renderer)
{

}


bool init()
{

	return true;
}


int main()
{
	canvas.window = SDL_CreateWindow("EightBall", W, H, 0);
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