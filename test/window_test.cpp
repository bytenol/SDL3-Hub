#include <gtest/gtest.h>
#include <SDL3/SDL.h>



TEST(SDLWindowSmoke, WindowTest)
{
	ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO) > 0) << "SDL3 Initialization failed";

	SDL_Window* window = SDL_CreateWindow("TestWindow", 640, 480, 0);
	ASSERT_NE(window, nullptr) << "SDL3 Failed to create window";

	SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
	ASSERT_NE(renderer, nullptr) << "SDL3 Failed to create renderer";
}
