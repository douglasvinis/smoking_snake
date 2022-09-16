// -------------------------------------
//  @created: 2022-09-15
//  @author:  Douglas Vinicius
//  @email:   douglvini@gmail.com
// -------------------------------------

#include <stdlib.h>

#include <SDL2/SDL.h>

// some type redefinition of my preference.
typedef unsigned long long u64;
typedef unsigned int b32;
typedef unsigned int u32;
typedef unsigned char u8;
typedef int s32;
#define true 1
#define false !true

// the size of the game window.
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 720

#define BG_COLOR_0 0x5a0059

// debug stuff.
#define DEBUG_MODE 1 // this activate some debug printf's and ASSERT

#ifdef DEBUG_MODE
#define ASSERT(expression) if (!(expression)) *((u8*)0x0) = 0;
#else
#define ASSERT(...)
#endif

// Data layout
typedef struct
{
	float r, g, b, a;
}Color;

// @note Pixmap is here just because i may decide draw a Pixmap into a Pixmap later instead of drawing everything
// into the SDL_Surface always.
typedef struct
{
	u32 width;
	u32 height;
	s32 pitch;
	// SDL_Surface: 0xXX_RR_GG_BB || Pixmap: 0xAA_RR_GG_BB.
	b32 ro_alpha; // read only alpha. This may cause @speed problems.
	u8* pixels;
}Pixmap;

typedef struct
{
	float dir_x;
	float dir_y;
}Input;

typedef struct
{
	b32 initialized;
	u32 points;
	float pos_x;
	float pos_y;
}Game;


// Draw procs
u32 get_u32_color(Color color)
{
	u32 result = ((u32)(color.a*255.0) << 24 |
			(u32)(color.r*255.0) << 16|
			(u32)(color.g*255.0) << 8 |
			(u32)(color.b*255.0));
	return result;
}

void draw_rectangle(Pixmap* pixmap, s32 pos_x, s32 pos_y, s32 width, s32 height, u32 color)
{
	// clipping the rectangle.
	s32 min_x = pos_x;
	s32 min_y = pos_y;
	s32 max_x = pos_x + width;
	s32 max_y = pos_y + height;;
	if (min_x < 0) min_x = 0;
	if (min_x > pixmap->width) min_x = pixmap->width;
	if (min_y < 0) min_y = 0;
	if (min_y > pixmap->height) min_y = pixmap->height;
	if (max_x < 0) max_x = 0;
	if (max_x > pixmap->width) max_x = pixmap->width;
	if (max_y < 0) max_y = 0;
	if (max_y > pixmap->height) max_y = pixmap->height;

	u8* start_row = (u8*)pixmap->pixels + min_y * pixmap->pitch;
	for (s32 _y=0; _y < (max_y-min_y); _y++)
	{
		u32* pixel = (u32*)(start_row + _y*pixmap->pitch) + min_x;
		for (s32 _x=0; _x < (max_x-min_x); _x++)
		{
			*pixel++ = color;
		}
	}
}

// Game procs
void game_tick(Pixmap* backbuffer, Game* game, Input* input, float dt)
{
	if (!game->initialized)
	{
		game->initialized = true;
		game->pos_x = WINDOW_WIDTH/2 - 25;
		game->pos_y = WINDOW_HEIGHT/2 - 25;
	}
	game->pos_x += input->dir_x * 1000.0 * dt;
	game->pos_y += input->dir_y * 1000.0 * dt;
	// drawing.
	draw_rectangle(backbuffer, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, BG_COLOR_0);
	draw_rectangle(backbuffer, (u32)game->pos_x, (u32)game->pos_y, 50, 50, 0x00d0d0);
}

int main()
{
	SDL_Init(SDL_INIT_TIMER| SDL_INIT_VIDEO| SDL_INIT_EVENTS);
	SDL_Window* window = SDL_CreateWindow("Smoking Snake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
			WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	if (window)
	{
		// creating the default sdl window surface.
		SDL_Surface* surface = SDL_GetWindowSurface(window);
		Pixmap sdl_pixmap;
		sdl_pixmap.pixels = surface->pixels;
		sdl_pixmap.pitch = surface->pitch;
		sdl_pixmap.ro_alpha = true;
		sdl_pixmap.width = surface->w;
		sdl_pixmap.height = surface->h;
		Pixmap* backbuffer = &sdl_pixmap;

		// setup time.
		u32 time_last_frame = 0;
		u32 frame_time = 16; // 60fps
		float delta_time = 1.0/60.0; // @todo this will only work in 16ms.

		// get some game memory.
		Game* game = malloc(sizeof(Game));
		game->initialized = false;

		Input input = {};

		b32 is_running = true;
		while (is_running)
		{
			input.dir_x = 0;
			input.dir_y = 0;
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
					case SDL_QUIT:
						is_running = false;
						printf("] Quitting the game...\n");
					break;
					case SDL_KEYDOWN:
					case SDL_KEYUP:
					{
						b32 is_key_down = event.key.type == SDL_KEYDOWN;
						SDL_Keycode keycode = event.key.keysym.sym;
						switch (keycode) 
						{
							case SDLK_w: input.dir_y = -1; break;
							case SDLK_s: input.dir_y = 1; break;
							case SDLK_a: input.dir_x = -1; break;
							case SDLK_d: input.dir_x = 1; break;
						}
					}break;
				}
			}

			// update and draw the game.
			game_tick(backbuffer, game, &input, delta_time);
			
			// sleep some time to maintain 16ms if needed.
			u32 work_time = SDL_GetTicks64() - time_last_frame;
			// if work time is greater than the frame time just dont sleep.
			u32 sleep_time = 0;
			if (work_time < frame_time) 
			{
				sleep_time = frame_time - work_time;
				SDL_Delay(sleep_time);
			}
#if DEBUG_MODE
			printf("] work-frame | %2dms %2dms\n", work_time, work_time + sleep_time);
#endif
			SDL_UpdateWindowSurface(window);
			time_last_frame = SDL_GetTicks64();
		}
	}
	else printf("] Cant create a SDL_Window.\n");
	return 0;
}
