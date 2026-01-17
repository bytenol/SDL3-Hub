/*
* @file Archimedes.cpp
* @date 19th Jan, 2025
*/
#include <vector>
#include <cmath>
#include <SDL3/SDL.h>

SDL_Renderer* renderer;
int W = 680;
int H = 480;

SDL_FRect pond;

struct Vec2
{
	float x = 0.0f, y = 0.0f;
	Vec2& operator+=(const Vec2& o)
	{
		x += o.x;
		y += o.y;
		return *this;
	}

	Vec2 operator+(const Vec2& o)
	{
		return { x + o.x, y + o.y };
	}

	Vec2 operator-(const Vec2& o)
	{
		return { x - o.x, y - o.y };
	}

	Vec2 operator*(const float& f)
	{
		return { x * f, y * f };
	}
};

struct Ball
{
	float mass;
	SDL_Color color;
	Vec2 pos;
	Vec2 vel;

	float GetRadius() const
	{
		return 10.0f + mass * 10.0f;
	}
};

std::vector<Ball> balls;

void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);

void init()
{
	pond.x = 0.0f;
	pond.y = H * 0.5f;
	pond.w = W;
	pond.h = H - pond.y;

	balls.push_back({ 1.0f, { 255, 0, 0, 255 } });
	balls[0].pos.x = 50.0f;
	balls[0].pos.y = 20.0f;
	balls[0].vel.x = 40.0f;
}

void render(SDL_Renderer* renderer)
{
	//SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);

	SDL_SetRenderDrawColor(renderer, 0, 110, 200, 10);
	SDL_RenderFillRect(renderer, &pond);

	for (const auto& ball : balls)
	{
		SDL_SetRenderDrawColor(renderer, ball.color.r, ball.color.g, ball.color.b, ball.color.a);
		drawFilledCircle(renderer, ball.pos.x, ball.pos.y, ball.GetRadius());
	}
}


void update(float dt, SDL_Renderer* renderer)
{
	for (auto& ball : balls)
	{
		ball.pos += ball.vel * dt;

		const float g = 10.0f;
		Vec2 weight{ 0, ball.mass * g };
		float density = 1.0f;
		float bVolume = ball.mass / density;

		float airDensity = 0.5f;
		float waterDensity = 1.2f;
		Vec2 upthrust{ 0, 0 };
		Vec2 drag{ 0, 0 };

		float bRadius = ball.GetRadius();
		if (ball.pos.y + bRadius >= pond.y)
		{
			float dist = std::abs((ball.pos.y + bRadius) - pond.y);
			if (dist <= bRadius * 2.0f)
			{
				float sWater = waterDensity * (dist / bRadius) * bVolume * g;
				float sAir = airDensity * ((bRadius - dist) / bRadius) * bVolume * g;
				upthrust.y = sAir + sWater;
				drag = ball.vel * -0.2f * ((dist / bRadius) * bVolume);
			}
			else {
				upthrust.y = waterDensity * bVolume * g;
				drag = ball.vel * -0.2f;
			}
		}
		else {
			upthrust.y = airDensity * bVolume * g;
		}

		Vec2 force, acc;
		force += weight - upthrust + drag;
		acc = force * (1 / ball.mass);
		ball.vel += acc * dt;
	}
}


int main()
{
	if (SDL_Init(SDL_INIT_VIDEO) <= 0)
	{
		SDL_Log("SDL_INITIALIZATION ERROR: %s", SDL_GetError());
		return -1;
	}

	auto window = SDL_CreateWindow("Archimedes", W, H, 0);
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
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);
		render(renderer);
		update(1 / 60.0f, renderer);
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