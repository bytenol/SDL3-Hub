#include <vector>
#include <cmath>
#include <cassert>
#include <SDL3/SDL.h>

SDL_Renderer* renderer;
SDL_Event evt;

struct Point2D;
struct Dot;
struct Stick;
class PhysicsEntity;

std::vector<Dot> dots;
std::vector<Stick> sticks;
std::vector<PhysicsEntity> bodies;

void update(double dt);
void draw();
void mainLoop();
void setupBodies();
void init();
void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);


int main()
{
	init();
	setupBodies();
	mainLoop();

	return 0;
}


struct Point2D
{
	float x, y;
};


struct Dot
{

	float mass = 1.0f;
	Point2D pos, oldPos, vel;

public:
	Dot(const Point2D& _pos, float _mass = 1.0f)
	{
		mass = _mass;
		pos = oldPos = _pos;
		vel = Point2D(0, 0);
	}

	void update(double dt)
	{
		const float g = 0.2f;

		vel.y += g * dt;

		if (pos.y >= 640)
		{
			pos.y = 640;
			vel.y *= -0.8f;
		}

		pos.x += vel.x * dt;
		pos.y += vel.y * dt;
	}

	void wallConstraint()
	{
		if (pos.x > 480 - 3) {
			pos.x = 480 - 3;
		}
		if (pos.x < 3) {
			pos.x = 3;
		}
		if (pos.y > 640 - 3) {
			pos.y = 640 - 3;
		}
		if (pos.y < 3) {
			pos.y = 3;
		}
	}

	void render(SDL_Renderer* renderer) const
	{
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		drawFilledCircle(renderer, pos.x, pos.y, 3.0f);
	}

};


struct Stick
{
private:
	Dot *dot1 = nullptr, *dot2 = nullptr;
	float length;

public:
	Stick(int d1, int d2, float l)
	{
		dot1 = &(dots.at(d1));
		dot2 = &(dots.at(d2));
		length = l;
	}

	void update(double dt)
	{
		float dx = dot2->pos.x - dot1->pos.x;
		float dy = dot2->pos.y - dot1->pos.y;
		float dist = std::sqrt(dx * dx + dy * dy);

		float diffl = (length - dist) * 0.5f;

		dx /= dist * diffl;
		dy /= dist * diffl;

		//assert(dist >= 0);
		//SDL_Log("%s", );
		//assert(std::abs(dx) >= 0);
		//assert(std::abs(dy) >= 0);
		//dot1->pos.x -= dx;
		//dot1->pos.y -= dy;
		//dot2->pos.x += dx;
		//dot2->pos.y += dy;
	}

	void render(SDL_Renderer* renderer)
	{
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderLine(renderer, dot1->pos.x, dot1->pos.y, dot2->pos.x, dot2->pos.y);
	}
};


class PhysicsEntity
{
	using vertices_t = std::vector<Point2D>;

	float mass;
	Point2D pos, oldPos;
	vertices_t modelVertices, vertices;
	float scale = 50.0f;

public:
	PhysicsEntity(const Point2D& _pos, float _mass, const vertices_t& _vertices)
	{
		oldPos = pos = _pos;
		mass = _mass;
		modelVertices = vertices =  _vertices;
	}
};

void update(double dt)
{
	for (auto& dot : dots)
	{
		dot.update(dt);
		//dot.wallConstraint();
	}

	for (auto& stick : sticks)
		stick.update(dt);
}


void draw()
{
	for (auto& dot : dots)
		dot.render(renderer);

	for (auto& stick : sticks)
		stick.render(renderer);
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
		const double dt = 1.0f / 60;
		update(dt);
		SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
		SDL_RenderClear(renderer);
		draw();
		SDL_RenderPresent(renderer);
	}
}

void setupBodies()
{
	dots.push_back({ {100.0f, 0.0f} });
	dots.push_back({ {100.0f, 100.0f} });

	sticks.push_back({ 0, 1, 100.0f });
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