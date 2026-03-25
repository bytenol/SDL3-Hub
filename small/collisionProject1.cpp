/*
* @file collisionProject1.cpp
* @date 1st Mar, 2026
*/
#include <iostream>
#include <vector>
#include <cmath>
#include <SDL3/SDL.h>
#include <chrono>
#include <random>
#include <memory>

#include "./include/phy/vec2.h"
#include "./include/phy/polygonrb.h"

// using namespace phy;

SDL_Renderer* renderer;
constexpr int W = 680;
constexpr int H = 480;
constexpr int FLOOR = 460;
std::chrono::high_resolution_clock::duration t0;


bool processEvent(SDL_Event& evt);
void addCircle(const float& x, const float& y, const float& r);
phy::PolygonRb* addRect(const float& x, const float& y, const float& w, const float& h);
void renderPolygon(phy::PolygonRb* polygon);
float randRange(const float& min, const float& max);
void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);
void drawStrokedCircle(SDL_Renderer* renderer, const float& px, const float& py, const float& r);


int selected = 0;
std::vector<std::unique_ptr<phy::RigidShape>> bodies;
phy::RigidShape* selectedBody = nullptr;
std::vector<phy::CollisionInfo> collisionInfos;


void init()
{
	auto b1 = addRect(200, 200, 50, 100);
	auto b2 = addRect(300, 200, 50, 100);
	b2->setRotation(45.0f);

    // addCircle(100, 200, 30);
	// addCircle(160, 235, 50);
    t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
}


void render(SDL_Renderer* renderer)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderLine(renderer, 0, FLOOR, W, FLOOR);

	phy::PolygonRb* polygon;
	phy::CircleRb* circle;
	
	for(auto& body: bodies) {
		SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
		if(&(*body) == selectedBody) 
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		switch(body->type)
		{
			case phy::RbShapeType::POLYGON:
				polygon = dynamic_cast<phy::PolygonRb*>(&(*body));
				renderPolygon(polygon);
				break;
			case phy::RbShapeType::CIRCLE:
				circle = dynamic_cast<phy::CircleRb*>(&(*body));
				drawStrokedCircle(renderer, body->pos.x, body->pos.y, circle->radius);
				break;
		}
		// renderPolygon(body);
	}

	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
	for(auto& info: collisionInfos) {
		SDL_RenderLine(renderer, info.start.x, info.start.y, info.end.x, info.end.y);
	}

}


void update(float dt, SDL_Renderer* renderer)
{
	if(bodies.empty()) return;
	selectedBody = &(*bodies[selected % bodies.size()]);
	collisionInfos.clear();
	
	for(auto& body: bodies) {
		for(auto& body2: bodies) {
			if(&body >= &body2) continue;
			phy::CollisionInfo info;
			// if(phy::circleCollisionTest(body, body2, info))
			// 	collisionInfos.push_back(info);
			if((body->type == body2->type) && body->type == phy::RbShapeType::POLYGON) {
				auto poly1 = dynamic_cast<phy::PolygonRb*>(&(*body));
				auto poly2 = dynamic_cast<phy::PolygonRb*>(&(*body2));
				if(phy::satCollision(poly1, poly2, info)) {
					// collisionInfos.push_back(info);
					
				}
			}
		}
	}
}


bool processEvent(SDL_Event& evt) {
	float ex, ey;
	switch(evt.type) {
		case SDL_EVENT_QUIT:
			return true;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			ex = evt.motion.x;
			ey = evt.motion.y;
			addCircle(ex, ey, randRange(8, 40));
			return false;
		case SDL_EVENT_KEY_DOWN:
			// if(!selectedPolygon) break;
			switch(evt.key.key) {
				case SDLK_W:
					selectedBody->pos.y--;
					break;
				case SDLK_S:
					selectedBody->pos.y++;
					break;
				case SDLK_D:
					selectedBody->pos.x++;
					break;
				case SDLK_A:
					selectedBody->pos.x--;
					break;
				case SDLK_LEFT:
					selected = std::max(0, selected--);
					break;
				case SDLK_RIGHT:
					selected++;
					break;
			}
		
	}
	return false;
}


int main()
{
	if (SDL_Init(SDL_INIT_VIDEO) <= 0)
	{
		SDL_Log("SDL_INITIALIZATION ERROR: %s", SDL_GetError());
		return -1;
	}

	auto window = SDL_CreateWindow("collisionProject1", W, H, 0);
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
			windowShouldClose = processEvent(evt);
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

void renderPolygon(phy::PolygonRb* polygon)
{
	auto rotation = polygon->getRotation();
	for(int i = 0; i < polygon->vertices.size(); i++) {
		auto v1 = polygon->pos + polygon->vertices[i].rotate(rotation);
		auto v2 = polygon->pos + polygon->vertices[(i+1)%polygon->vertices.size()].rotate(rotation);
		SDL_RenderLine(renderer, v1.x, v1.y, v2.x, v2.y);
	}
}


phy::PolygonRb* addRect(const float& x, const float& y, const float& w, const float& h)
{
	std::vector<phy::vec3> vertices {
		{-w/2, -h/2},
		{w/2, -h/2},
		{w/2, h/2},
		{-w/2, h/2}
	};

	bodies.push_back(std::make_unique<phy::PolygonRb>());
	bodies.back()->vertices = vertices;
	bodies.back()->pos = { x, y };

	return dynamic_cast<phy::PolygonRb*>(&(*bodies.back()));
}


void addCircle(const float &x, const float &y, const float &r)
{
	bodies.push_back(std::make_unique<phy::CircleRb>(r));
	bodies.back()->pos = { x, y };
}
