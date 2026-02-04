#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <chrono>
#include <variant>
#include <SDL3/SDL.h>

#include "./include/phy/vec2.h"

constexpr int W = 640;
constexpr int H = 480;
constexpr float fixedTimeStep = 1.0f / 60.0f;
float fixedTimeAccumulator = 0.0f;

std::chrono::high_resolution_clock::duration t0;

bool init();
void update(const float& dt);
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

struct AABB
{
	phy::vec2 pos, size;
};

enum class BodyType
{
	DEFAULT,
	PARTICLE
};


struct PhysicsObject
{
	phy::vec2 pos, lastPos, acc;
	float radius = 1.0f;
	float mass = 1.0f;
	virtual ~PhysicsObject() = default;
};

struct Particle: PhysicsObject
{

};


struct Stick
{
	PhysicsObject *vertA, *vertB;
	float length;
};


class PhysicsWorld
{
	using body_type = std::unique_ptr<PhysicsObject>;

	std::vector<std::unique_ptr<PhysicsObject>> vertices;
	std::vector<Stick> sticks;

	public:

	template<typename T, typename... Args>
	T& createObject(Args&&... args) {
		static_assert(std::is_base_of_v<PhysicsObject, T>);
		auto vertex = std::make_unique<T>(std::forward<Args>(args)...);
		T& ref = *vertex;
		vertices.push_back(std::move(vertex));
		return ref;
	}

	void addStick(const int& a, const int& b, const float& length)
	{
		sticks.push_back({ &(*vertices[a]), &(*vertices[b]), length });
	}

	void update(const float& dt) 
	{
		calcAcceleration();
		integratePosition(dt);
		
		for(int i = 0; i < 5; i++) {
			solveConstraints();
			solveBoundaryConstraint();
		}
	}

	void setBoundary(const float& x, const float& y, const float& w, const float& h)
	{
		boundary.pos.x = x; boundary.pos.y = y;
		boundary.size.x = w; boundary.size.y = h;
	}


	void render(SDL_Renderer* renderer) 
	{

		SDL_FRect rect{ boundary.pos.x, boundary.pos.y, boundary.size.x, boundary.size.y };
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderRect(renderer, &rect);

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		for(auto& vertex: vertices)
			drawFilledCircle(renderer, vertex->pos.x, vertex->pos.y, vertex->radius);

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		for(auto& stick: sticks) {
			auto& v1 = stick.vertA->pos;
			auto& v2 = stick.vertB->pos;
			SDL_RenderLine(renderer, v1.x, v1.y, v2.x, v2.y);
		}

	}


	size_t size() {
		return vertices.size();
	}


	private:
		AABB boundary;
		float g = 50.0f;
		float damping = 0.98f;
		float groundFriction = 0.85f;

		void solveBoundaryConstraint() {
			for(auto& vertex: vertices)
			{
				auto vel = (vertex->pos - vertex->lastPos) * damping;
				if(vertex->pos.y + vertex->radius > boundary.pos.y + boundary.size.y)
				{
					vertex->pos.y = boundary.pos.y + boundary.size.y - vertex->radius;
					vertex->lastPos.y = vertex->pos.y + vel.y * groundFriction;
				}

				if(vertex->pos.x - vertex->radius < 0)
				{
					vertex->pos.x = vertex->radius;
					vertex->lastPos.x = vertex->pos.x + vel.x * groundFriction;
				} else if(vertex->pos.x + vertex->radius > boundary.pos.x + boundary.size.x) {
					vertex->pos.x = boundary.pos.x + boundary.size.x - vertex->radius;
					vertex->lastPos.x = vertex->pos.x + vel.x * groundFriction;
				}
			}
		}

		void solveConstraints()
		{
			for(auto& stick: sticks)
			{
				auto delta = stick.vertB->pos - stick.vertA->pos;
				float dist = delta.length();
				if(dist == 0.0f) continue;
				auto normal = delta * (1 / dist);
				float diff = (dist - stick.length) * 0.5f;

				float wA = 1 / stick.vertA->mass;
				float wB = 1 / stick.vertB->mass;
				float sum = wA + wB;

				stick.vertA->pos += normal * diff * (wA / sum);
				stick.vertB->pos -= normal * diff * (wB / sum);
			}
		}

		void integratePosition(const float& dt) 
		{
			for(auto& vertex: vertices) {
				auto tmp = vertex->pos;
				auto vel = (vertex->pos - vertex->lastPos) * damping;
				vertex->pos += vel + vertex->acc * (dt * dt);
				vertex->acc = phy::vec2{ 0, 0 };
				vertex->lastPos = tmp;
			}
		}

		void calcAcceleration() {
			for(auto& vertex: vertices) {
				phy::vec2 weight { 0, vertex->mass * g };
				auto force = weight;
				vertex->acc += force * (1 / vertex->mass);
			}
		}

} world;


void update(const float& dt)
{
	
}


void render(SDL_Renderer* renderer)
{
	world.render(renderer);
}


bool init()
{
	world.setBoundary(0.0f, 0.0f, W, H);

	auto createParticle = [](const float& px, const float& py, const float& r = 3.0f) -> Particle& {
		auto& p = world.createObject<Particle>();
		p.pos = phy::vec2{ px, py };
		p.lastPos = p.pos;
		p.radius = r;

		return p;
	};

	for(int i = 0; i < 1; i++)
	{

		createParticle(randRange(30, W - 50), randRange(0, 1), 20);
	}

	auto& a = createParticle(100, 0);
	a.lastPos.y = -5.0f;
	createParticle(150, 0);
	createParticle(150, 50);
	createParticle(100, 50);

	world.addStick(1, 2, 50);
	world.addStick(2, 3, 50);
	world.addStick(3, 4, 50);
	world.addStick(4, 1, 50);
	world.addStick(1, 3, std::hypot(50, 50));
	world.addStick(2, 4, std::hypot(50, 50));

	return true;
}


int main()
{
	canvas.window = SDL_CreateWindow("Verlet Integration", W, H, 0);
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

		if(evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
		{
		
		}

		if(evt.type == SDL_EVENT_MOUSE_MOTION)
		{
			
		}

		if(evt.type == SDL_EVENT_MOUSE_BUTTON_UP) {

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
			world.update(fixedTimeStep);
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