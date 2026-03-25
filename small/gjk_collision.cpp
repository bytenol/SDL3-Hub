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

std::vector<phy::vec3> minkowskiPos, minkSimplex;

bool processEvent(SDL_Event& evt);
bool gjkCollision(phy::PolygonRb* polygon1, phy::PolygonRb* poly2, std::vector<phy::vec3>& simplex);
bool epa(phy::PolygonRb* polygon1, phy::PolygonRb* polygon2, std::vector<phy::vec3>& simplex, 
	phy::vec3& outNormal, float& outDepth);
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

struct EPAEdge
{
	phy::vec3 normal; 
	float distance;
	int index;
};


void init()
{
	auto b1 = addRect(200, 200, 50, 100);
	auto b2 = addRect(300, 200, 50, 100);
	b2->setRotation(45.0f);

    // addCircle(100, 200, 30);
	// addCircle(160, 235, 50);
    t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
}

bool gjkCollision(phy::PolygonRb* A, phy::PolygonRb* B, std::vector<phy::vec3>& simplex)
{
	auto support = [&](const phy::vec3& dir) {
		return A->findSupportPoint(dir) - B->findSupportPoint(dir * -1.0f);
	};

	auto doSimplex = [](std::vector<phy::vec3>& simplex, phy::vec3& dir)
	{
		if(simplex.size() == 2) {
			// line case
			auto a = simplex[1];
			auto b = simplex[0];
			auto ab = b - a;
			auto ao = a * -1.0f;
			dir = ab * ao * ab;

			if (dir.length() == 0)
                dir = phy::vec3(ab.y, -ab.x);
			
		} else {
			// triangle case
			auto a = simplex[2];
			auto b = simplex[1];
			auto c = simplex[0];

			auto ab = b - a;
            auto ac = c - a;
            auto ao = a * -1.0f;

			auto abPerp = ac * ab * ab;
            auto acPerp = ab * ac * ac;

            if (abPerp.dotProduct(ao) > 0) {
                simplex = { b, a };
                dir = abPerp;
            }
            else if (acPerp.dotProduct(ao) > 0) {
                simplex = { c, a };
                dir = acPerp;
            }
            else {
                // Origin is inside triangle
                return true;
            }
		}

		return false;
	};

	// just utility to visualize minkowski difference
	minkowskiPos.clear();
	for(int i = 0; i < 360; i++) {
		auto d = phy::vec3::fromPolarCoord(i * 3.14159f / 180.0f);
		auto sp = support(d);
		minkowskiPos.push_back(sp);
	}

	auto dir = B->pos - A->pos;
	if(dir.length() == 0) dir = {1, 0};

	auto p = support(dir);
	simplex.push_back(p);
	dir *= -1.0f;

	const int MAX_ITER = 50;
	int iter = 0;
	while(iter++ < MAX_ITER) {
		p = support(dir);

		// if it is not beyond origin
		if(p.dotProduct(dir) <= 0) 
			return false;

		simplex.push_back(p);

		if(doSimplex(simplex, dir))
			return true;
	}

	return false;
}
 

bool epa(phy::PolygonRb* polygon1, phy::PolygonRb* polygon2, std::vector<phy::vec3>& simplex, 
	phy::vec3& outNormal, float& outDepth)
{

	auto support = [&](const phy::vec3& dir) {
		return polygon1->findSupportPoint(dir) - polygon2->findSupportPoint(dir * -1.0f);
	};

	float tolerance = 0.0001f;

	const int MAX_ITER = 50;
	int iter = 0;
	while(iter++ < MAX_ITER) {

		EPAEdge closestEdge;
		closestEdge.distance = INFINITY;
		for(int i = 0; i < simplex.size(); i++) {
			int j = (i + 1) % simplex.size();
			auto a = simplex[i];
			auto b = simplex[j];
			auto edge = b - a;
			auto normal = edge.perp(1).normalize();
			auto dp = a.dotProduct(normal);
			if(dp < closestEdge.distance) {
				closestEdge.distance = dp;
				closestEdge.index = j;
				closestEdge.normal = normal;
			}
		}

		auto p = support(closestEdge.normal);
		float d = p.dotProduct(closestEdge.normal);
		if(d - closestEdge.distance < tolerance) {
			outDepth = d;
			outNormal = closestEdge.normal;
			return true;
		}

		simplex.insert(simplex.begin() + closestEdge.index, p);

	}

	return false;
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
	}

	phy::vec3 org { 500, 200 };

	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
	auto mp = phy::PolygonRb();
	mp.vertices = minkowskiPos;
	mp.pos = org;
	renderPolygon(&mp);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	auto ms = phy::PolygonRb();
	mp.vertices = minkSimplex;
	mp.pos = org;
	renderPolygon(&mp);

	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	drawFilledCircle(renderer, org.x, org.y, 2);

	minkSimplex.clear();
	minkowskiPos.clear();
}


void update(float dt, SDL_Renderer* renderer)
{
	if(bodies.empty()) return;
	selectedBody = &(*bodies[selected % bodies.size()]);
	collisionInfos.clear();

	for(size_t i = 0; i < bodies.size(); i++) {
		for(size_t j = i + 1; j < bodies.size(); j++) {
			auto* poly1 = dynamic_cast<phy::PolygonRb*>(bodies[i].get());
			auto* poly2 = dynamic_cast<phy::PolygonRb*>(bodies[j].get());

			if(poly1 && poly2) {
				std::vector<phy::vec3> simplex;
				if(gjkCollision(poly1, poly2, simplex)) {
					phy::vec3 normal;
					float depth;
					minkSimplex = simplex;
					if(epa(poly1, poly2, simplex, normal, depth)) {
						auto displ = normal * (depth * 0.5f);
						poly1->pos -= displ;
						poly2->pos += displ;
					}
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
				case SDLK_UP:
					selectedBody->theta++;
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

	auto window = SDL_CreateWindow("GJK Collision", W, H, 0);
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
