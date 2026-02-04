/*
TODO: once ball rect has intersect with a static ball, just ignore other collisions
*/
#include <iostream>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <variant>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "./include/phy/vec2.h"

constexpr int W = 2048 * 0.5;
constexpr int H = 1156 * 0.5;
constexpr float fixedTimeStep = 1.0f / 240.0f;
float fixedTimeAccumulator = 0.0f;

std::chrono::high_resolution_clock::duration t0;

class Texture;

bool init();
Texture loadTexture(const std::string& path);
void reset();
void update(const float& dt);
void render(SDL_Renderer* renderer);
void pollEvent(SDL_Event& evt);
void animate();
void initBalls();
void initWalls();
float randRange(const float& min, const float& max);
void drawImage(SDL_Renderer* renderer, const Texture& texture, const float& x, const float& y, const float& w, const float& h);
void drawFilledCircle(SDL_Renderer* renderer, const float& x, const float& y, const float& radius);

struct
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	bool windowShouldClose = false;
	SDL_Event evt;
} canvas;

struct Texture {
	SDL_Texture* tex = nullptr;
	float w, h;
};

std::map<std::string, Texture> textures;

struct AABB
{
	phy::vec2 pos;
	phy::vec2 size;

	bool intersect(const AABB& other)
	{
		return pos.x <= other.pos.x && pos.x + size.x >= other.pos.x + other.size.x 
		&& pos.y <= other.pos.y && pos.y + size.y >= other.pos.y + other.size.y; 
	}
};

std::vector<AABB> walls;

enum class BodyType 
{
	DEFAULT,
	BALL,
	WALL
};

enum class WallPos
{
	INCLINED,
	TOP,
	BOTTOM,
	LEFT,
	RIGHT
};

struct Vertex
{
	phy::vec2 pos, vel;
	float mass = 1.0f;
	bool isStatic = false;

	BodyType type = BodyType::DEFAULT;

	Vertex() = default;
	virtual ~Vertex() = default;
	virtual AABB getBoundary() = 0;
};

struct Wall: Vertex
{
	phy::vec2 start, end;
	WallPos position;

	Wall(): Vertex() {
		type = BodyType::WALL;
		position = WallPos::INCLINED;
		isStatic = true;
	}

	AABB getBoundary() override {
		return { start, end - start };
	};
};

struct Ball: public Vertex 
{
	float radius = 10.0f;
	int textureId = -1;

	Ball(): Vertex() {
		type = BodyType::BALL;
	}

	AABB getBoundary() override {
		return { phy::vec2{ pos.x - radius, pos.y - radius}, phy::vec2{ radius * 2.0f, radius * 2.0f } };
	};
};

Ball* selectedBall = nullptr;
struct {
	phy::vec2 pos;
	bool isActive = false;
} mouse;
std::vector<Ball*> balls;


class PhysicsWorld
{
	using body_type = std::unique_ptr<Vertex>;
	std::vector<body_type> bodies;
	const float dragFactor = 0.2f;
	const float wallFriction = 0.9f;

	// body_type* selected = nullptr;
	// std::variant<Ball*, Wall*> bodyPtr;

	public:

	template<typename T, typename... Args>
	T& createObject(Args&&... args) {
		static_assert(std::is_base_of_v<Vertex, T>);
		auto body = std::make_unique<T>(std::forward<Args>(args)...);
		T& ref = *body;
		bodies.push_back(std::move(body));
		return ref;
	}

	void update(const float& dt) 
	{
		// update position
		int i = 0;
		for(auto& body: bodies) {
			if(body->isStatic) continue;
			body->pos += body->vel * dt;
			if(body->vel.length() < 0.01f) {
				body->vel.x = 0;
				body->vel.y = 0;
			}
			i++;
		}

		// get collision
		for(int i = 0; i < bodies.size(); i++) {
			auto& body1 = bodies[i];
			if(body1->type != BodyType::BALL || body1->isStatic) continue;

			for(int j = i + 1; j < bodies.size(); j++) {
				auto& body2 = bodies[j];
				auto b1 = body1->getBoundary();
				auto b2 = body2->getBoundary();
				if(b1.pos.x < b2.pos.x + b2.size.x &&
					b1.pos.x + b1.size.x > b2.pos.x && 
					b1.pos.y < b2.pos.y + b2.size.y && 
					b1.pos.y + b1.size.y > b2.pos.y
				) {
					ballToBallCollisionResolve(body1, body2);
					ballToWallCollisionResolve(body1, body2);
				}
			}
		}

		// update force, acc, velocity
		for(auto& body: bodies) {
			if(body->isStatic) continue;
			auto force = body->vel * -dragFactor;
			auto acc = force * (1 / body->mass);
			body->vel += acc * dt;
		}
	}

	void render(SDL_Renderer* renderer) 
	{
		Ball* ball = nullptr;
		Wall* wall = nullptr;

		int i = 0;
		for(auto& body: bodies) {
			switch(body->type) {
				case BodyType::BALL:
					SDL_SetRenderDrawColor(renderer, 255, 100, 0, 255);
					if(body->isStatic)
						SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
					ball = dynamic_cast<Ball*>(&(*body));
					drawFilledCircle(renderer, ball->pos.x, ball->pos.y, ball->radius);
					break;
				case BodyType::WALL:
					SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
					wall = dynamic_cast<Wall*>(&(*body));
					SDL_RenderLine(renderer, wall->start.x, wall->start.y, wall->end.x, wall->end.y);
					break;
			}

			// SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			// auto b = body->getBoundary();
			// SDL_FRect rect{ b.pos.x, b.pos.y, b.size.x, b.size.y };
			// SDL_RenderRect(renderer, &rect);
			i++;
		}
	}

	size_t size() {
		return bodies.size();
	}

	private:
		void ballToWallCollisionResolve(body_type& body1, body_type& body2)
		{
			if(body2->type != BodyType::WALL)
				return;
			
			auto ball = dynamic_cast<Ball*>(&(*body1));
			auto wall = dynamic_cast<Wall*>(&(*body2));

			if(wall->position == WallPos::TOP) {
				if(body1->pos.y - ball->radius < wall->start.y) {
					body1->pos.y = wall->start.y + ball->radius;
					ball->vel.y *= -wallFriction;
				}
				return;
			}

			if(wall->position == WallPos::BOTTOM) {
				if(body1->pos.y + ball->radius > wall->start.y) {
					body1->pos.y = wall->start.y - ball->radius;
					ball->vel.y *= -wallFriction;
				}
				return;
			}

			if(wall->position == WallPos::LEFT) {
				if(body1->pos.x - ball->radius < wall->start.x) {
					body1->pos.x = wall->start.x + ball->radius;
					ball->vel.x *= -wallFriction;
				}
				return;
			}

			if(wall->position == WallPos::RIGHT) {
				if(body1->pos.x + ball->radius > wall->start.x) {
					body1->pos.x = wall->start.x - ball->radius;
					ball->vel.x *= -wallFriction;
				}
				return;
			}

		}

		void ballToBallCollisionResolve(body_type& body1, body_type& body2)
		{
			if(body1->type != BodyType::BALL || body2->type != BodyType::BALL)
				return;

			auto b1 = dynamic_cast<Ball*>(&(*body1));
			auto b2 = dynamic_cast<Ball*>(&(*body2));

			auto dist = body2->pos - body1->pos;
			auto distLen = dist.length();
			auto maxRadius = b1->radius + b2->radius;

			if(body2->isStatic && distLen < maxRadius * 0.5f) {
				body1->vel *= 0.0f;
				// body1->pos = body2->pos;
				std::cout << "Hello wotld" << std::endl;
				return;
			}

			if(distLen < maxRadius) {

				auto normal = dist * (1 / distLen);
				const float depth = maxRadius - distLen;
				auto displ = normal * depth * 0.5f;
				body1->pos -= displ;
				body2->pos += displ;

				// before impact
				auto normalVel1 = normal * body1->vel.dotProduct(normal);
				auto normalVel2 = normal * body2->vel.dotProduct(normal);

				auto tangentVel1 = body1->vel - normalVel1;
				auto tangentVel2 = body2->vel - normalVel2;

				float u1 = normalVel1.dotProduct(normal);
				float u2 = normalVel2.dotProduct(normal);
				float m1 = body1->mass;
				float m2 = body2->mass;
				float tm = m1 + m2;

				float v1 = ((m1-m2) * u1 + 2 * m2 * u2) / tm;
				float v2 = ((m2-m1) * u2 + 2 * m1 * u1) / tm;

				normalVel1 = normal * v1;
				normalVel2 = normal * v2;

				body1->vel = normalVel1 + tangentVel1;
				body2->vel = normalVel2 + tangentVel2;
			}
			
		}

} world;


template<typename T>
class Quadtree
{
	AABB boundary;
	int capacity, minArea;
	std::vector<T*> objects;
	std::vector<std::unique_ptr<Quadtree<T>>> children;

	public:
		Quadtree() = default;

		void resize(const AABB& b, const int& c = 4) 
		{
			boundary = b;
			capacity = c;
		}

		void insert(T* object) 
		{
			objects.push_back(object);

			std::cout << objects.size() << std::endl;
		}

		void render(SDL_Renderer* renderer)
		{
			auto p = boundary.pos - boundary.size * 0.5f;
			SDL_FRect rect{ p.x, p.y, boundary.size.x, boundary.size.y };
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			SDL_RenderRect(renderer, &rect);

			SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
			for(auto& obj: objects) 
			{
				auto p = obj->center - (obj->size * 0.5f);
				// SDL_FRect rect{ p.x, p.y, obj->size.x, obj->size.y };
				// SDL_RenderFillRect(renderer, &rect);
				
			}

			for(auto& child: children)
				child->render(renderer);
		}
};

Quadtree<Vertex> qtree;

void update(const float& dt)
{
	selectedBall = balls[0];
}


void render(SDL_Renderer* renderer)
{
	drawImage(renderer, textures["table"], 0, 0, W, H);

	for(auto& ball: balls) {
		auto& texture = textures["ball_"+std::to_string(ball->textureId)];
		SDL_FRect srcRect { 0, 0, texture.w, texture.h };
		SDL_FRect dstRect{ ball->pos.x - ball->radius, ball->pos.y - ball->radius, ball->radius * 2.0f, ball->radius * 2.0f };
		SDL_RenderTexture(renderer, texture.tex, &srcRect, &dstRect);
	}

	if(mouse.isActive) {
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderLine(renderer, selectedBall->pos.x, selectedBall->pos.y, mouse.pos.x, mouse.pos.y);
	}

	// auto& cueTex = textures["cue"];
	// drawImage(renderer, textures["cue"], mouse.pos.x, mouse.pos.y, W, H);

	// world.render(renderer);
}


bool init()
{
	// qtree.resize({ phy::vec2{ 0, 0 }, phy::vec2{ W, H } })

	// auto& ball1 = world.createObject<Ball>();
	// ball1.radius = 50.0f;
	// ball1.
	initBalls();
	initWalls();
	

	return true;
}


int main()
{
	canvas.window = SDL_CreateWindow("EightBall", W, H, 0);
	canvas.renderer = SDL_CreateRenderer(canvas.window, nullptr);

	if (!canvas.window || !canvas.renderer)
	{
		SDL_Log("Error creating canvas window or renderer: %s", SDL_GetError());
		return -1;
	}

	textures["table"] = loadTexture("/table.png");
	textures["triangle"] = loadTexture("/triangle.png");
	textures["cue"] = loadTexture("/cue.png");

	for(int i = 1; i <= 16; i++) {
		auto name = std::string("ball_") + std::to_string(i);
		textures[name] = loadTexture("/"+name + ".png");
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
			phy::vec2 mouse{ evt.motion.x, evt.motion.y };
			auto dist = selectedBall->pos - mouse;
			if(dist.length() < selectedBall->radius) {
				::mouse.isActive = true;
			}
		}

		if(evt.type == SDL_EVENT_MOUSE_MOTION)
		{
			mouse.pos.x = evt.motion.x;
			mouse.pos.y = evt.motion.y;
			
		}

		if(evt.type == SDL_EVENT_MOUSE_BUTTON_UP) {
			if(!mouse.isActive) return;
			mouse.isActive = false;
			auto vel = selectedBall->pos - mouse.pos;
			constexpr float maxSpeed = 200.0f;
			auto nVel = vel.normalize();
			if(vel.length() * 0.5f > maxSpeed) 
				vel = nVel * maxSpeed;
			selectedBall->vel = vel;
			selectedBall = nullptr;
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


void initBalls()
{
	auto createBall = [](const float& x, const float& y, const float& r, const bool& s = false) -> Ball&
	{
		auto& ball = world.createObject<Ball>();
		ball.pos = phy::vec2{ x, y };
		ball.radius = r;
		ball.isStatic = s;

		return ball;
	};
	
	float rad = textures["ball_1"].w * 0.12f;
	auto& cueBall = createBall(W * 0.85f, H * 0.5f, rad);
	cueBall.textureId = 16;
	balls.push_back(&cueBall);

	int bCount = 1;
	float x0 = W * 0.40f;
	float y0 = H * 0.50f;
	const float rad2 = rad * 2.0f;
	
	for(int i = 0; i < 5; i++) {
		for(int j = 0; j < i + 1; j++) {
			float px = x0 - (i * rad2);
			float py = y0 - (i * rad) + j * rad2;
			auto& ball = createBall(px, py, rad);
			ball.textureId = bCount;
			balls.push_back(&ball);
			bCount++;
		}
	}

	rad = rad * 1.25f;
	// left
	createBall(W * 0.04492f, H * 0.09343f, rad, true);
	createBall(W * 0.04492f, H * (1 - 0.09343f), rad, true);
	// middle
	createBall(W * 0.4951f, H * 0.06920f, rad, true);
	createBall(W * 0.4951f, H * (1 - 0.06920f), rad, true);
	// right
	createBall(W * 0.9463f, H * 0.09343f, rad, true);
	createBall(W * 0.9463f, H * (1 - 0.09343f), rad, true);
}


void initWalls()
{
	walls.clear();

	auto createWall = [](const float& sx, const float& sy, const float& ex, const float& ey, const WallPos& p)
	{
		auto& wall = world.createObject<Wall>();
		wall.start = phy::vec2{ sx, sy };
		wall.end = phy::vec2{ ex, ey };
		wall.position = p;
	};

	// create walls
	constexpr float wallWidth = W * (0.4620f - 0.0947f);
	constexpr float wallHeight = H * 0.1160f;
	
	// top-left
	constexpr float walltopX1 = W * 0.0947f;
	createWall(walltopX1, wallHeight, walltopX1 + wallWidth, wallHeight, WallPos::TOP);
	
	// top-right
	constexpr float walltopX2 = W * 0.5283f;
	createWall(walltopX2, wallHeight, walltopX2 + wallWidth, wallHeight, WallPos::TOP);

	// bottom right
	createWall(walltopX2, H - wallHeight, walltopX2 + wallWidth, H - wallHeight, WallPos::BOTTOM);
	createWall(walltopX1, H - wallHeight, walltopX1 + wallWidth, H - wallHeight, WallPos::BOTTOM);

	// left
	constexpr float wallLeftY = H * 0.1782f;
	createWall(wallHeight, wallLeftY, wallHeight, wallHeight + wallWidth * 1.08f, WallPos::LEFT);

	// right
	createWall(W - wallHeight, wallLeftY, W - wallHeight, wallHeight + wallWidth * 1.08f, WallPos::RIGHT);
}

Texture loadTexture(const std::string& path)
{
	Texture texture;

	std::string filePath = __FILE__;
	auto assetRoot = std::filesystem::path(filePath).parent_path().parent_path().string() + "/assets/eightball" + path;
	const char* spritePath = assetRoot.c_str();

	texture.tex = IMG_LoadTexture(canvas.renderer, spritePath);
	if (!texture.tex) {
		SDL_Log("Failed to load image: %s", SDL_GetError());
		return texture;
	}

	SDL_GetTextureSize(texture.tex, &(texture.w), &(texture.h));
	return texture;
}


void drawImage(SDL_Renderer* renderer, const Texture& texture, const float &x, const float &y, const float &w, const float &h)
{
	SDL_FRect srcRect{ 0, 0, texture.w, texture.h };
	SDL_FRect dstRect{ x, y, w, h };
	SDL_RenderTexture(renderer, texture.tex, &srcRect, &dstRect);
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