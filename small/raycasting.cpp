/**
 * @file raycasting2d.cpp
 * @date 6th oct, 2024
 *
 * This is an implementation a raycasting in a 2d tilemap world
 */
#include <iostream>
#include <vector>
#include <cmath>
#include <SDL3/SDL.h>
#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif


using Map_t = std::vector<short>;

template<typename T>
T getMapId(const Map_t& map, const T& y, const T& x);

float degToRad(float f);

void drawFilledCircle(SDL_Renderer* renderer, const float& x, const float& y, const float& radius);

float maxDist = -INFINITY;
const short TILESIZE = 64;
const short TILE_COL = 8;
const short TILE_ROW = 8;
bool shouldQuit = false;
SDL_Event evt;


struct
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    unsigned int w = 0;
    unsigned int h = 0;
} canvas;


struct Vec2
{
    float x;
    float y;
};


struct Ray
{
    float angle;
    Vec2 start;
    Vec2 end;
    float dist;
    bool isLeft;
};


struct Castable
{
    Vec2 pos;
    Vec2 vel;
    float rotation;     // in degrees
    float fov;
    std::vector<Ray> rays;
} player;


std::vector<Castable> characters;


Map_t levelMap{
    1,1,1,1,1,1,1,1,
    1,0,0,0,0,1,0,1,
    1,0,1,0,0,0,0,1,
    1,0,1,1,0,1,0,1,
    1,0,0,1,0,1,0,1,
    1,0,0,1,1,1,0,1,
    1,0,0,1,0,0,0,1,
    1,1,1,1,1,1,1,1,
};


void init()
{
    player.pos.x = 80;
    player.pos.y = 80;
    player.fov = 60;
    player.rotation = 0.0f;

    float rayInc = player.fov / (canvas.w / 2);
    float fovHalf = player.fov / 2;

    for (float angle = -fovHalf; angle < fovHalf; angle += rayInc)
        player.rays.push_back(Ray{ angle });

}



void update(float dt)
{
    for (auto it = player.rays.begin(); it != player.rays.end(); it++)
    {
        float angleInRadians = degToRad(player.rotation + it->angle);
        Vec2 dir{ std::cos(angleInRadians), std::sin(angleInRadians) };

        // simplified dot product
        bool isLeft = (-1 * dir.x) >= 0.0f;
        bool isUp = (-1 * dir.y) >= 0.0f;

        short leftCoeff = isLeft ? -1 : 1;
        short upCoeff = isUp ? -1 : 1;

        // check horizontal collision
        float yOffset = player.pos.y - std::floor(player.pos.y / TILESIZE) * TILESIZE;
        float yA = (isUp ? yOffset : TILESIZE - yOffset);
        float xA = std::abs(yA / std::tan(angleInRadians));

        Vec2 ray1;
        ray1.x = player.pos.x + xA * leftCoeff;
        ray1.y = player.pos.y + yA * upCoeff;

        int tx = std::floor(ray1.x / TILESIZE);
        int ty = isUp ? (std::floor(ray1.y / TILESIZE) - 1) : std::ceil(ray1.y / TILESIZE);
        int id = getMapId(levelMap, ty, tx);

        yA = TILESIZE;
        xA = std::abs(yA / std::tan(angleInRadians));

        while (id == 0)
        {
            ray1.x += xA * leftCoeff;
            ray1.y += yA * upCoeff;
            tx = std::floor(ray1.x / TILESIZE);
            ty = isUp ? (std::floor(ray1.y / TILESIZE) - 1) : std::ceil(ray1.y / TILESIZE);
            id = getMapId(levelMap, ty, tx);
        }

        // check vertical collision
        Vec2 ray2;
        float xOffset = player.pos.x - (std::floor(player.pos.x / TILESIZE) * TILESIZE);
        xA = isLeft ? xOffset : TILESIZE - xOffset;
        yA = std::abs(xA * std::tan(angleInRadians));

        ray2.x = player.pos.x + xA * leftCoeff;
        ray2.y = player.pos.y + yA * upCoeff;
        tx = isLeft ? std::floor(ray2.x / TILESIZE) + (isLeft ? -1 : 0) : std::ceil(ray2.x / TILESIZE);
        ty = std::floor(ray2.y / TILESIZE);
        id = getMapId(levelMap, ty, tx);

        xA = TILESIZE;
        yA = std::abs(xA * std::tan(angleInRadians));

        while (id == 0)
        {
            ray2.x += xA * leftCoeff;
            ray2.y += yA * upCoeff;
            tx = isLeft ? std::floor(ray2.x / TILESIZE) + (isLeft ? -1 : 0) : std::ceil(ray2.x / TILESIZE);
            ty = std::floor(ray2.y / TILESIZE);
            id = getMapId(levelMap, ty, tx);
        }

        Vec2 d1, d2;
        d1.x = player.pos.x - ray1.x;
        d1.y = player.pos.y - ray1.y;
        float h1 = std::hypotf(d1.x, d1.y);

        d2.x = player.pos.x - ray2.x;
        d2.y = player.pos.y - ray2.y;
        float h2 = std::hypotf(d2.x, d2.y);

        auto& min = (h1 < h2) ? ray1 : ray2;
        it->start = player.pos;
        it->end = min;
        it->dist = ((h1 < h2) ? h1 : h2) * std::cos(angleInRadians);
        it->isLeft = (&min == &ray2);

        maxDist = std::max(maxDist, it->dist);
    }
}


void render(SDL_Renderer* renderer)
{

    // render tile
    for (short i = 0; i < TILE_ROW; i++) {
        for (short j = 0; j < TILE_COL; j++) {
            short id = getMapId<short>(levelMap, i, j);
            float px = j * TILESIZE;
            float py = i * TILESIZE;
            SDL_FRect rect{ px, py, (float)TILESIZE, (float)TILESIZE };

            if (id == 0) {
                SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
                SDL_RenderFillRect(renderer, &rect);
                SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
                SDL_RenderRect(renderer, &rect);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0x00, 0xff);
                SDL_RenderFillRect(renderer, &rect);
                SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
                SDL_RenderRect(renderer, &rect);
            }
        }
    }

    auto pOffset = 512;

    int x = 0;
    for (const auto& r : player.rays) {
        float px = pOffset + x;
        float h = std::min((r.dist - 277.0f) / 277.0f * 64, canvas.h * 0.5f);
        float py = canvas.h * 0.5 * 0.5 - h * 0.5;

        if (r.isLeft) SDL_SetRenderDrawColor(renderer, 0x00, 0xff, 0x00, 0xff);
        else SDL_SetRenderDrawColor(renderer, 0x00, 0x33, 0x00, 0xff);
        SDL_RenderLine(renderer, px, py, px, py + h);
        x++;

        SDL_SetRenderDrawColor(renderer, 0x68, 0xf2, 0x52, 0xff);
        SDL_RenderLine(renderer, r.start.x, r.start.y, r.end.x, r.end.y);
    }

    SDL_SetRenderDrawColor(renderer, 0x32, 0x54, 0xa4, 0xff);
    SDL_RenderLine(renderer, pOffset, canvas.h / 2, canvas.w, canvas.h / 2);

    SDL_SetRenderDrawColor(renderer, 0x32, 0x54, 0xa4, 0xff);
    drawFilledCircle(renderer, player.pos.x, player.pos.y, 4);

}


void processEvent(SDL_Event& evt, bool& shouldQuit)
{
    if (evt.type == SDL_EVENT_QUIT) {
        shouldQuit = true;
        return;
    }

    if (evt.type == SDL_EVENT_KEY_DOWN) {
        const float angleInRadians = degToRad(player.rotation);
        player.vel.x = std::cos(angleInRadians);
        player.vel.y = std::sin(angleInRadians);

        switch (evt.key.which)
        {
        case SDLK_UP:
            player.pos.x += player.vel.x;
            player.pos.y += player.vel.y;
            break;
        case SDLK_DOWN:
            player.pos.x -= player.vel.x;
            player.pos.y -= player.vel.y;
            break;
        case SDLK_LEFT:
            player.rotation--;
            if (player.rotation < -360) player.rotation = 0;
            break;
        case SDLK_RIGHT:
            player.rotation++;
            if (player.rotation > 360) player.rotation = 0;
            break;
        default:
            break;
        }
    }
}

void gameLoop()
{
    while (SDL_PollEvent(&evt))
        processEvent(evt, shouldQuit);

    SDL_SetRenderDrawColor(canvas.renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(canvas.renderer);
    render(canvas.renderer);
    update(1 / 60);
    SDL_RenderPresent(canvas.renderer);
}


void mainLoop()
{
#ifdef EMSCRIPTEN
    emscripten_set_main_loop(gameLoop, 0, 1);
    // emscripten_set_main_loop_timing(EMSCRIPTEN_SET_MAIN_LOOP_TIMING_RAF, 0, nullptr);
#else
    while (!shouldQuit)
        gameLoop();
#endif
}


int main(int argc, char const* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "INITIALIZATION failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    canvas.w = 1024;
    canvas.h = 512;
    canvas.window = SDL_CreateWindow("Raycasting3d", canvas.w, canvas.h, 0);
    if (!canvas.window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    canvas.renderer = SDL_CreateRenderer(canvas.window, nullptr);
    if (!canvas.renderer) {
        SDL_DestroyWindow(canvas.window);
        SDL_Quit();
        std::cerr << "Unable to create renderer: " << SDL_GetError() << std::endl;
        return -1;
    }

    std::cout << "Initializing common" << std::endl;

    init();
    mainLoop();

    SDL_DestroyWindow(canvas.window);
    SDL_Quit();

    return 0;
}


template<typename T>
T getMapId(const Map_t& map, const T& y, const T& x)
{
    T res;
    T index = y * TILE_COL + x;
    if (index >= map.size()) return -1;
    try {
        res = map.at(index);
    }
    catch (...) {
        res = -1;
    }
    return res;
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


float degToRad(float f)
{
    return f * 3.14159f / 180;
}
