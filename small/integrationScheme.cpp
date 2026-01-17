#include <vector>
#include <cmath>
#include <chrono>
#include <cassert>
#include <SDL3/SDL.h>

SDL_Renderer* renderer;
SDL_Event evt;

struct Vec2
{
	float x = 0.0f;
	float y = 0.0f;
};

Vec2 pos, oldPos, vel, oldVel;
float time, oldTime;

std::chrono::steady_clock::time_point t0;

void update(double dt);
void draw();
void mainLoop();
void init();
void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);

void setupParams()
{
	pos.x = oldPos.x = 100.0f;
	pos.y = oldPos.y = 20.0f;

	vel.x = oldVel.x = 0.0f;
	vel.y = oldVel.y = 0.0f;

	time = oldTime = 0.0f;
}


int main()
{
	init();
	setupParams();
	mainLoop();

	return 0;
}

void update(double dt)
{

}


void draw()
{
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	drawFilledCircle(renderer, pos.x, pos.y, 40.0f);
}


void mainLoop()
{
	bool windowShouldClose = false;
	t0 = std::chrono::high_resolution_clock::now();
	while (!windowShouldClose)
	{
		while (SDL_PollEvent(&evt))
		{
			if (evt.type == SDL_EVENT_QUIT)
				windowShouldClose = true;
		}
		decltype(t0) t1 = std::chrono::high_resolution_clock::now();
		double dt = static_cast<double>((t1 - t0).count());
		t0 = t1;
		oldTime = time;
		time = dt;
		update(dt);
		SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
		SDL_RenderClear(renderer);
		draw();
		SDL_RenderPresent(renderer);
	}
}


void init()
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Verlet Engine 1", 480, 640, 0);
	if (!window) return;
	renderer = SDL_CreateRenderer(window, nullptr);
	if (!renderer) return;
}


void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius)
{
	auto drawHorizontalLine = [](SDL_Renderer* renderer, int x1, int x2, int y) -> void {
		for (int x = x1; x <= x2; x++)
			SDL_RenderPoint(renderer, x, y);
		};

	int x = 0;
	int y = radius;
	int d = 3 - int(radius) << 1;

	while (y >= x) {
		// Draw horizontal lines (scanlines) for each section of the circle
		drawHorizontalLine(r, px - x, px + x, py - y);
		drawHorizontalLine(r, px - x, px + x, py + y);
		drawHorizontalLine(r, px - y, px + y, py - x);
		drawHorizontalLine(r, px - y, px + y, py + x);

		// Update decision parameter and points
		if (d < 0) {
			d = d + (x << 2) + 6;
		}
		else {
			d = d + ((x - y) << 2) + 10;
			y--;
		}
		x++;

	}
}