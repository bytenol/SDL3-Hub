/*
*/
#include <iostream>
#include <filesystem>
#include <memory>
#include <algorithm>
#include <ranges>
#include <random>
#include <chrono>
#include <map>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

using board_t = std::vector<int>;

class Texture;

const int TILESIZE = 128;
board_t board;
std::default_random_engine randEngine;
bool nextIsPlayer = false;


std::map<std::string, std::string> msg{
	{"TIE", "It's a TIE: Press R to Restart"},
	{"RESTART", "Press R to Restart"},
	{"AI_WIN", "You Lose This Round: Press R to Restart"},
	{"PL_WIN", "You Win This Round: Press R to Restart"}
};


bool init();
void clearBoard();
bool isWinning(const board_t& board, int player);
board_t getEmptyIndices(const board_t& board);
int minimax(const board_t& board, int depth, bool isMinimazing);
void AIPlay();
int randRangeInt(int min, int max);
Texture loadTexture(const std::string& path);
void reset();
void update();
void render(SDL_Renderer* renderer);
void pollEvent(SDL_Event& evt);
void animate();

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

Texture sprite;


int main()
{
	if (!init())
		return -1;
	reset();
	nextIsPlayer = randRangeInt(0, 10) > 7;
	animate();
	SDL_DestroyWindow(canvas.window);
	SDL_Quit();
	return 0;
}

void update()
{

}

void render(SDL_Renderer* renderer)
{
	for (size_t i = 0; i < 3; i++) {
		for (size_t j = 0; j < 3; j++) {
			int px = j * TILESIZE;
			int py = i * TILESIZE;
			SDL_FRect srcRect{ 2 * TILESIZE, 0, TILESIZE, sprite.h };
			SDL_FRect dstRect{ px, py, TILESIZE, sprite.h };
			SDL_RenderTexture(renderer, sprite.tex, &srcRect, &dstRect);

			srcRect.x = 3 * TILESIZE;
			SDL_RenderTexture(renderer, sprite.tex, &srcRect, &dstRect);
			 
			int boardId = board[i * 3 + j];
			if (boardId >= 0) {
				srcRect.x = boardId * TILESIZE;
				SDL_RenderTexture(renderer, sprite.tex, &srcRect, &dstRect);
			}
		}
	}
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

		if (evt.type == SDL_EVENT_KEY_UP) {
			if (evt.key.key == SDLK_R) {
				SDL_Log("Clearing board");
				clearBoard();
			}
		}

		if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP && nextIsPlayer) {
			int ex = evt.button.x / TILESIZE;
			int ey = evt.button.y / TILESIZE;
			int pId = ey * 3 + ex;
			auto emptyIndices = getEmptyIndices(board);
			auto isValidPress = std::find(emptyIndices.begin(), emptyIndices.end(), pId);
			if (isValidPress == emptyIndices.end()) {
				SDL_Log("Invalid Chosen Index");
				return;
			}

			board[pId] = 1;
			
			if (isWinning(board, 1)) {
				SDL_Log("Player is Winning");
				return;
			}

			AIPlay();
		}
	}
}

void animate()
{
	while (!canvas.windowShouldClose)
	{
		pollEvent(canvas.evt);
		update();
		SDL_SetRenderDrawColor(canvas.renderer, 255, 255, 255, 255);
		SDL_RenderClear(canvas.renderer);
		render(canvas.renderer);
		SDL_RenderPresent(canvas.renderer);
	}
}


bool init()
{
	canvas.window = SDL_CreateWindow("TicTacToe", TILESIZE * 3, TILESIZE * 3, 0);
	canvas.renderer = SDL_CreateRenderer(canvas.window, nullptr);

	if (!canvas.window || !canvas.renderer)
	{
		SDL_Log("Error creating canvas window or renderer: %s", SDL_GetError());
		return false;
	}

	sprite = loadTexture("/tictac.png");

	randEngine.seed(std::chrono::system_clock::now().time_since_epoch().count());

	return true;
}

void clearBoard()
{
	board = std::vector<int>{ 
		-1, -1, -1, 
		-1, -1, -1, 
		-1, -1, -1
	};
}

bool isWinning(const board_t& board, int player)
{
	std::vector<std::vector<int>> wPos {
		{ 0, 1, 2 },
		{ 3, 4, 5 },
		{ 6, 7, 8 },

		{ 0, 3, 6 },
		{ 1, 4, 7 },
		{ 2, 5, 8 },

		{ 0, 4, 8 },
		{ 2, 4, 6 }
	};

	for (const auto& pos : wPos) {
		auto isWinningLoc = std::all_of(pos.cbegin(), pos.cend(), [&player, &board](int i) {
			return board[i] == player;
		});

		if (isWinningLoc) return true;
	}

	return false;
}


board_t getEmptyIndices(const board_t& board)
{
	board_t res;
	res.clear();

	for (int i = 0; i < 9; i++)
		if (board[i] == -1)
			res.push_back(i);

	return res;
}

int minimax(const board_t& board, int depth, bool isMinimazing)
{
	auto emptyIndices = getEmptyIndices(board);

	if (depth <= 0 || emptyIndices.size() == 0)
		return 0;

	if (isWinning(board, 1)) return 1;
	if (isWinning(board, 0)) return -1;

	if (isMinimazing) {
		int minValue = INFINITY;

		for (const auto& index : emptyIndices) {
			board_t nBoard{ board.begin(),  board.end() };
			nBoard[index] = 0;
			auto g = minimax(nBoard, depth - 1, false);
			minValue = std::min(minValue, g);
		}

		return minValue;
	}
	else {
		int maxValue = -INFINITY;

		for (const auto& index : emptyIndices) {
			board_t nBoard{ board.begin(),  board.end() };
			nBoard[index] = 1;
			auto g = minimax(nBoard, depth - 1, true);
			maxValue = std::max(maxValue, g);
		}

		return maxValue;
	}

}

void AIPlay()
{

	if (isWinning(board, 0) || isWinning(board, 1)) {
		SDL_Log("%s", msg["RESTART"].c_str());
		return;
	}

	auto emptyIndices = getEmptyIndices(board);

	// play at random position 
	if (emptyIndices.size() >= 8) {
		board[emptyIndices[randRangeInt(0, emptyIndices.size() - 1)]] = 0;
		return;
	}

	if (emptyIndices.empty()) {
		SDL_Log("%s", msg["TIE"].c_str());
		return;
	}

	int minValue = INFINITY;
	int chosenIndex = -1;
	for (const auto& index : emptyIndices) {
		board_t nBoard{ board.begin(),  board.end() };
		nBoard[index] = 0;
		auto g = minimax(nBoard, 4, false);
		if (g <= minValue) {
			minValue = g;
			chosenIndex = index;
		}
	}

	if (chosenIndex >= 0) {
		board[chosenIndex] = 0;
		nextIsPlayer = true;
	}
	
	if (isWinning(board, 0)) {
		SDL_Log("%s", msg["AI_WIN"].c_str());
	}
}

int randRangeInt(int min, int max)
{
	std::uniform_int_distribution d{ min, max };
	return d(randEngine);
}

Texture loadTexture(const std::string& path)
{
	Texture texture;

	std::string filePath = __FILE__;
	auto assetRoot = std::filesystem::path(filePath).parent_path().parent_path().string() + "/assets" + path;
	const char* spritePath = assetRoot.c_str();

	texture.tex = IMG_LoadTexture(canvas.renderer, spritePath);
	if (!texture.tex) {
		SDL_Log("Failed to load image: %s", SDL_GetError());
		return texture;
	}

	SDL_GetTextureSize(texture.tex, &(texture.w), &(texture.h));
	return texture;
}

void reset()
{
	clearBoard();

	if (!nextIsPlayer) AIPlay();
}
