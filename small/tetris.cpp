/// @file main.cpp
/// @brief a simple clone of the popular tetris game
/// @date 26th October, 2024
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <queue>
#include <chrono>
#include <random>
#include <cassert>
// #define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

/**
* @todo draw next tetromino
* @todo draw score
* @todo Cast shadows
* @todo tetromino "I" cannot move left when vertical
* @todo gameOver and other game states
*/

int score;
float F_TILESIZE;
size_t TILE_SIZE;
constexpr int ROW_SIZE = 20;
constexpr int COL_SIZE = 10;
float t0, t1, moveTimeStep, elapsedTime;


struct {

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Event evt;
    int width;
    int height;
    bool isOpen = true;

} canvas;

enum TetrominoAction
{
    M_UP,
    M_LEFT,
    M_RIGHT,
    M_DOWN,
    CW_ROTATE,
    CCW_ROTATE,
};


/// @brief Principal class for the tetromino's block
class Tetromino
{
    using texel_t = std::vector<std::vector<unsigned short>>;

public:
    Tetromino();
    void move(TetrominoAction action);
    void rotate(TetrominoAction action);
    void draw(SDL_Renderer* renderer);
    void save();

private:
    short selectedIndex = 0;
    texel_t matrix;
    SDL_Color color;
    short posX = 0, posY = -10;
    bool isInitialized = false;

    inline const int getWidth() const;
    inline const int getHeight() const;

    static std::vector<texel_t> tet_pixels;
};


struct CollisionInfo
{
    SDL_Color color;
    bool isBlocked = false;
};


std::vector<std::vector<CollisionInfo>> collisionBoard;

Tetromino* pCurrentTetromino = nullptr;
std::queue<Tetromino> nextTetrominos;
std::vector<Tetromino> currentTetromino;

void setCanvasSize(int width, int height);

std::tuple<float, float, float> indexToPos(int j, int i);

int randRange(int min, int max);


void init()
{
    score = 0;
    elapsedTime = 0;
    TILE_SIZE = canvas.width * 0.8 / 20;
    F_TILESIZE = TILE_SIZE * 0.9f;
    setCanvasSize(TILE_SIZE * 20, TILE_SIZE * ROW_SIZE);

    for (short i = 0; i < 5; i++) nextTetrominos.push({});

    currentTetromino.push_back({});
    pCurrentTetromino = &currentTetromino.back();

    // set collision board
    collisionBoard.clear();
    for (size_t i = 0; i < ROW_SIZE; i++) {
        collisionBoard.push_back({});
        for (size_t j = 0; j < COL_SIZE; j++) {
            collisionBoard[i].push_back({});
        }
    }
    collisionBoard.resize(ROW_SIZE);
}


void update(float dt)
{
    elapsedTime += dt;
    pCurrentTetromino = currentTetromino.size() ? &currentTetromino.back() : nullptr;

    if (elapsedTime >= 0.5f) {
        if (pCurrentTetromino)
            pCurrentTetromino->move(TetrominoAction::M_DOWN);
        elapsedTime = 0.0f;
    }

    for (size_t i = 0; i < collisionBoard.size(); i++) {
        bool isFilled = true;
        for (size_t j = 0; j < collisionBoard[i].size(); j++) {
            auto& id = collisionBoard[i][j];
            if (!id.isBlocked) {
                isFilled = false;
                continue;
            };
        }
        if (isFilled) {
            collisionBoard.erase(collisionBoard.begin() + i);
            std::vector<CollisionInfo> t(COL_SIZE);
            collisionBoard.insert(collisionBoard.begin(), t);
            score += 3;
        }
    }
}


void render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(renderer);

    for (size_t i = 0; i < collisionBoard.size(); i++) {
        for (size_t j = 0; j < collisionBoard[i].size(); j++) {
            const auto& id = collisionBoard[i][j];
            auto [px, py, spacing] = indexToPos(j, i);
            SDL_FRect f_rect{ px, py, F_TILESIZE, F_TILESIZE };
            if (!id.isBlocked) {
                SDL_SetRenderDrawColor(renderer, 0xcc, 0xcc, 0xcc, 40);
                SDL_RenderRect(renderer, &f_rect);
            }
            else {
                SDL_SetRenderDrawColor(renderer, id.color.r, id.color.g, id.color.b, 0xff);
                SDL_RenderFillRect(renderer, &f_rect);
            }
        }
    }

    if (pCurrentTetromino) pCurrentTetromino->draw(renderer);
}


void processEvent(SDL_Event& evt)
{
    if (evt.type == SDL_EVENT_QUIT) {
        canvas.isOpen = false;
        return;
    }

    if (evt.type == SDL_EVENT_KEY_UP) {
        if (!pCurrentTetromino) return;

        switch (evt.key.key)
        {
        case SDLK_LEFT:
            pCurrentTetromino->move(TetrominoAction::M_LEFT);
            break;
        case SDLK_RIGHT:
            pCurrentTetromino->move(TetrominoAction::M_RIGHT);
            break;
        case SDLK_DOWN:
            pCurrentTetromino->move(TetrominoAction::M_DOWN);
            break;
        case SDLK_UP:
            pCurrentTetromino->move(TetrominoAction::M_UP);
            break;
        case SDLK_A:
            pCurrentTetromino->rotate(TetrominoAction::CCW_ROTATE);
            break;
        case SDLK_D:
            pCurrentTetromino->rotate(TetrominoAction::CW_ROTATE);
            break;
        case SDLK_SPACE:
            pCurrentTetromino->save();
            break;
        default:
            break;
        }
    }
}


void setCanvasSize(int width, int height)
{
    canvas.width = width;
    canvas.height = height;
    SDL_SetWindowSize(canvas.window, width, height);
}


std::tuple<float, float, float> indexToPos(int j, int i)
{
    const float spacing = (TILE_SIZE - F_TILESIZE) * 0.5f;
    const float px = j * TILE_SIZE + spacing;
    const float py = i * TILE_SIZE + spacing;
    return std::tuple(px, py, spacing);
}



void loop()
{
    t1 = SDL_GetTicks();
    float dt = (t1 - t0) * 0.001f;
    t0 = t1;
    render(canvas.renderer);
    update(dt);
    while (SDL_PollEvent(&canvas.evt))
        processEvent(canvas.evt);
    SDL_RenderPresent(canvas.renderer);
}


void mainLoop()
{
    t0 = SDL_GetTicks();
#ifdef EMSCRIPTEN
    emscripten_set_main_loop(loop, 0, 1);
#else
    while (canvas.isOpen)
        loop();
#endif
}


bool initSDL(const std::string_view title, int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL2 failed to initialise: " << SDL_GetError() << std::endl;
        return false;
    }

    const char* _title = title.data();
    canvas.window = SDL_CreateWindow(_title, width, height, 0);
    if (!canvas.window) {
        std::cerr << "Window Creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    canvas.renderer = SDL_CreateRenderer(canvas.window, nullptr);
    if (!canvas.renderer) {
        std::cerr << "Renderer Initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }

    canvas.width = width;
    canvas.height = height;

    return true;
}


int randRange(int min, int max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}


int main(int argc, char* argv[])
{
    if (!initSDL("", 640, 640)) return -1;
    // std::cout << "Hello world from SDL" << std::endl;
    init();
    mainLoop();
    return 0;
}

Tetromino::Tetromino()
{
    selectedIndex = randRange(0, tet_pixels.size() - 1);
    assert(selectedIndex < tet_pixels.size());

    auto& selected = tet_pixels[selectedIndex];
    for (short i = 1; i < selected.size(); i++)
        matrix.push_back(selected[i]);

    int rotation_amt = randRange(0, 5);
    for (short i = 0; i < rotation_amt; i++)
        rotate(TetrominoAction::CCW_ROTATE);

    color.r = selected[0][0];
    color.g = selected[0][1];
    color.b = selected[0][2];

    posX = randRange(0, COL_SIZE - getWidth());
    posY = -getHeight();
}


void Tetromino::move(TetrominoAction action)
{
    char vx = action == TetrominoAction::M_LEFT ? -1 : action == TetrominoAction::M_RIGHT ? 1 : 0;
    char vy = action == TetrominoAction::M_UP ? -1 : action == TetrominoAction::M_DOWN ? 1 : 0;

    bool isColliding = false;

    for (size_t i = 0; i < getHeight(); i++) {
        for (size_t j = 0; j < getWidth(); j++) {
            auto& id = matrix[i][j];
            if (id != 0) {
                int new_posX = posX + vx + j;
                int new_posY = posY + vy + i;
                if (new_posY < 0) continue;
                if (new_posX < 0 || new_posX >= COL_SIZE || new_posY >= ROW_SIZE) {
                    isColliding = true;
                    break;
                }
                if ((collisionBoard[new_posY][new_posX]).isBlocked) {
                    isColliding = true;
                    break;
                }
            }
        }
    }

    if (isColliding && action == TetrominoAction::M_DOWN) {
        save();
    }

    if (!isColliding) {
        posX += vx;
        posX = std::max(posX, (short)0);
        posY += vy;
    }
}


void Tetromino::rotate(TetrominoAction action)
{
    std::vector<std::vector<unsigned short>> r_matrix;

    for (size_t i = 0; i < matrix[0].size(); i++) {
        r_matrix.push_back({});
        for (size_t j = 0; j < matrix.size(); j++) {
            if (action == TetrominoAction::CCW_ROTATE)
                r_matrix[i].push_back(matrix[j][matrix[0].size() - 1 - i]);
            else
                r_matrix[i].push_back(matrix[matrix.size() - j - 1][i]);
        }
    }

    bool isColliding = false;
    for (size_t i = 0; i < r_matrix.size(); i++) {
        for (size_t j = 0; j < r_matrix[0].size(); j++) {
            auto& id = r_matrix[i][j];
            if (id != 0) {
                const int px = posX + j;
                const int py = posY + i;
                if (py < 0) continue;
                if (px < 0 || px >= COL_SIZE || py >= ROW_SIZE) {
                    isColliding = true;
                    break;
                }
                if ((collisionBoard[py][px]).isBlocked) {
                    isColliding = true;
                    break;
                }
            }
        }
    }

    if (!isColliding) matrix = r_matrix;

}


void Tetromino::draw(SDL_Renderer* renderer)
{
    // short i, j;
    // short matrixMaxY = 0;   // support point for the matrix;
    // int maxDist = ROW_SIZE * 2;

    // for(j = 0; j < getWidth(); j++) {
    //     for(i = 0; i < getHeight(); i++) {
    //         auto& id = matrix[j][i];
    //         if(id != 0) {
    //             matrixMaxY = std::max(matrixMaxY, i);
    //             short oy = posY + i;
    //             bool isHit  = false;
    //             while (!isHit) {
    //                 oy++;
    //                 if(oy >= ROW_SIZE) {
    //                     oy = ROW_SIZE;
    //                     break;
    //                 }
    //                 isHit = (collisionBoard[oy][j]).isBlocked;
    //             }
    //             maxDist = std::min(maxDist, (int)oy);
    //         }
    //     }
    // }

    // draw shadow
    // for(size_t i = 0; i < getHeight(); i++) {
    //     for(size_t j = 0; j < getWidth(); j++) {
    //         auto& id = matrix[i][j];
    //         auto [px, py, spacing] = indexToPos(posX + j, maxDist - getHeight() + i);
    //         if(id != 0) {
    //             SDL_FRect f_rect{ px, py, F_TILESIZE, F_TILESIZE  };
    //             SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 0xff);
    //             SDL_RenderDrawRectF(renderer, &f_rect);
    //         }
    //     }
    // }

    // draw tetromino
    for (size_t i = 0; i < matrix.size(); i++) {
        for (size_t j = 0; j < matrix[0].size(); j++) {
            unsigned short id = matrix[i][j];
            if (id != 0) {
                auto [px, py, spacing] = indexToPos(posX + j, posY + i);
                SDL_FRect f_rect{ px, py, F_TILESIZE, F_TILESIZE };
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 0xff);
                SDL_RenderFillRect(renderer, &f_rect);
            }
        }
    }

}


void Tetromino::save()
{
    for (size_t i = 0; i < matrix.size(); i++) {
        for (size_t j = 0; j < matrix[i].size(); j++) {
            auto& id = matrix[i][j];
            if (id != 0) {
                auto& info = collisionBoard[posY + i][posX + j];
                info.isBlocked = true;
                info.color.r = color.r;
                info.color.g = color.g;
                info.color.b = color.b;
            }
        }
    }

    currentTetromino.pop_back();
    assert(currentTetromino.size() == 0);
    pCurrentTetromino = nullptr;
    currentTetromino.push_back(nextTetrominos.front());
    nextTetrominos.pop();
    nextTetrominos.push({});
}

inline const int Tetromino::getWidth() const
{
    return matrix[0].size();
}


inline const int Tetromino::getHeight() const
{
    return matrix.size();
}


// The first vector in the block matrix is the color (rgb) of the matrix
std::vector<Tetromino::texel_t> Tetromino::tet_pixels = {
    {   //Z
        {255, 0, 0},
        {0,1,1},
        {1,1,0},
    },
    {   // Z_inv
        {55, 70, 255},
        {1,1,0},
        {0,1,1},
    },
    {   // T
        {255, 120, 0},
        {0,1,0},
        {1,1,1},
    },
    {   // I
        {0, 255, 80},
        {0,1,0},
        {0,1,0},
        {0,1,0},
        {0,1,0},
    },
    {   // L
        {45, 86, 93},
        {1,1,1},
        {1,0,0},
    },
    {
        {97, 107, 200},
        {1,0,0},
        {1,1,1},
    },
    {
        {87, 200, 43},
        {1,1},
        {1,1}
    }
};