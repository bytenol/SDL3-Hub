#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <list>
#include <chrono>
#include <random>
#include <memory>

#include "./include/phy/geometry.h"

using namespace std;

SDL_Renderer* renderer;
constexpr int W = 1024;
constexpr int H = 640;
std::chrono::high_resolution_clock::duration t0;

float randRange(const float& min, const float& max);
bool processEvent(SDL_Event& evt);
void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);

bool pointInRect(const SDL_FPoint& point, const SDL_FRect& rect);
bool rectToRectIntersect(const SDL_FRect& a, const SDL_FRect& b);


template<typename T>
class Quadtree {

    phy::Rect2D boundary;
    int capacity = 4;
    std::vector<T*> objects;
    bool isDivided = false;
    std::vector<std::unique_ptr<Quadtree>> children;

    public:

        Quadtree() = default;

        void resize(const phy::Rect2D& b, const int& c) {
            boundary = b;
            capacity = c;
            objects.clear();
            isDivided = false;
            children.clear();
        }

        bool insert(T& object) {
            if(object.getArea() > boundary.getArea())
                return false;

            objects.push_back(&object);

        //     if(!pointInRect(point, boundary)) return;

        //     if(objects.size() < capacity) {
        //         objects.emplace_back(point);
        //     } else {
        //         if(!isDivided) {
        //             isDivided = true;
        //             const float wHalf = boundary.w / 2;
        //             const float hHalf = boundary.h / 2;
        //             children.push_back(Quadtree({ boundary.x, boundary.y, wHalf, hHalf }, capacity));
        //             children.push_back(Quadtree({ boundary.x + wHalf, boundary.y, wHalf, hHalf }, capacity));
        //             children.push_back(Quadtree({ boundary.x, boundary.y + hHalf, wHalf, hHalf }, capacity));
        //             children.push_back(Quadtree({ boundary.x + wHalf, boundary.y + hHalf, wHalf, hHalf }, capacity));

        //             for(auto it = objects.begin(); it != objects.end(); it++) {
        //                 for(int i = 0; i < children.size(); i++) {
        //                     auto& child = children[i];
        //                     child.insert(*it);
        //                 }
        //             }

        //             objects.clear();

        //         }   // if !isDivided ends
        //     }

        //     if(isDivided) {
        //         for(auto& child: children) {
        //             child.insert(point);
        //         }
        //     }

            return false;
        }

        std::vector<T> findObject(const decltype(boundary)& range) {
            // if(!rectToRectIntersect(boundary, range)) return {};

            // std::vector<T> res;

            // if(!isDivided) {
            //     for(auto& pt: objects) {
            //         if(pointInRect(pt, range)) res.emplace_back(pt);
            //     }
            //     return res;
            // }

            // else {
            //     for(auto& child: children) {
            //         auto pt = child.findObject(range);
            //         res.insert(res.begin(), pt.begin(), pt.end());
            //     }
            // }

            // return res;
        }

        size_t size() const {
            return objects.size();
        }

        void render(SDL_Renderer* renderer) {
            SDL_FRect rect{ boundary.pos.x, boundary.pos.y, boundary.size.x, boundary.size.y };
            SDL_RenderRect(renderer, &rect);

            std::cout << objects.size() << std::endl;
            for(auto& object: objects) {
                // rect.x = object->pos.x;
                // rect.y = object->pos.y;
                // rect.w = object->size.x;
                // rect.h = object->size.y;
                // SDL_RenderRect(renderer, &rect);
                drawFilledCircle(renderer, object->pos.x, object->pos.y, 5);
            }

            for(auto& child: children) 
                child->render(renderer);
        }
};

Quadtree<phy::Rect2D> qtree;
std::vector<phy::Rect2D> objects;
std::vector<SDL_FColor> colors;


void init()
{
    qtree.resize(phy::Rect2D{ {0, 0}, {W, H} }, 4);
    
    for(int i = 0; i < 4; i++) {
        const float w = randRange(20, 40);
        const float h = randRange(20, 40);
        const float x = randRange(w, W - w);
        const float y = randRange(h, H - h);
        phy::Rect2D rect{ {x, y}, {w, h} };
        objects.push_back(rect);
        qtree.insert(objects[i]);
        colors.push_back({ randRange(0, 255), randRange(0, 255), randRange(0, 255) });
    }

    std::cout << qtree.size() << " -> " << objects.size() << std::endl;

    t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
}

void render(SDL_Renderer* renderer)
{

    for(int i = 0; i < objects.size(); i++) {
        const auto& r = objects[i];
        const auto& color = colors[i];
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
        SDL_FRect rect{ r.pos.x, r.pos.y, r.size.x, r.size.y };
        SDL_RenderFillRect(renderer, &rect);
    }


    // render quadtree
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    qtree.render(renderer);

    // SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    // SDL_RenderRect(renderer, &rect);

    // auto ranged = qtree.findObject(rect);
    // SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    // for(auto& point: ranged) {
    //     drawFilledCircle(renderer, point.x, point.y, 1);
    // } 
}


void update(float dt, SDL_Renderer* renderer)
{
   
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
            // qtree.insert({px, py});
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
