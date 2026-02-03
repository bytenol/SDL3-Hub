#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <list>
#include <chrono>
#include <random>
#include <memory>

#include "./include/phy/geometry.h"
#include "./include/phy/quadtree.h"

using namespace std;

SDL_Renderer* renderer;
constexpr int W = 1024;
constexpr int H = 640;
std::chrono::high_resolution_clock::duration t0;

float randRange(const float& min, const float& max);
bool processEvent(SDL_Event& evt);
void makeBlock(const float& x, const float& y, const float& w, const float& h);
void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);

template<typename T>
void renderQuadtree(SDL_Renderer* renderer, const phy::Quadtree<T>& qtree);

bool pointInRect(const SDL_FPoint& point, const SDL_FRect& rect);
bool rectToRectIntersect(const SDL_FRect& a, const SDL_FRect& b);

std::vector<phy::Rect2D> objects;
std::vector<SDL_Color> colors;
phy::Quadtree<phy::Rect2D> qtree;


void init()
{

    for(int i = 0; i < 4; i++) {
        makeBlock(randRange(50, W - 100), randRange(50, H - 100), randRange(20, 50), randRange(20, 50));
    }

    t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
}

void render(SDL_Renderer* renderer)
{
    renderQuadtree(renderer, qtree);
}


void update(float dt, SDL_Renderer* renderer)
{
    qtree.resize(phy::Rect2D{ {0, 0}, {W, H}}, 4);

    for(auto& object: objects) qtree.insert(&object);
}

bool processEvent(SDL_Event& evt) {
	switch(evt.type) {
		case SDL_EVENT_QUIT:
			return true;
		case SDL_EVENT_KEY_DOWN:
            
            break;

        case SDL_EVENT_MOUSE_MOTION:
            
            // const float px = evt.motion.x;
            // const float py = evt.motion.y;
            // selectedBall->pos = { px, py };
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            const float px = evt.motion.x;
            const float py = evt.motion.y;
            makeBlock(px, py, randRange(20, 50), randRange(20, 50));
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

	auto window = SDL_CreateWindow("QuadTree", W, H, 0);
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

float randRange(const float& min, const float& max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
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


template<typename T>
void renderQuadtree(SDL_Renderer* renderer, const phy::Quadtree<T>& qtree)
{
    SDL_FRect rect;
    auto& allObjects = qtree.getObjects();

    for(int i = 0; i < allObjects.size(); i++) 
    {
        auto& object = allObjects[i];
        auto& color = colors[i];
        rect.x = object->pos.x;
        rect.y = object->pos.y;
        rect.w = object->size.x;
        rect.h = object->size.y;
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
        SDL_RenderFillRect(renderer, &rect);    
    }


    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    auto& boundary = qtree.getBoundary();
    rect = { boundary.pos.x, boundary.pos.y, boundary.size.x, boundary.size.y };
    SDL_RenderRect(renderer, &rect);

    for(auto& child: qtree.getChildren()) {
        renderQuadtree(renderer, *child);
    }

}


void makeBlock(const float& x, const float& y, const float& w, const float& h)
{
    phy::Rect2D rect;
    rect.pos.x = x;
    rect.pos.y = y;
    rect.size = { w, h };
    objects.push_back(rect);
    colors.push_back(SDL_Color{ (unsigned char)randRange(0, 255), (unsigned char)randRange(0, 255), (unsigned char)randRange(0, 255) });
}
