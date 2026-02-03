#include <iostream>
#include <vector>
#include <list>
#include <chrono>
#include <random>
#include <memory>
#include <filesystem>
#include <cmath>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "./include/phy/vec2.h"

#define PI 3.14159

using namespace std;

class Texture;

SDL_Renderer* renderer = nullptr;
constexpr int W = 450;
constexpr int H = 225;

std::chrono::high_resolution_clock::duration t0;

struct Texture {
	SDL_Texture* tex = nullptr;
	float w, h;
};

Texture earthSprite;

void update(const float& dt);
float degToRad(const float& rad);
Texture loadTexture(SDL_Renderer* renderer, const std::string& path);
void drawFilledCircle(SDL_Renderer* r, float px, float py, float radius);
void drawImage(SDL_Renderer* renderer, const Texture& texture, const float &x, const float &y, const float &w, const float &h);


struct {

	phy::vec2 pos, dir;
	float rotation = 0.0f;

	float fov = 45;
	float zNear = 10.0f;
	float zFar = 50;

	void update(const float& dt) 
	{
		theta = degToRad(rotation);
		const float angleRad = degToRad(fov * 0.5f);
		auto dRight = phy::vec2::fromAngle(angleRad);
		auto dLeft = phy::vec2::fromAngle(-angleRad);

		const float maxDist = (zFar - zNear);
		a = pos + dRight * zNear;
		b = pos + dRight * maxDist;
		c = pos + dLeft * zNear;
		d = pos + dLeft * maxDist;
	}

	void render(SDL_Renderer* renderer)
	{
		// Direction vector
		float dirX = cos(theta);
		float dirY = sin(theta);

		// Perpendicular vector (camera plane)
		float planeX = -dirY;
		float planeY =  dirX;

		// Lock texture for pixel access
		void* pixels;
		int pitch;
		SDL_LockTexture(earthSprite.tex, nullptr, &pixels, &pitch);
		Uint32* texPixels = (Uint32*)pixels;

		SDL_Texture* screenTex =
			SDL_CreateTexture(renderer,
				SDL_PIXELFORMAT_RGBA8888,
				SDL_TEXTUREACCESS_STREAMING,
				W, H);

		void* screenPixels;
		int screenPitch;
		SDL_LockTexture(screenTex, nullptr, &screenPixels, &screenPitch);
		Uint32* screen = (Uint32*)screenPixels;

		float cameraHeight = 40.0f;
		float horizon = H * 0.5f;


		for (int y = horizon; y < H; y++)
		{
			float rowDist = cameraHeight / (y - horizon);

			// World step per pixel
			float stepX = rowDist * (planeX * tan(degToRad(fov * 0.5f))) * 2.0f / W;
			float stepY = rowDist * (planeY * tan(degToRad(fov * 0.5f))) * 2.0f / W;

			// Leftmost world position
			float worldX =
				pos.x + dirX * rowDist -
				planeX * rowDist * tan(degToRad(fov * 0.5f));

			float worldY =
				pos.y + dirY * rowDist -
				planeY * rowDist * tan(degToRad(fov * 0.5f));

			Uint32* row = (Uint32*)((Uint8*)screen + y * screenPitch);

			for (int x = 0; x < W; x++)
			{
				int texX = ((int)worldX) & ((int)earthSprite.w - 1);
				int texY = ((int)worldY) & ((int)earthSprite.h - 1);
				// texX = (texX % (int)earthSprite.w + earthSprite.w) % (int)earthSprite.w;
				// texY = (texY % (int)earthSprite.h + earthSprite.h) % (int)earthSprite.h;


		// 		Uint32* texel =
		// 			(Uint32*)((Uint8*)texPixels + texY * pitch) + texX;

		// 		row[x] = *texel;

		// 		worldX += stepX;
		// 		worldY += stepY;
			}
		}
	}


	void rtender(SDL_Renderer* renderer)
	{


		phy::vec2 nPos;
		auto offset = phy::vec2(W * 0.5, 0);
		
		nPos = offset + pos;
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderLine(renderer, offset.x + a.x, offset.y + a.y, offset.x + b.x, offset.y + b.y);
		SDL_RenderLine(renderer, offset.x + c.x, offset.y + c.y, offset.x + d.x, offset.y + d.y);
		drawFilledCircle(renderer, nPos.x, nPos.y, 3);

		SDL_SetRenderDrawColor(renderer, 190, 25, 50, 255);
		auto aPos = nPos + phy::vec2::fromAngle(theta) * 20.0f;
		SDL_RenderLine(renderer, nPos.x, nPos.y, aPos.x, aPos.y);
	}

	private:
		phy::vec2 a, b, c, d;
		float theta;

} player;


void init()
{
    earthSprite = loadTexture(renderer, "/earth.jpeg");
	player.pos.x = 100;
	player.pos.y = 100;
    t0 = std::chrono::high_resolution_clock::now().time_since_epoch();
}

void update(const float& dt)
{
	player.update(dt);
}

void render(SDL_Renderer* renderer)
{
    

	SDL_FRect srcRect, dstRect;
    srcRect = { 0, 0, earthSprite.w, earthSprite.h };
    dstRect = { earthSprite.w, 0, earthSprite.w, earthSprite.h };
    SDL_RenderTexture(renderer, earthSprite.tex, &srcRect, &dstRect);
	// const auto& text = *(earthSprite.tex);
	// drawImage(renderer, text, earthSprite.w, 0, earthSprite.w, earthSprite.h);

	player.render(renderer);
}



bool processEvent(SDL_Event& evt) {
	switch(evt.type) {
		case SDL_EVENT_QUIT:
			return true;
		case SDL_EVENT_KEY_DOWN:
            switch(evt.key.key) {
				case SDLK_LEFT:
					player.rotation--;
					break;
				case SDLK_RIGHT:
					player.rotation++;
					break;	
			}
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

	auto window = SDL_CreateWindow("Mode 7", W, H, 0);
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
		update(dt);
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

float degToRad(const float &rad)
{
    return (rad * PI) / 180.0f;
}

Texture loadTexture(SDL_Renderer *renderer, const std::string &path)
{
	Texture texture;

	std::string filePath = __FILE__;
	auto assetRoot = std::filesystem::path(filePath).parent_path().parent_path().string() + "/assets" + path;
	const char* spritePath = assetRoot.c_str();

	texture.tex = IMG_LoadTexture(renderer, spritePath);
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


float randRange(const float& min, const float& max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

