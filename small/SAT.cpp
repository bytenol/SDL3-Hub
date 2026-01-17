///**
// * @file SAT_test/main.cpp
// * @brief a simple implementation of the seperating axis theorem
// * @date 24th Sept, 2024
// *
// * This file shows the implementation of the seperating axis theorem, A
// * common collision detection techniques that works for any convex polygons
// * and quadrilaterals.
// */
//#include <iostream>
//#include <chrono>
//#include <SDL3/SDL.h>
//#include <random>
//#include <emscripten/emscripten.h>
//#include <phy/Vector.h>
//
//using namespace phy;
//
//
//struct Canvas
//{
//    SDL_Window* window = nullptr;
//    SDL_Renderer* renderer = nullptr;
//    SDL_Event evt;
//    int w;
//    int h;
//
//} canvas;
//
//
//struct Polygon
//{
//    struct {
//        unsigned int r = 255;
//        unsigned int g = 0;
//        unsigned int b = 255;
//    } color;
//
//    Vector2 pos;
//    Vector2 vel;
//    float rotation = 0.0f;
//    float radius = 0.0f;
//
//    // original vertex data without any transformation
//    std::vector<Vector2> vertices;
//
//    // transformed vertex data: always get updated in every frame
//    std::vector<Vector2> transformed;
//
//    Polygon() = default;
//    Polygon(const std::vector<Vector2>& vertices)
//    {
//        this->vertices = vertices;
//        this->transformed = vertices;
//    }
//};
//
//
//
///// @brief Initialiaze the canvas object
///// @param canvas The canvas object
///// @return true if successflly initialized
//bool initCanvas(Canvas& canvas);
//
//
///// @brief Initialize all variables
///// @param cnv The canvas object
//void init(Canvas& cnv);
//
//void mainLoop();
//
//void render(Canvas& cnv);
//
//void update(float dt, Canvas& cnv);
//
//
///// @brief Carry out seperating axis theorem algorithms on polygons
///// @param polygon The polygon to check 
///// @param polygon2 The potential polygon it will collide with
///// @return true if there is any collision
//bool sat_collision(Polygon& polygon, Polygon& polygon2);
//
//decltype(std::chrono::high_resolution_clock::now()) lastTime;
//
//std::vector<Polygon> polygons;
//int W, H;
//
//
//// The functions inside this block are exported
//extern "C"
//{
//    EMSCRIPTEN_KEEPALIVE int sat_main()
//    {
//        try {
//            initCanvas(canvas);
//        }
//        catch (...) {
//            return -1;
//        }
//        init(canvas);
//        emscripten_set_main_loop(mainLoop, 0, 1);
//        return 0;
//    }
//
//    EMSCRIPTEN_KEEPALIVE void setSize(int w_, int h_)
//    {
//        canvas.w = w_;
//        canvas.h = h_;
//        emscripten_set_canvas_size(w_, h_);
//    }
//}
//
//void makePolygon(int amount)
//{
//    std::random_device rd;
//    std::mt19937 eng(rd());
//
//    auto randRange = [&eng](int min, int max) {
//        std::uniform_int_distribution<> distr(min, max);
//        return distr(eng);
//        };
//
//    for (int i = 0; i < amount; i++)
//    {
//        int sides = randRange(3, 6);
//        int step = 360 / sides;
//
//        float radius = randRange(5, 50);
//
//        // polar coordinates
//        std::vector<Vector2> vertices;
//        for (int j = 0; j < 360; j += step)
//        {
//            float angle = j * 3.14159f / 180;
//            vertices.push_back({ std::cos(angle) * radius, std::sin(angle) * radius });
//        }
//
//        Polygon polygon{ vertices };
//        polygon.radius = radius;
//        polygon.pos.x = randRange(radius * 1.4, canvas.w - radius * 1.4);
//        polygon.pos.y = randRange(radius * 1.4, canvas.h - radius * 1.4);
//
//        float vAng = randRange(0, 360) * 3.14159f / 180;
//        polygon.vel = Vector2(std::cos(vAng) * radius, std::sin(vAng) * 3);
//
//        polygons.push_back(polygon);
//    }
//}
//
//
//
//void init(Canvas& cnv)
//{
//    makePolygon(25);
//    polygons[1].vel = Vector2(0, 0);    // mouse polygon
//    int isFullScreen;
//    emscripten_get_canvas_size(&W, &H, &isFullScreen);
//    lastTime = std::chrono::high_resolution_clock::now();
//}
//
//
//void update(float dt, Canvas& cnv)
//{
//    // transform the whole polygon
//    for (auto& polygon : polygons)
//    {
//        polygon.pos += polygon.vel * dt;
//        for (int i = 0; i < polygon.vertices.size(); i++)
//            polygon.transformed[i] = polygon.pos + polygon.vertices[i].rotate(polygon.rotation);
//    }
//
//    for (auto polygon = polygons.begin(); polygon != polygons.end(); polygon++)
//    {
//        polygon->color.b = 255;
//
//        // wall boundary check
//        if (polygon->pos.x - polygon->radius <= 0) {
//            polygon->pos.x = polygon->radius;
//            polygon->vel.x *= -1;
//        }
//        else if (polygon->pos.x + polygon->radius >= W) {
//            polygon->pos.x = W - polygon->radius;
//            polygon->vel.x *= -1;
//        }
//
//        if (polygon->pos.y - polygon->radius <= 0) {
//            polygon->pos.y = polygon->radius;
//            polygon->vel.y *= -1;
//        }
//        else if (polygon->pos.y + polygon->radius >= H) {
//            polygon->pos.y = H - polygon->radius;
//            polygon->vel.y *= -1;
//        }
//
//        // check for sat collision
//        for (auto polygon2 = polygon + 1; polygon2 != polygons.end(); polygon2++)
//            if (sat_collision(*polygon, *polygon2))
//                polygon->color.b = 0;
//    }
//
//}
//
//
//bool sat_collision(Polygon& polygon, Polygon& polygon2)
//{
//    Polygon* poly1 = &polygon;
//    Polygon* poly2 = &polygon2;
//
//    for (int i = 0; i < 2; i++)
//    {
//        if (i > 0)
//        {
//            poly1 = &polygon2;
//            poly2 = &polygon;
//        }
//
//        for (int i = 0; i < poly1->transformed.size(); i++)
//        {
//            int inext = (i + 1) % poly1->transformed.size();
//            auto p1 = poly1->transformed[i];
//            auto p2 = poly1->transformed[inext];
//            auto vDir = p2 - p1;
//            auto normal = Vector2{ vDir.y, -vDir.x }.normalize();
//
//            float min_1 = INFINITY, max_1 = -INFINITY;
//            for (auto it = poly1->transformed.begin(); it != poly1->transformed.end(); it++)
//            {
//                float dp = it->dotProduct(normal);
//                min_1 = std::min(min_1, dp);
//                max_1 = std::max(max_1, dp);
//            }
//
//            float min_2 = INFINITY, max_2 = -INFINITY;
//            for (auto it = poly2->transformed.begin(); it != poly2->transformed.end(); it++)
//            {
//                float dp = it->dotProduct(normal);
//                min_2 = std::min(min_2, dp);
//                max_2 = std::max(max_2, dp);
//            }
//
//            if (!(min_1 <= max_2 && min_2 <= max_1))
//                return false;
//        }
//    }
//
//    return true;
//}
//
//
//void render(Canvas& canvas)
//{
//    // draw polygons
//    SDL_SetRenderDrawColor(canvas.renderer, 255, 0, 0, 255);
//    for (auto& body : polygons)
//    {
//        SDL_SetRenderDrawColor(canvas.renderer, body.color.r, body.color.g, body.color.b, 255);
//        for (size_t i = 0; i < body.vertices.size(); i++)
//        {
//            auto v1 = body.transformed[i];
//            auto v2 = body.transformed[(i + 1) % body.transformed.size()];
//            SDL_RenderDrawLine(canvas.renderer, v1.x, v1.y, v2.x, v2.y);
//        }
//    }
//}
//
//
//void handleEvent(SDL_Event* evt)
//{
//    if (evt->type == SDL_MOUSEMOTION)
//    {
//        polygons[1].pos.x = evt->motion.x;
//        polygons[1].pos.y = evt->motion.y;
//    }
//
//    if (evt->type == SDL_KEYDOWN) {
//        if (evt->key.keysym.sym == 4) {
//            polygons[1].rotation += 1.f;
//        }
//    }
//}
//
//
//void mainLoop()
//{
//    SDL_SetRenderDrawColor(canvas.renderer, 0x00, 0x00, 0x00, 0xff);
//    SDL_RenderClear(canvas.renderer);
//    render(canvas);
//    SDL_RenderPresent(canvas.renderer);
//
//    auto now = std::chrono::high_resolution_clock::now();
//    auto dt = (now - lastTime).count() * 10e-9;
//    lastTime = now;
//
//    while (SDL_PollEvent(&canvas.evt) != 0)
//        handleEvent(&canvas.evt);
//
//    update(dt, canvas);
//
//}
//
//
//bool initCanvas(Canvas& canvas)
//{
//    if (SDL_Init(SDL_INIT_VIDEO) < 0)
//    {
//        std::cerr << "SDL failed to initialize" << std::endl;
//        return false;
//    }
//
//    canvas.window = SDL_CreateWindow("SAT Collision", 0, 0, canvas.w, canvas.h, SDL_WINDOW_SHOWN);
//    if (!canvas.window)
//    {
//        std::cerr << "SDL failed to create window" << std::endl;
//        return false;
//    }
//
//    canvas.renderer = SDL_CreateRenderer(canvas.window, 0, SDL_RENDERER_ACCELERATED);
//    if (!canvas.renderer)
//    {
//        std::cerr << "SDL failed to create renderer" << std::endl;
//        return false;
//    }
//
//    SDL_SetRelativeMouseMode(SDL_TRUE);
//
//    return true;
//}
