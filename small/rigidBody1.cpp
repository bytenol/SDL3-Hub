/*
* @file RigidBody1.cpp
* @date 11th Jan, 2026
*/
#include <iostream>
#include <vector>
#include <cmath>
#include <SDL3/SDL.h>
#include <chrono>

#include "./include/phy/vec2.h"

SDL_Renderer* renderer;
constexpr int W = 680;
constexpr int H = 480;
constexpr int FLOOR = 460;
std::chrono::high_resolution_clock::duration t0;

float v = 1;
float w = 0.5;
float angDispl = 0;


void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);

struct polygon {
    std::vector<phy::vec2> vertices;
    phy::vec2 pos, vel, acc, force;
    float mass = 1;
	float im = 1;
    float angVelo = 0;
	float torque = 0;
	float theta = 0;
    SDL_Color color{ 255, 0, 0 };

    void setRotation(const float& angle) {
		theta = 0;
        for(auto& vert: vertices) {
            vert = vert.rotate(angle);
        }
    } 

	float getRotation() const  {
		return theta;
	}
    

    void render(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

        phy::vec2 prev;
        for(int i = 0; i < vertices.size() - 1; i++) {
            const auto curr = pos + vertices[i];
            const auto next = pos + vertices[i + 1];
            prev = next;
            SDL_RenderLine(renderer, curr.x, curr.y, next.x, next.y);
        }

        const auto next = pos + vertices[0];
        SDL_RenderLine(renderer, prev.x, prev.y, next.x, next.y);
    }
};

polygon poly;


void init()
{
    std::vector<phy::vec2> triangleVert {
        { -50, -20 },
		{ 50, -20},
		{50, 20},
		{-50, 20}
    };

    polygon p1;
    p1.vertices = triangleVert;
    p1.pos = { W/2, 30 };
    p1.vel = { 0.0f, 0.0f };
    p1.mass = 1.0f;
	p1.im = 5000;
	p1.setRotation(3.141596/4);
	poly = p1;

    t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
}

void render(SDL_Renderer* renderer)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderLine(renderer, 0, FLOOR, W, FLOOR);
	poly.render(renderer);
}


void update(float dt, SDL_Renderer* renderer)
{
	// move object
	poly.pos += poly.vel * dt;
	poly.setRotation(poly.angVelo * dt);

	// check bounce
	bool isColliding = false, testCollision = false;
	int j;
	for(int i = 0; i < poly.vertices.size(); i++) {
		auto nPos = poly.pos + poly.vertices[i].rotate(poly.getRotation());
		if(nPos.y >= FLOOR) {
			if(!testCollision) {
				testCollision = true;
				j = i;
			}
		}
	}

	// test collision
	if(testCollision) {
		auto dist = poly.pos.y + poly.vertices[j].rotate(poly.getRotation()).y - FLOOR;
		poly.pos.y -= dist;
		phy::vec2 normal { 0, -1 };

		auto rp1 = poly.vertices[j].rotate(poly.getRotation());
		auto vp1 = poly.vel + rp1.perp(-poly.angVelo * rp1.length());
		auto rp1Xnormal = rp1.crossProduct(normal);
		float cr = 0.4f;
		float impulse = -(1+cr)*vp1.dotProduct(normal)/(1/poly.mass + rp1Xnormal*rp1Xnormal/poly.im); 
		poly.vel += normal * (impulse/poly.mass);
		poly.angVelo += rp1.crossProduct(normal)*impulse/poly.im;

		testCollision = false;
	}

	// calc force
	auto force = phy::vec2(0, poly.mass * 10);
	float torque = 0;
	torque += -1 * poly.angVelo;

	auto acc = force * (1/poly.mass);
	auto alph = torque / poly.im;
	poly.vel += acc * dt;
	poly.angVelo += alph * dt;
}


int main()
{
	if (SDL_Init(SDL_INIT_VIDEO) <= 0)
	{
		SDL_Log("SDL_INITIALIZATION ERROR: %s", SDL_GetError());
		return -1;
	}

	auto window = SDL_CreateWindow("Rigid Body 1", W, H, 0);
	if (!window)
	{
		SDL_Log("WINDOW_CREATION_FAILED: %s", SDL_GetError());
		return -1;
	}

	renderer = SDL_CreateRenderer(window, nullptr);
	if (!renderer)
	{
		SDL_Log("RENDERER_INITIALIZATION_FAILED: %s", SDL_GetError());
		return -1;
	}

	SDL_Event evt;
	bool windowShouldClose = false;
	init();
	while (!windowShouldClose)
	{
		while (SDL_PollEvent(&evt))
		{
			if (evt.type == SDL_EVENT_QUIT)
				windowShouldClose = true;
		}
        const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        const float dt = (now - t0).count() * 10e-9;
        t0 = now;
		update(dt, renderer);
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);
		render(renderer);
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
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