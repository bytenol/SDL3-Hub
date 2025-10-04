#include <vector>
#include <cmath>
#include <cassert>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

SDL_Renderer* renderer;
SDL_Event evt;

const int W = 480;
const int H = 640;

float rotation = 0.0f;

struct Ball
{
	glm::vec3 pos;
	glm::mat4 model;
	float r;
};


std::vector<Ball> balls;

void update(double dt);
void draw();
void mainLoop();
void init();
void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);

void setupParams()
{
	for (int i = 0; i <= 360; i += 90)
	{
		float ang = glm::radians((float)i);
		float radius = 50.0f;
		float px = glm::cos(ang) * radius;
		float py = glm::sin(ang) * radius;

		Ball b;
		b.r = radius * 0.2f;
		b.pos = glm::vec3{ W/2 + px, H/2 + py, 0.0f };
		balls.push_back(b);
	}
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
	rotation += 0.02f;
	auto rotModel = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3{ 0.0f, 0.0f, 1.0f });

	for (auto& b : balls)
		b.pos = glm::vec3(glm::vec4(b.pos[0], b.pos[1], b.pos[2], 0.0f) * rotModel);
}


void draw()
{
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

	for(const auto& b: balls)
		drawFilledCircle(renderer, b.pos[0], b.pos[1], b.r);
}


void mainLoop()
{
	bool windowShouldClose = false;
	while (!windowShouldClose)
	{
		while (SDL_PollEvent(&evt))
		{
			if (evt.type == SDL_EVENT_QUIT)
				windowShouldClose = true;
		}
		update(1.0f / 60.0f);
		SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
		SDL_RenderClear(renderer);
		draw();
		SDL_RenderPresent(renderer);
	}
}


void init()
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Transformation", W, H, 0);
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