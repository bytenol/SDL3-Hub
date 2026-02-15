/*
* @file RigidBody1.cpp
* @date 11th Jan, 2026
* Added SAT collision on 14th Feb, 2026.
*/
#include <iostream>
#include <vector>
#include <cmath>
#include <SDL3/SDL.h>
#include <chrono>

#include "./include/phy/vec2.h"
#include "./include/phy/polygonrb.h"

SDL_Renderer* renderer;
constexpr int W = 680;
constexpr int H = 480;
constexpr int FLOOR = 460;
std::chrono::high_resolution_clock::duration t0;

float v = 1;
float w = 0.5;
float angDispl = 0;
bool useSat = true;

struct collisionInfo {
	phy::vec2 vertex, intersection, edge;
	float depth = 0.0f;

	void render(SDL_Renderer* renderer) {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderLine(renderer, vertex.x, vertex.y, intersection.x, intersection.y);
	}

	float length() {
		return (intersection - vertex).length();
	}
};

bool checkPolygonCollision(phy::polygon& poly1, phy::polygon& poly2, collisionInfo& minCollision);
bool satCollision(phy::polygon& poly1, phy::polygon& pol2, collisionInfo& minCollision);
bool processEvent(SDL_Event& evt);
void renderPolygon(phy::polygon& polygon);
void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);


int selected = 0;
phy::polygon* selectedPolygon = nullptr;
std::vector<phy::polygon> polygons;
std::vector<collisionInfo> collisionInfos;


void init()
{
    std::vector<phy::vec2> triangleVert {
        { -50, -20 },
		{ 50, -20},
		{50, 20},
		{-50, 20}
    };

    phy::polygon p1;
    p1.vertices = triangleVert;
    p1.pos = { W/2, 70 };
    p1.mass = 1.0f;
	p1.setRotation(3.141596/4);
	polygons.push_back(p1);

	p1.pos.x -= 100;
	p1.setRotation(0);
	polygons.push_back(p1);

    t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
}

bool satCollision(phy::polygon& poly1, phy::polygon& poly2, collisionInfo& minCollision)
{
	phy::polygon* polygon1 = &poly1;
	phy::polygon* polygon2 = &poly2;

	float depth = INFINITY;
	phy::polygon* refPolygon = polygon1;

	for(int i = 0; i < 2; i++) {
		if(i > 0) {
			polygon1 = &poly2;
			polygon2 = &poly1;
		}

		for(int i = 0; i < polygon1->vertices.size(); i++)
		{
			auto rotation = polygon1->getRotation();
			auto p1 = polygon1->pos + polygon1->vertices[i].rotate(rotation);
			auto p2 = polygon1->pos + polygon1->vertices[(i + 1) % polygon1->vertices.size()].rotate(rotation);
			auto edge = (p2 - p1).normalize();
			auto normal = edge.perp(1).normalize();

			float minA = INFINITY, maxA = -INFINITY;
			float minB = INFINITY, maxB = -INFINITY;

			for(int i = 0; i < polygon1->vertices.size(); i++)
			{
				auto pos = polygon1->pos + polygon1->vertices[i].rotate(polygon1->getRotation());
				auto dp = pos.dotProduct(normal);
				minA = std::min(dp, minA);
				maxA = std::max(dp, maxA);
			}

			for(int i = 0; i < polygon2->vertices.size(); i++)
			{
				auto pos = polygon2->pos + polygon2->vertices[i].rotate(polygon2->getRotation());
				auto dp = pos.dotProduct(normal);
				minB = std::min(dp, minB);
				maxB = std::max(dp, maxB);
			}

			bool isColliding = maxA >= minB && maxB >= minA;
			if(!isColliding) {
				return false;
			}

			float overlap = std::min(maxA, maxB) - std::max(minA, minB);
			
			if(overlap < depth) {
				depth = overlap;
				minCollision.depth = depth;
				minCollision.edge = normal;
			}
		}

	}
	
	return true;
}


bool checkPolygonCollision(phy::polygon& poly1, phy::polygon& poly2, collisionInfo& minCollision) {
	phy::polygon* polygon1 = &poly1;
	phy::polygon* polygon2 = &poly2;

	float minLength = INFINITY;

	bool hasCollided = false;

	for(int i = 0; i < 2; i++) {
		if(i > 0) {
			polygon1 = &poly2;
			polygon2 = &poly1;
		}

		for(int i = 0; i < polygon1->vertices.size(); i++) {
			auto l1 = polygon1->pos;
			auto l2 = l1 + polygon1->vertices[i].rotate(polygon1->getRotation());

			for(int j = 0; j < polygon2->vertices.size(); j++) {
				auto l3 = polygon2->pos + polygon2->vertices[j].rotate(polygon2->getRotation());
				auto l4 = polygon2->pos + polygon2->vertices[(j+1)%polygon2->vertices.size()].rotate(polygon2->getRotation());

				float denom = (l1.x - l2.x) * (l3.y - l4.y) - (l1.y - l2.y) * (l3.x - l4.x);
				float t = ((l1.x - l3.x) * (l3.y - l4.y) - (l1.y - l3.y) * (l3.x - l4.x)) / denom;
				float u = -((l1.x - l2.x) * (l1.y - l3.y) - (l1.y - l2.y) * (l1.x - l3.x)) / denom;

				if(t >= 0 && t <= 1 && u >= 0 && u <= 1) {
					phy::vec2 intersection { l1.x + t * (l2.x - l1.x), l1.y + t * (l2.y - l1.y) };
					float d = (intersection - l2).length();
					if(d < minLength) {
						minLength = d;
						minCollision.depth = d;
						minCollision.edge = (l4 - l3).normalize().perp(1);
						hasCollided = true;
					}
				}
			}

		}

	}

	return hasCollided;
}


void render(SDL_Renderer* renderer)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderLine(renderer, 0, FLOOR, W, FLOOR);
	
	for(auto& polygon: polygons) {
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		if(&polygon == selectedPolygon) 
			SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
		renderPolygon(polygon);
	}

	for(auto& info: collisionInfos) info.render(renderer);

}


void update(float dt, SDL_Renderer* renderer)
{
	selectedPolygon = &(polygons[selected % polygons.size()]);
	collisionInfos.clear();
	
	for(auto& polygon: polygons) {
		for(auto& polygon2: polygons) {
			if(&polygon != &polygon2) {
				collisionInfo info;
				int(*a)(int, int);
				bool(*collisionFunction)(phy::polygon& p1, phy::polygon& p2, collisionInfo& c);
				collisionFunction = useSat ? satCollision : checkPolygonCollision;
				if(collisionFunction(polygon, polygon2, info)) {
					auto normal = info.edge;
					auto displ = normal * (info.depth * 0.5f);
					polygon.pos += displ;
					polygon2.pos -= displ;
				}
			}
		}
	}

}

bool processEvent(SDL_Event& evt) {
	switch(evt.type) {
		case SDL_EVENT_QUIT:
			return true;
		case SDL_EVENT_KEY_DOWN:
			// if(!selectedPolygon) break;
			switch(evt.key.key) {
				case SDLK_W:
					selectedPolygon->pos.y--;
					break;
				case SDLK_S:
					selectedPolygon->pos.y++;
					break;
				case SDLK_D:
					selectedPolygon->pos.x++;
					break;
				case SDLK_A:
					selectedPolygon->pos.x--;
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

	auto window = SDL_CreateWindow("Polygon Collision", W, H, 0);
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

void renderPolygon(phy::polygon &polygon)
{
	for(int i = 0; i < polygon.vertices.size(); i++) {
		auto v1 = polygon.pos + polygon.vertices[i].rotate(polygon.getRotation());
		auto v2 = polygon.pos + polygon.vertices[(i+1)%polygon.vertices.size()].rotate(polygon.getRotation());
		SDL_RenderLine(renderer, v1.x, v1.y, v2.x, v2.y);
	}
}
