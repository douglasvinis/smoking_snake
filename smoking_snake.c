// -------------------------------------
//  @created: 2022-09-15
//  @author:  Douglas Vinicius
//  @email:   douglvini@gmail.com
// -------------------------------------

#include <SDL2/SDL.h>

// some type redefinition of my preference.
typedef unsigned int b32;
typedef unsigned int u32;
typedef unsigned char u8;
typedef int s32;
#define true 1
#define false !true

// the size of the game window.
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 720

int main()
{
	SDL_Init(SDL_INIT_TIMER| SDL_INIT_VIDEO| SDL_INIT_EVENTS);
	SDL_Window* window = SDL_CreateWindow("Smoking Snake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
			WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	if (window)
	{
		// creating the default sdl window surface.
		SDL_Surface* surface = SDL_GetWindowSurface(window);

		b32 is_running = true;
		while (is_running)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
					case SDL_QUIT:
						is_running = false;
						printf("] Quitting the game...\n");
					break;
				}
			}

			// painting the surface yellow and green
			for (s32 _y=0; _y < surface->h; _y++)
			{
				u32* pixel = (u32*)((u8*)surface->pixels + _y*surface->pitch);
				u32 color = 0xf1f100;
				if (_y > (surface->h/2)) color = 0x00c000;
				for (s32 _x=0; _x < surface->w; _x++)
				{
					*pixel++ = color;
				}
			}
			printf("frame.\n");
			SDL_UpdateWindowSurface(window);
		}
	}
	else printf("] Cant create a SDL_Window.\n");
	return 0;
}
