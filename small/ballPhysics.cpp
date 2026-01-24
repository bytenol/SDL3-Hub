/*
* @file ballPhysics.cpp
* @date 19th Jan, 2026
*/
#include <iostream>
#include <vector>
#include <cmath>
#include <SDL3/SDL.h>
#include <chrono>
#include <random>

#include "./include/phy/vec2.h"
#include "./include/phy/geometry.h"

using namespace phy;

SDL_Renderer* renderer;
constexpr int W = 1024;
constexpr int H = 640;
constexpr float fixedTimeStep = 1.0f / 6.0f;
float timeAccumulator = 0.0f;
std::chrono::high_resolution_clock::duration t0;

float randRange(const float& min, const float& max);
void physicsProcess(const float& dt);
void update(const float& dt, SDL_Renderer* renderer);
bool processEvent(SDL_Event& evt);
void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);
bool pointInRect(const SDL_FPoint& point, const SDL_FRect& rect);
bool rectToRectIntersect(const SDL_FRect& a, const SDL_FRect& b);

class Ball;


template<typename T>
class Quadtree {

    int capacity = 4;
    SDL_FRect boundary;

    public:
        std::vector<T> objects;
        bool isDivided = false;

        std::vector<Quadtree> children;

        Quadtree() = default;

        Quadtree(const SDL_FRect& r, int maxObj = 4) {
            boundary = r;
            capacity = maxObj;
        }

        void insert(const T& object) {
            if(!pointInRect(SDL_FPoint{ object->pos.x, object->pos.y }, boundary)) return;

            if(objects.size() < capacity) {
                objects.emplace_back(object);
            } else {
                if(!isDivided) {
                    isDivided = true;
                    const float wHalf = boundary.w / 2;
                    const float hHalf = boundary.h / 2;
                    children.push_back(Quadtree({ boundary.x, boundary.y, wHalf, hHalf }, capacity));
                    children.push_back(Quadtree({ boundary.x + wHalf, boundary.y, wHalf, hHalf }, capacity));
                    children.push_back(Quadtree({ boundary.x, boundary.y + hHalf, wHalf, hHalf }, capacity));
                    children.push_back(Quadtree({ boundary.x + wHalf, boundary.y + hHalf, wHalf, hHalf }, capacity));

                    for(auto it = objects.begin(); it != objects.end(); it++) {
                        for(int i = 0; i < children.size(); i++) {
                            auto& child = children[i];
                            child.insert(*it);
                        }
                    }

                    objects.clear();

                }   // if !isDivided ends
            }

            if(isDivided) {
                for(auto& child: children) {
                    child.insert(object);
                }
            }

        }

        std::vector<T> findObject(const decltype(boundary)& range) {
            if(!rectToRectIntersect(boundary, range)) return {};

            std::vector<T> res;

            if(!isDivided) {
                for(auto& object: objects) {
                    if(pointInRect({ object->pos.x, object->pos.y }, range)) res.emplace_back(object);
                }
                return res;
            }

            else {
                for(auto& child: children) {
                    auto pt = child.findObject(range);
                    res.insert(res.begin(), pt.begin(), pt.end());
                }
            }

            return res;
        }

        size_t size() const {
            return objects.size();
        }

        void render(SDL_Renderer* renderer) {
            SDL_RenderRect(renderer, &boundary);
            for(auto& child: children) 
                child.render(renderer);

            for(auto& object: objects) {
                drawFilledCircle(renderer, object->pos.x, object->pos.y, 1);
            }
        }
};

Quadtree<Ball*> qtree;


int selectedIndex = 0;
Ball* selectedBall = nullptr;
std::vector<Ball> balls;

class Ball {
    float g = 10;

    public:
        vec2 pos{0, 0}, 
            vel{ 0, 0 }, 
            force{ 0, 0}, 
            acc{ 0, 0 };

        float mass, radius;

        SDL_Color color;

        Ball(const vec2& p, const float& r, const float& m) {
            pos = p;
            mass = m;
            radius = r;

            color.r = randRange(0, 255);
            color.g = randRange(0, 255);
            color.b = randRange(0, 255);
        }

        void update(const float& dt, std::vector<Ball*>& balls) {
            force.x = 0;
            force.y = 0;
            checkWallBounce();
            ballToBallCollision(balls);
            pos += vel * dt;
            vec2 friction{ vel * -0.2f };
            vec2 weight { 0, mass * g };

            force += weight + friction;
            acc = force * (1/mass);
            vel += acc * dt;
        }

        void render(SDL_Renderer* renderer) {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
            if(this == selectedBall) 
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            drawFilledCircle(renderer, pos.x, pos.y, radius);
        }

    private:
        void checkWallBounce() {
            if(pos.y + radius > H) {
                pos.y = H - radius;
                vel.y *= -0.8f;
                force -= { 0, mass * g };
            }

            if(pos.x - radius < 0) {
                pos.x = radius;
                vel.x *= -0.3f;
            }

            if(pos.x + radius > W) {
                pos.x = W - radius;
                vel.x *= -0.3f;
            }
        }

        void ballToBallCollision(std::vector<Ball*>& balls) {
            for(auto& ball: balls) {
                if(ball != this) {
                    auto dist = ball->pos - pos;
                    const float maxRadius = radius + ball->radius;
                    const float dl = dist.length();

                    if(dl < maxRadius) {
                        auto normal = dist.normalize();
                        auto displ = normal * ((maxRadius - dl) * 0.5);
                        pos -= displ;
                        ball->pos += displ;

                        auto normalVel1 = normal * vel.dotProduct(normal);
                        auto normalVel2 = normal * ball->vel.dotProduct(normal);

                        auto tangentVel1 = vel - normalVel1;
                        auto tangetVel2 = ball->vel - normalVel2;

                        const float m1 = mass;
                        const float m2 = ball->mass;
                        const float u1 = normalVel1.dotProduct(normal);
                        const float u2 = normalVel2.dotProduct(normal);
                        
                        const float v1 = ((m1-m2)*u1+2*m2*u2)/(m1+m2);
                        const float v2 = ((m2-m1)*u2+2*m1*u1)/(m1+m2);

                        normalVel1 = normal * v1;
                        normalVel2 = normal * v2;

                        vel = tangentVel1 + normalVel1;
                        ball->vel = tangetVel2 + normalVel2;

                    }
                }
            }
        }
};

float accT = 0;

void init()
{
    balls.clear();

    for(int i = 0; i < 200; i++) {
        const float radius = randRange(10, 20);
        balls.push_back({ {randRange(0, W), randRange(0, 90)}, radius, radius * 0.5f });
    }
    balls.push_back({ {randRange(0, W), 0}, 20, 20 * 0.5f });

    t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
}

void render(SDL_Renderer* renderer)
{
    for(auto& ball: balls) ball.render(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    qtree.render(renderer);
}


void physicsProcess(const float& dt)
{
    selectedBall = &(balls[selectedIndex % balls.size()]);

    qtree = Quadtree<Ball*>({ 0, 0, W, H }, 4);
    for(auto& ball: balls) qtree.insert(&ball);

    for(auto& ball: balls) {
        auto ranged = qtree.findObject({ ball.pos.x - 50, ball.pos.y - 50, 100, 100 });
        ball.update(dt, ranged);
    }
        
}


void update(const float& dt, SDL_Renderer* renderer) 
{
    // accT += dt;
    // if(accT >= 1.5f && balls.size() < 500) {
    //     accT = 0;
    //     const float radius = randRange(10, 20);
    //     balls.push_back({ {randRange(0, W), -radius}, radius, radius * 0.5f });
    // }
}


bool processEvent(SDL_Event& evt) {
	switch(evt.type) {
		case SDL_EVENT_QUIT:
			return true;
		case SDL_EVENT_KEY_DOWN:
            if(!selectedBall) break;

            switch(evt.key.key) {
                case SDLK_LEFT:
                    selectedIndex--;
                    break;
                case SDLK_RIGHT:
                    selectedIndex++;
                    break;
                case SDLK_W:
                    selectedBall->pos.y--;
                    break;
                case SDLK_S:
                    selectedBall->pos.y++;
                    break;
                case SDLK_A:
                    selectedBall->pos.x--;
                    break;
                case SDLK_D:
                    selectedBall->pos.x++;
                    break;
            }
            break;

        case SDL_EVENT_MOUSE_MOTION:
            if(!selectedBall) break;
            const float px = evt.motion.x;
            const float py = evt.motion.y;
            // selectedBall->pos = { px, py };
            break;
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

	auto window = SDL_CreateWindow("Ball Physics", W, H, 0);
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

        timeAccumulator += dt;

        while(timeAccumulator >= fixedTimeStep) {
            physicsProcess(fixedTimeStep);
            timeAccumulator -= fixedTimeStep;
        }

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

bool pointInRect(const SDL_FPoint &point, const SDL_FRect &rect)
{
    return (point.x >= rect.x && point.x <= rect.x + rect.w && 
        point.y >= rect.y && point.y <= rect.y + rect.h);
}

bool rectToRectIntersect(const SDL_FRect &a, const SDL_FRect &b)
{
    return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y);
}


float randRange(const float& min, const float& max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}