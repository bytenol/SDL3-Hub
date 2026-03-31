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
#include <numeric>

#include "./include/phy/vec2.h"
#include "./include/phy/polygonrb.h"

// using namespace phy;

using simplex_t = std::vector<phy::vec3>;

SDL_Renderer* renderer;
constexpr int W = 680;
constexpr int H = 480;
constexpr int FLOOR = 460;
std::chrono::high_resolution_clock::duration t0;

std::vector<phy::vec3> contactPoints;


bool processEvent(SDL_Event& evt);
phy::PolygonRb* addPolygon(const float& x, const float& y, const float& segmentCount, const float& radius);
bool gjk_collision(phy::RigidShape* body1, phy::RigidShape* body2, simplex_t& simplex);
void epa_collision(phy::RigidShape* body1, phy::RigidShape* body2, simplex_t& simplex, phy::vec3& normal, float& depth);
void addCircle(const float& x, const float& y, const float& r);
phy::PolygonRb* addRect(const float& x, const float& y, const float& w, const float& h);
void renderBody(SDL_Renderer* renderer, phy::CircleRb* circle);
void renderBody(SDL_Renderer* renderer, phy::PolygonRb* polygon);
float randRange(const float& min, const float& max);
void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);
void drawStrokedCircle(SDL_Renderer* renderer, const float& px, const float& py, const float& r);


int selected = 0;
std::vector<std::unique_ptr<phy::RigidShape>> bodies;
phy::RigidShape* selectedBody = nullptr;


bool gjk_collision(phy::RigidShape* body1, phy::RigidShape* body2, simplex_t& simplex)
{
	auto minkowskiSupport = [](phy::RigidShape* A, phy::RigidShape* B, const phy::vec3& dir) {
		return A->findSupportPoint(dir) - B->findSupportPoint(dir * -1.0f);
	};

	auto doSimplex = [](std::vector<phy::vec3>& simplex, phy::vec3& dir)
	{
		if(simplex.size() == 2) 
		{
			// line case
			auto a = simplex[1];
			auto b = simplex[0];
			auto ab = b - a;
			auto ao = a * -1.0f;
			dir = ab * ao * ab;

			if (dir.length() == 0)
                dir = phy::vec3(ab.y, -ab.x);

		} else {
			auto a = simplex[2];
			auto b = simplex[1];
			auto c = simplex[0];

			auto ac = c - a;
			auto ab = b - a;
			auto ao = a * -1.0f;

			auto abPerp = ab * ab * ac;	// tripple product
            auto acPerp = ab * ac * ac;

			if(abPerp.dotProduct(ao) > 0) {
				simplex = {b, a};
				dir = abPerp;
			} else if(acPerp.dotProduct(ao) > 0) {
				simplex = { c, a };
				dir = acPerp;
			} else {
				return true;
			}
		}

		return false;
	};

	auto dir = body2->pos - body1->pos;
	if(dir.length() == 0) dir = { 1, 0, 0 };

	auto p = minkowskiSupport(body1, body2, dir);
	simplex.push_back(p);
	dir *= -1.0f;

	const int MAX_ITER = 50;
	int iter = 0;

	while(iter++ < MAX_ITER)
	{
		p = minkowskiSupport(body1, body2, dir);
		if(p.dotProduct(dir) <= 0) 
			return false;

		simplex.push_back(p);
		if(doSimplex(simplex, dir))
			return true;
	}

	return false;
}


struct EpaEdge {
	int index;
	float depth;
	phy::vec3 normal;
};


void epa_collision(phy::RigidShape* body1, phy::RigidShape* body2, simplex_t& simplex, phy::vec3& normal, float& depth)
{

	auto minkowskiSupport = [](phy::RigidShape* A, phy::RigidShape* B, const phy::vec3& dir) {
		return A->findSupportPoint(dir) - B->findSupportPoint(dir * -1.0f);
	};

	const int MAX_ITER = 50;
	int iter = 0;
	const float tolerance = 0.0001f;

	EpaEdge closestEdge;
	bool isFound = false;

	while(iter++ < MAX_ITER && !isFound) {
		
		float minDot = INFINITY;

		for(int i = 0; i < simplex.size(); i++) {
			int j = (i + 1) % simplex.size();
			auto a = simplex[i];
			auto b = simplex[j];
			auto edge = b - a;
			auto normal = edge.perp(1).normalize();
			auto dp = a.dotProduct(normal);
			if(dp < minDot) {
				minDot = dp;
				closestEdge.normal = normal;
				closestEdge.depth = dp;
				closestEdge.index = j;
			}
		}

		auto p = minkowskiSupport(body1, body2, closestEdge.normal);
		auto d = p.dotProduct(closestEdge.normal);
		if(d - closestEdge.depth < tolerance) {
			normal = closestEdge.normal;
			depth = closestEdge.depth;
			isFound = true;
		}

		if(!isFound) {
			simplex.insert(simplex.begin() + closestEdge.index, p);
		}
	}
}



std::vector<phy::vec3> clipEdge(phy::RigidShape* polyA, phy::RigidShape* polyB, phy::vec3 normal) {

	const float EPS = 1e-4f;

    struct Edge { phy::vec3 v1, v2; };

    auto getWorldVertex = [](phy::RigidShape* p, int i) {
        return p->pos + p->vertices[i].rotate(p->getRotation());
    };

    // -----------------------------
    // Find best edge (reference/incident)
    // -----------------------------
    auto findBestEdge = [&](phy::RigidShape* poly, const phy::vec3& n) {
        int count = poly->vertices.size();

        // 1. Find support vertex
        float maxDot = -INFINITY;
        int index = 0;
        for (int i = 0; i < count; i++) {
            auto v = getWorldVertex(poly, i);
            float d = v.dotProduct(n);
            if (d > maxDot) {
                maxDot = d;
                index = i;
            }
        }

        // 2. Get adjacent vertices
        int prev = (index - 1 + count) % count;
        int next = (index + 1) % count;

        auto v = getWorldVertex(poly, index);
        auto vPrev = getWorldVertex(poly, prev);
        auto vNext = getWorldVertex(poly, next);

        // 3. Edges
        auto e1 = (v - vPrev).normalize();
        auto e2 = (vNext - v).normalize();

        // 4. Normals
        phy::vec3 n1 = e1.perp(1).normalize();
        phy::vec3 n2 = e2.perp(1).normalize();

        // 5. Choose edge most aligned with normal
        if (n1.dotProduct(n) > n2.dotProduct(n))
            return Edge{ vPrev, v };
        else
            return Edge{ v, vNext };
    };

    // -----------------------------
    // Get edges
    // -----------------------------
    Edge edgeA = findBestEdge(polyA, normal);
    Edge edgeB = findBestEdge(polyB, normal * -1.0f);

    // -----------------------------
    // Decide reference vs incident
    // -----------------------------
    auto edgeDirA = (edgeA.v2 - edgeA.v1).normalize();
    auto edgeDirB = (edgeB.v2 - edgeB.v1).normalize();

    phy::vec3 normalA = edgeDirA.perp(1).normalize();
    phy::vec3 normalB = edgeDirB.perp(1).normalize();

    Edge refEdge, incEdge;

    if (std::abs(normalA.dotProduct(normal)) >= std::abs(normalB.dotProduct(normal))) {
        refEdge = edgeA;
        incEdge = edgeB;
    } else {
        refEdge = edgeB;
        incEdge = edgeA;
        normal *= -1.0f;
    }

	
    // -----------------------------
    // Clipping helper
    // -----------------------------
    auto clip = [](const phy::vec3& n, float c, std::vector<phy::vec3>& face) {
        std::vector<phy::vec3> out;

        float d1 = n.dotProduct(face[0]) - c;
        float d2 = n.dotProduct(face[1]) - c;

        if (d1 >= 0) out.push_back(face[0]);
        if (d2 >= 0) out.push_back(face[1]);

        if (d1 * d2 < 0) {
            float t = d1 / (d1 - d2);
            out.push_back(face[0] + (face[1] - face[0]) * t);
        }

        return out;
    };

    // -----------------------------
    // Start clipping
    // -----------------------------
    std::vector<phy::vec3> incident = { incEdge.v1, incEdge.v2 };

    phy::vec3 refDir = (refEdge.v2 - refEdge.v1).normalize();

    // Side planes
    float c1 = refDir.dotProduct(refEdge.v1);
    float c2 = (refDir * -1.0f).dotProduct(refEdge.v2);

    auto cp1 = clip(refDir, c1, incident);
    if (cp1.empty()) return {};

    auto cp2 = clip(refDir * -1.0f, c2, cp1);
    if (cp2.empty()) return {};

    // -----------------------------
    // Final contact points
    // -----------------------------
    std::vector<phy::vec3> contacts;

    float refC = normal.dotProduct(refEdge.v1);

    for (auto& p : cp2) {
        float separation = normal.dotProduct(p) - refC;
        if (separation <= EPS) {
            contacts.push_back(p);
        }
    }

    return contacts;

}


void init()
{
	auto b1 = addRect(200, 200, 50, 100);
	auto b2 = addRect(300, 200, 50, 100);

	auto b3 = addPolygon(400, 300, 5, 50);
	b2->setRotation(45.0f);

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
				renderBody(renderer, polygon);
				break;
			case phy::RbShapeType::CIRCLE:
				circle = dynamic_cast<phy::CircleRb*>(&(*body));
				renderBody(renderer, circle);
				break;
		}
		// renderPolygon(body);
	}


	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	for(auto contact: contactPoints) {
		drawFilledCircle(renderer, contact.x, contact.y, 3);
	}
	contactPoints.clear();

}


void update(float dt, SDL_Renderer* renderer)
{
	if(bodies.empty()) return;
	selectedBody = &(*bodies[selected % bodies.size()]);

	for(int i = 0; i < bodies.size(); i++) {
		for(int j = i + 1; j < bodies.size(); j++) {
			auto poly1 = bodies[i].get();
			auto poly2 = bodies[j].get();

			std::vector<phy::vec3> simplex;
			if(gjk_collision(poly1, poly2, simplex)) {
				float depth = 0.0f;
				phy::vec3 normal;
				epa_collision(poly1, poly2, simplex, normal, depth);
				contactPoints = clipEdge(poly1, poly2, normal);
				std::cout << contactPoints.size() << std::endl;
				auto displ = normal * depth * 0.5f;
				poly1->pos -= displ;
				poly2->pos += displ;
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
					selected = std::max(0, selected - 1);
					break;
				case SDLK_RIGHT:
					selected++;
					break;
				case SDLK_UP:
					selectedBody->theta++;
					break;
				case SDLK_DOWN:
					selectedBody->theta--;
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

	auto window = SDL_CreateWindow("GJK Remastered", W, H, 0);
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


void renderBody(SDL_Renderer* renderer, phy::PolygonRb* polygon)
{
	auto rotation = polygon->getRotation();
	for(int i = 0; i < polygon->vertices.size(); i++) {
		auto v1 = polygon->pos + polygon->vertices[i].rotate(rotation);
		auto v2 = polygon->pos + polygon->vertices[(i+1)%polygon->vertices.size()].rotate(rotation);
		SDL_RenderLine(renderer, v1.x, v1.y, v2.x, v2.y);
	}

	auto p0 = polygon->pos + polygon->vertices[0].rotate(rotation);
	SDL_RenderLine(renderer, polygon->pos.x, polygon->pos.y, p0.x, p0.y);
}


void renderBody(SDL_Renderer* renderer, phy::CircleRb* body)
{
	drawStrokedCircle(renderer, body->pos.x, body->pos.y, body->radius);
	auto rPos = body->pos + phy::vec3::fromPolarCoord(body->getRotation(), body->radius);
	SDL_RenderLine(renderer, body->pos.x, body->pos.y, rPos.x, rPos.y);
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
	bodies.back()->setRotation(randRange(0.0f, 360.0f));
}


phy::PolygonRb* addPolygon(const float& x, const float& y, const float& segmentCount, const float& radius)
{

	std::vector<phy::vec3> vertices;

	const int MAX_VERTEX = 360;
	const int ANG_INC = MAX_VERTEX / segmentCount;
	for(int i = 0; i < MAX_VERTEX; i += ANG_INC) {
		const float a = i * std::numbers::pi / 180.0f;
		vertices.push_back({ std::cos(a) *  radius, std::sin(a) * radius });
	}

	bodies.push_back(std::make_unique<phy::PolygonRb>());
	bodies.back()->vertices = vertices;
	bodies.back()->pos = { x, y };

	return dynamic_cast<phy::PolygonRb*>(&(*bodies.back()));
}