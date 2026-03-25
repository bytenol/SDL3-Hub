#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <chrono>
#include <SDL3/SDL.h>

#include "./include/phy/vec2.h"
#include "./include/phy/world.h"

constexpr int W = 640;
constexpr int H = 480;

std::chrono::high_resolution_clock::duration t0;

struct
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	bool windowShouldClose = false;
	SDL_Event evt;
} canvas;

phy::PhysicsWorld world;


bool init();
void process(const float& dt);
void render(SDL_Renderer* renderer);
void pollEvent(SDL_Event& evt);
void animate();
float randRange(const float& min, const float& max);
void drawFilledCircle(SDL_Renderer* renderer, const float& x, const float& y, const float& radius);
void drawStrokedCircle(SDL_Renderer* renderer, const float& px, const float& py, const float& r);


void process(const float& dt)
{
	world.process(dt);
}


void render(SDL_Renderer* renderer)
{
	const phy::RigidBody* bodyPtr = nullptr;
	const phy::CircleRb* circlePtr = nullptr;

	for(const auto& body: world)
	{
		auto rotation = body->getRotation();
		auto vsz = body->vertices.size();
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		
		for(int i = 0; i < vsz; i++)
		{
			auto v1 = body->pos + body->vertices[i].rotate(rotation);
			auto v2 = body->pos + body->vertices[(i+1) % vsz].rotate(rotation);
			SDL_RenderLine(renderer, v1.x, v1.y, v2.x, v2.y);
		}

		auto r0 = body->pos + body->vertices[0].rotate(rotation);
		SDL_RenderLine(renderer, body->pos.x, body->pos.y, r0.x, r0.y);
	}

	auto wBound = world.getBoundary();
	SDL_FRect rect{ wBound.pos.x, wBound.pos.y, wBound.size.x, wBound.size.y };
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderRect(renderer, &rect);
}


bool init()
{
	world.setFps(60.0f);
	world.setBoundary({ 0, -200 }, {W, H + 200});

	auto& c1 = world.createObject<phy::CircleRb>();
	c1.pos = { 300, 20.0f };
	c1.setRadius(20.0f);
	c1.setRotation(0 * 3.14159f / 180.0f);
	c1.setInertia(0.5f * (20 * 20));

	// auto& c2 = world.createObject<phy::CircleRb>();
	// c2.pos = { 310, 100};
	// c2.setRadius(30.0f);
	// c2.setRotation(45 * 3.14159f / 180.0f);
	// c2.vel.y = -20.0f;
	// c1.setInertia(0.5f * (30 * 30));

	auto& w1 = world.createObject<phy::PolygonRb>();
	w1.vertices = {
		{ 100.0f, 0.0f },
		{ 500.0f, 0.0f }
	};
	w1.pos = { 100.0f, 100.0f };
	w1.isStatic = true;

	return true;
}


int main()
{
	canvas.window = SDL_CreateWindow("Rigid System", W, H, 0);
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
		pollEvent(canvas.evt);
		process(dt);
		SDL_SetRenderDrawColor(canvas.renderer, 0, 0, 0, 255);
		SDL_RenderClear(canvas.renderer);
		render(canvas.renderer);
		SDL_RenderPresent(canvas.renderer);
	}
}

void drawFilledCircle(SDL_Renderer* renderer, const float& px, const float& py, const float& radius)
{
    auto drawHorizontalLine = [](SDL_Renderer* renderer, int x1, int x2, int y) -> void {
        for (int x = x1; x <= x2; x++) {
			SDL_FRect rect{ (float)x1, (float)y, x2-x1+1.0f, 1.0f};
			SDL_RenderFillRect(renderer, &rect);
			// SDL_RenderPoint(renderer, x, y);
		}
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


void drawStrokedCircle(SDL_Renderer* renderer, const float& px, const float& py, const float& r)
{
    const int segments = 32;
    float thetaStep = 2.0f * 3.14159f / segments;
    float prevX = px + r;
    float prevY = py;

    for(int i = 1; i <= segments; i++) {
        float theta = i * thetaStep;
        float x = px + r * std::cos(theta);
        float y = py + r * std::sin(theta);
        SDL_RenderLine(renderer, prevX, prevY, x, y);
        prevX = x;
        prevY = y;
    }
}


float randRange(const float& min, const float& max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}