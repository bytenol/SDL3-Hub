/*
TODO: once ball rect has intersect with a static ball, just ignore other collisions
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

std::chrono::high_resolution_clock::duration t0;

bool init();
void update(const float& dt);
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

phy::vec3 pos, vel, acc;
constexpr float mass = 1.0f;
constexpr float g = 10.0f;
constexpr float radius = 20.0f;

phy::vec3 calcAcc(const phy::vec3& vel) {
	phy::vec3 weight{ 0.0f, mass * g };
	phy::vec3 drag = vel * -0.1f;
	auto force = weight + drag;
	auto acc = force * (1 / mass);
	return acc;
};


void physicsProcess(const float& dt)
{
	// runge-kutta (RK4) scheme
	auto p1 = pos;
	auto v1 = vel;
	auto a1 = calcAcc(v1);
	auto p2 = p1 + v1 * (dt * 0.5f);
	auto v2 = v1 + a1 * (dt * 0.5f);
	auto a2 = calcAcc(v2);
	auto p3 = p1 + v2 * (dt * 0.5f);
	auto v3 = v1 + a2 * (dt * 0.5f);
	auto a3 = calcAcc(v3);
	auto p4 = p1 + v3 * dt;
	auto v4 = v1 + a3 * dt;
	auto a4 = calcAcc(v4);

	pos += (v1 + v2 * 2.0f + v3 * 2.0f + v4) * (dt / 6.0f);
	vel += (a1 + a2 * 2.0f + a3 * 2.0f + a4) * (dt / 6.0f);

	if(pos.y + radius > H) {
		pos.y = H - radius;
		vel.y *= -0.85f;
	}
}


void update(const float& dt)
{

}


void render(SDL_Renderer* renderer)
{
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	drawFilledCircle(renderer, pos.x, pos.y, radius);
}


bool init()
{
	pos = { W * 0.5f, 0.0f };
	vel = { 0.0f, 0.0f };
	acc = { 0.0f, 0.0f };
	return true;
}


int main()
{
	canvas.window = SDL_CreateWindow("Integration Scheme", W, H, 0);
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
		const float dt = (t1 - t0).count() * 10e-9;
		t0 = t1;
		fixedTimeAccumulator += dt;
		pollEvent(canvas.evt);
		update(dt);

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