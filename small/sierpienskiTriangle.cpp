/*
* @file sierpienskiTriangle.cpp
* @date 15th dec, 2024
*/
#include <iostream>
#include <vector>
#include <random>
#include <cassert>
#include <SDL3/SDL.h>

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
std::mt19937 gen;


struct Point {
	float x, y;
};


struct Triangle
{
	Point points[3];
};


std::vector<Point> points;
Point selectedPoint;
Triangle currTriangle;

void render();
void init();
int randRange(int min, int max);


int main()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "SDL_INITIALIZATION_FAILED:: " << SDL_GetError() << std::endl;
		return -1;
	}

	window = SDL_CreateWindow("Sierpienski Triangle", 500, 500, 0);
	renderer = SDL_CreateRenderer(window, nullptr);
	if (!window || !renderer) {
		std::cerr << "WINDOW_CREATION_FAILED:: " << SDL_GetError() << std::endl;
		return -1;
	}

	init();

	SDL_Event evt;
	bool windowShouldClose = false;

	while (!windowShouldClose) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);

		while (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_EVENT_QUIT) {
				windowShouldClose = true;
				continue;
			}
		}
		render();
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}


void render()
{
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	for (auto& point : points) {
		SDL_RenderPoint(renderer, point.x, point.y);
	}
}


void init()
{
	std::random_device rd;
	gen = std::mt19937(rd());

	currTriangle.points[0].x = 250;
	currTriangle.points[0].y = 50;
	currTriangle.points[1].x = 50;
	currTriangle.points[1].y = currTriangle.points[2].y = 350;
	currTriangle.points[2].x = 450;

	points.push_back(currTriangle.points[0]);
	points.push_back(currTriangle.points[1]);
	points.push_back(currTriangle.points[2]);

	// The first point to be selected
	selectedPoint.x = 250;
	selectedPoint.y = 150;

	for (int i = 0; i < 50000; i++) {
		int vertexIndex = randRange(0, 2);
		auto& vertex = currTriangle.points[vertexIndex];
		float midX = (selectedPoint.x + vertex.x) / 2.0f;
		float midY = (selectedPoint.y + vertex.y) / 2.0f;
		selectedPoint.x = midX;
		selectedPoint.y = midY;
		points.push_back({ midX, midY });
	}
}


int randRange(int min, int max) {
	std::uniform_int_distribution<int> dis(min, max);
	return dis(gen);
}