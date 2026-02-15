/*
TODO: once ball rect has intersect with a static ball, just ignore other collisions
*/
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <chrono>
#include <SDL3/SDL.h>

#include "./include/phy/vec2.h"

constexpr int W = 640;
constexpr int H = 480;
constexpr float fixedTimeStep = 1.0f / 60.0f;
float fixedTimeAccumulator = 0.0f;

std::chrono::high_resolution_clock::duration t0;/*  */


bool init();
void update(const float& dt);
void physicsProcess(const float& dt);
void render(SDL_Renderer* renderer);
void pollEvent(SDL_Event& evt);
void animate();
float randRange(const float& min, const float& max);
void drawFilledCircle(SDL_Renderer* renderer, const float& x, const float& y, const float& radius);

struct
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	bool windowShouldClose = false;
	SDL_Event evt;
} canvas;

struct Particle
{
    phy::vec2 pos, vel, acc, lastPos;
    float mass = 1.0f;
};

constexpr float g = 10.0f;
constexpr float k = 0.2f;
int ropeLength;
phy::vec2 pivot1, pivot2;
std::vector<Particle> particles;
std::vector<std::vector<Particle*>> constraints;

phy::vec2 makeSpringForce(const phy::vec2& a, phy::vec2& b, const float& k)
{
    auto dist = b - a;
    auto l = dist.length();
    float x = l - ropeLength;
    return dist * ((-k * x) / l);
}

void calcAcceleration()
{
    for(int i = 0; i < particles.size(); i++)
    {
        auto& particle = particles[i];
        auto& constraint = constraints[i];

        phy::vec2 weight{ 0.0f, particle.mass * g };
        phy::vec2 spring{ 0.0f, 0.0f };

        for(auto& particle2: constraint)
            spring += makeSpringForce(particle.pos, particle2->pos, k);
        auto force = weight;
        particle.acc += force * (1.0f / particle.mass);
    }
}


void solveConstraint() {
    for(int i = 0; i < particles.size(); i++)
    {
        auto& particle = particles[i];
        auto& constraint = constraints[i];

        for(int j = 0; j < constraint.size(); j++)
        {
            auto& pConst = constraint[j];
            auto delta = pConst->pos - particle.pos;
            float dist = delta.length();
            if(dist == 0.0f) continue;
            auto normal = delta * (1 / dist);
            float diff = (dist - ropeLength) * (0.5f * k);

            float wA = 1 / particle.mass;
            float wB = 1 / pConst->mass;
            float sum = wA + wB;

            particle.pos += normal * diff * (wA / sum);
            pConst->pos -= normal * diff * (wB / sum);
        }
    }
}

void physicsProcess(const float& dt)
{   
    // apply force and calculate acceleration
    calcAcceleration();

    // velocity verlet
    for(auto& particle: particles)
    {
        auto tmp = particle.pos;
        auto vel = (particle.pos - particle.lastPos) * 0.999f;
        particle.pos += vel + particle.acc * (dt * dt);
        particle.acc = { 0.0f, 0.0f };
        particle.lastPos = tmp;
    }


    // solve constraint
    for(int i = 0; i < 5; i++) solveConstraint();
    

}


void update(const float& dt)
{
	particles.front().pos = pivot1;
    particles.front().lastPos = pivot1;
    particles.back().pos = pivot2;
    particles.back().lastPos = pivot2;
}


void render(SDL_Renderer* renderer)
{
    for(int i = 0; i < particles.size(); i++)
    {
        auto& next = particles[(i + 1) % particles.size()];
        auto& particle = particles[i];
        auto p = particle.pos;

        if(i + 1 < particles.size()) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderLine(renderer, p.x, p.y, next.pos.x, next.pos.y);
        }

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        drawFilledCircle(renderer, particle.pos.x, particle.pos.y, 2.0f);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    drawFilledCircle(renderer, pivot1.x, pivot1.y, 3.0f);
    drawFilledCircle(renderer, pivot2.x, pivot2.y, 2.0f);
}


bool init()
{
    pivot1 = { 100.0f, 200.0f };
    pivot2 = { 500.0f, 200.0f };

    int divMax = 50;
    ropeLength = (pivot2.x - pivot1.x) / divMax;
    for(int x = 0; x < divMax; x++) {
        Particle p;
        p.pos = { pivot1.x + (float)x * ropeLength, pivot1.y + randRange(-15.0f, 15.0f) };
        p.lastPos = p.pos + phy::vec2(randRange(-10.0f, 10.0f), randRange(-10.0f, 10.0f));
        p.vel = { 0.0f, 0.0f };
        p.acc = { 0.0f, 0.0f };
        // p.mass = randRange(0.3f, 1.0f);
        particles.push_back(p);
    }

    constraints.push_back({});  // for front
    for(int i = 1; i < particles.size() - 1; i++)
    {
        constraints.push_back({});
        constraints.back().push_back(&particles[i - 1]);
        constraints.back().push_back(&particles[i + 1]);
    }
    constraints.push_back({});  // for back

	return true;
}


int main()
{
	canvas.window = SDL_CreateWindow("SoftBodies", W, H, 0);
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
		const float dt = (t1 - t0).count() * 10e-9;
		t0 = t1;
		fixedTimeAccumulator += dt;
		pollEvent(canvas.evt);
		update(dt);

		while(fixedTimeAccumulator > fixedTimeStep) {
			physicsProcess(fixedTimeStep);
			fixedTimeAccumulator -= fixedTimeStep;
		}

		SDL_SetRenderDrawColor(canvas.renderer, 0, 0, 0, 255);
		SDL_RenderClear(canvas.renderer);
		render(canvas.renderer);
		SDL_RenderPresent(canvas.renderer);
	}
}

void drawFilledCircle(SDL_Renderer* renderer, const float& px, const float& py, const float& radius)
{
    auto drawHorizontalLine = [](SDL_Renderer* renderer, int x1, int x2, int y) -> void {
        for (int x = x1; x <= x2; x++)
            SDL_RenderPoint(renderer, x, y);
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


float randRange(const float& min, const float& max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}