/*
* @file RigidBody1.cpp
* @date 11th Jan, 2026
*/
#include <iostream>
#include <vector>
#include <cmath>
#include <SDL3/SDL.h>
#include <chrono>
#include <random>

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
float cr = 0.4;

struct collisionInfo {
	phy::vec2 vertex, intersection, edge, rp1, rp2;

	phy::vec2 getDir() const {
		return intersection - vertex;
	}

	void render(SDL_Renderer* renderer) const {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderLine(renderer, vertex.x, vertex.y, intersection.x, intersection.y);
	}

	float length() const {
		return getDir().length();
	}
};

float randRange(const float& min, const float& max);
void checkWallBounce(phy::polygon& poly);
bool checkPolygonCollision(phy::polygon& poly1, phy::polygon& poly2, collisionInfo& minCollision);
phy::polygon makeBlock(const float& w, const float& h, const float& m, const float& im);
void setupBlock(const float& w, const float& h, const float& angle, const float& x, const float& y);
bool processEvent(SDL_Event& evt);
void renderPolygon(phy::polygon& polygon);
void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);


int selected = 0;
phy::polygon* selectedPolygon = nullptr;
std::vector<phy::polygon> polygons;
std::vector<collisionInfo> collisionInfos;


void init()
{
	polygons.clear();

	setupBlock(6,6,45,200,90);
	setupBlock(10,10,0,240,25);
	setupBlock(20,20,30,320,40);
	setupBlock(14,6,60,380,40);
	setupBlock(10,10,-70,210,40);	
	setupBlock(20,20,-30,150,40);
	setupBlock(20,20,-130,190,70);
	setupBlock(20,20,-130,230,70);
	setupBlock(40,20,-130,300,70);
	setupBlock(20,20,-10,200,110);

    t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
}


bool checkPolygonCollision(phy::polygon& poly1, phy::polygon& poly2, collisionInfo& minCollision) {
	phy::polygon* polygon1 = &poly1;
	phy::polygon* polygon2 = &poly2;

	float minLength = INFINITY;
	bool isFirstPolyCollided = true;
	bool hasCollided = false;

	for(int iter = 0; iter < 2; iter++) {
		if(iter > 0) {
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
					collisionInfo info;
					info.vertex = l2;
					info.intersection.x = l1.x + t * (l2.x - l1.x);
					info.intersection.y = l1.y + t * (l2.y - l1.y);
					info.edge = (l4 - l3).normalize();
					info.rp1 = info.intersection - poly1.pos;
					info.rp2 = info.intersection - poly2.pos;
					
					if(info.length() < minLength) {
						minCollision = info;
						hasCollided = true;
						if(iter > 0) isFirstPolyCollided = false;
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
		SDL_SetRenderDrawColor(renderer, polygon.color.r, polygon.color.g, polygon.color.b, 255);
		if(&polygon == selectedPolygon) 
			SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
		renderPolygon(polygon);
	}

	for(auto& info: collisionInfos) info.render(renderer);

}


void update(float dt, SDL_Renderer* renderer)
{
	selectedPolygon = &(polygons[selected % polygons.size()]);
	// collisionInfos.clear();
	
	for(auto& polygon: polygons) {

		checkWallBounce(polygon);

		// collision detection
		for(auto& polygon2: polygons) {
			if(&polygon != &polygon2) {
				collisionInfo info;
				if(checkPolygonCollision(polygon, polygon2, info)) {
					auto displ = info.intersection - info.vertex;
					polygon.pos -= displ * 0.5;
					polygon2.pos += displ * 0.5;
					// checkWallBounce
					checkWallBounce(polygon);

					//collision resolution
					auto normal = info.edge.normalize().perp(1); //norm2.para(1);
					auto rp1 = info.rp1; //polygon.vertices[i].rotate(obj1.rotation);
					auto rp2 = info.rp2; //obj1.pos2D.add(rp1).subtract(obj2.pos2D);
					auto vp1 = polygon.vel + rp1.perp(-polygon.angVelo*rp1.length());
					auto vp2 = polygon2.vel + rp2.perp(-polygon2.angVelo*rp2.length());
					auto vr = vp1 - vp2;
					auto invm1 = 1/polygon.mass;
					auto invm2 = 1/polygon2.mass;
					auto invI1 = 1/polygon.im;
					auto invI2 = 1/polygon2.im;
					auto rp1Xn = rp1.crossProduct(normal);
					auto rp2Xn = rp1.crossProduct(normal);						
					auto impulse = -(1+cr)*vr.dotProduct(normal)/(invm1 + invm2 + rp1Xn*rp1Xn*invI1 + rp2Xn*rp2Xn*invI2); 
					polygon.vel = polygon.vel + normal * (impulse*invm1);
					polygon.angVelo += rp1.crossProduct(normal)*impulse*invI1;
					polygon2.vel = polygon2.vel - normal * (impulse*invm2);
					polygon2.angVelo += -rp2.crossProduct(normal) * impulse * invI2;
				}
			}
		}	// collision detection ends

		polygon.pos += polygon.vel * dt;
		polygon.setRotation(polygon.angVelo * dt);
		
		checkWallBounce(polygon);

		const float g = 5;
		phy::vec2 weight{ 0, polygon.mass * g };
		phy::vec2 drag = polygon.vel * -0.9;
		polygon.force = weight + drag;
		polygon.torque = 0;
		polygon.torque += -1 * polygon.angVelo;

		polygon.acc = polygon.force * (1/polygon.mass);
		const float alph = polygon.torque / polygon.im;

		polygon.vel += polygon.acc * dt;
		polygon.angVelo += alph * dt;
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

	auto window = SDL_CreateWindow("RigidBody 2", W, H, 0);
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
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
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

void checkWallBounce(phy::polygon &poly)
{
	int j, j2;
	bool testCollision = false, testCollision2 = false;
	for(int i = 0; i < poly.vertices.size(); i++) {
		const auto v = poly.pos + poly.vertices[i].rotate(poly.getRotation());
		if(v.y > FLOOR) {
			if(testCollision == false) {
				j = i;
				testCollision = true;
			} else {
				j2 = i;
				testCollision2 = true;
			}

			break;
		}
	}

	if(testCollision) {
		float rotation = poly.getRotation();
		poly.pos.y -= poly.pos.y + poly.vertices[j].rotate(rotation).y - FLOOR;
		if(testCollision2) {
			poly.pos.y -= poly.pos.y + poly.vertices[j2].rotate(rotation).y - FLOOR;
			testCollision2 = false;
		}

		phy::vec2 normal { 0, -1 };
		auto rp1 = poly.vertices[j].rotate(rotation);
		auto vp1 = poly.vel + rp1.perp(-poly.angVelo*rp1.length());
		auto rp1Xnormal = rp1.crossProduct(normal);
		auto impulse = -(1+cr)*vp1.dotProduct(normal)/(1/poly.mass + rp1Xnormal*rp1Xnormal/poly.im); 
		poly.vel = poly.vel + normal * (impulse/poly.mass);
		poly.angVelo += rp1.crossProduct(normal)*impulse/poly.im;
		testCollision = false;
	}

}

phy::polygon makeBlock(const float& w, const float& h, const float& m, const float& im){
	std::vector<phy::vec2> vertices {
		{-w/2,-h/2},
		{w/2,-h/2},
		{w/2,h/2},
		{-w/2,h/2}
	};

	phy::polygon p1;
	p1.vertices = vertices;
    p1.mass = m;
	p1.im = im;

	return p1;
}


void setupBlock(const float& w, const float& h, const float& angle, const float& x, const float& y){
	const float rho = 0.1;
	const float m = rho*w*h;
	const float im = m*(w*w+h*h)/12;
	auto block = makeBlock(w,h,m,im);
	block.color.r = randRange(0, 255);
	block.color.g = randRange(0, 255);
	block.color.b = randRange(0, 255);
	block.setRotation(angle*3.14159/180);
	block.pos = {x, y};
	polygons.push_back(block);
}


float randRange(const float& min, const float& max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}