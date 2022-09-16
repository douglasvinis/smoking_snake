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
typedef short s16;
typedef unsigned short u16;
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

//
// Data layout
//
typedef struct
{
	float r, g, b, a;
}Color;

//renderer
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

// input
typedef struct
{
	s16 changed;
	u16 up_down_diff;
}Key;
b32 is_just_down(Key key)
{
	b32 result = key.changed && key.up_down_diff == 0;
	return result;
}
b32 is_down(Key key)
{
	b32 result = key.up_down_diff == 1;
	return result;
}

// game
typedef struct
{
	Key up;
	Key left;
	Key right;
	Key down;
}Input;

typedef struct
{
	float x, y;
}Vec2;
// small util math functions
// @cleanup check if all these are used.
Vec2 vec2(float x, float y)
{
	Vec2 result = {x, y};
	return result;
}
Vec2 vec2_mul(float a, Vec2 b) {return vec2(a*b.x, a*b.y);}
Vec2 vec2_div(Vec2 a, float b) {return vec2(a.x/b, a.y/b);}
Vec2 vec2_add(Vec2 a, Vec2 b)  {return vec2(a.x+b.x, a.y+b.y);}
Vec2 vec2_sub(Vec2 a, Vec2 b) {return vec2(a.x-b.x, a.y-b.y);}
float vec2_length(Vec2 a) {return sqrtf(a.x*a.x + a.y*a.y);}
Vec2 vec2_normalize(Vec2 a) {return vec2_div(a, vec2_length(a));}

typedef struct
{
	Vec2 pos;
	Vec2 next_pos;
	Vec2 input_dir;
}SnakeCell;
typedef struct
{
	b32 initialized;
	u32 points;
	SnakeCell snake;
}Game;

//
// Renderer procs
//
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

void change_key(Key* key, s32 diff_add)
{
	key->changed = true;
	key->up_down_diff += diff_add;
}

//
// Game procs
//
#define CELL_SIZE 40
void game_tick(Pixmap* backbuffer, Game* game, Input* input, float dt)
{
	SnakeCell* snake = &game->snake;
	if (!game->initialized)
	{
		game->initialized = true;
		snake->pos = vec2_add(vec2_mul(0.5f, vec2(WINDOW_WIDTH, WINDOW_HEIGHT)), vec2(-25.0f, -25.0f));
		snake->input_dir = vec2(0, 1);
		snake->next_pos = snake->pos;
	}
	if (is_down(input->left)) {snake->input_dir.x = -1; snake->input_dir.y = 0;}
	if (is_down(input->right)) {snake->input_dir.x = 1; snake->input_dir.y = 0;}
	if (is_down(input->up)) {snake->input_dir.y = -1; snake->input_dir.x = 0;}
	if (is_down(input->down)) {snake->input_dir.y = 1; snake->input_dir.x = 0;}

	Vec2 pos_dist = vec2_sub(snake->next_pos, snake->pos);
	float n_pos_dist = vec2_length(pos_dist);
	Vec2 n_pos_dir = vec2_div(pos_dist, n_pos_dist);
	snake->pos = vec2_add(vec2_mul(200.0 * dt, n_pos_dir), snake->pos);
	if (n_pos_dist < 2.0f)
	{
		snake->pos = snake->next_pos;
		snake->next_pos = vec2_add(snake->pos, vec2_mul(CELL_SIZE, snake->input_dir));
	}
	//
	// Rendering
	//
	draw_rectangle(backbuffer, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, BG_COLOR_0);
	draw_rectangle(backbuffer, 360 - 25, 0, 40, WINDOW_HEIGHT, 0xc0c0c0);
	draw_rectangle(backbuffer, (u32)snake->pos.x, (u32)snake->pos.y, 40, 40, 0x00d0d0);
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
			//
			// Input
			//
			SDL_Event event;
			// cleanup input keys
			input.left.changed = false;
			input.right.changed = false;
			input.up.changed = false;
			input.down.changed = false;

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
						if (!event.key.repeat)
						{
							if (is_key_down)
							{
								switch (keycode) 
								{
									case SDLK_w: change_key(&input.up, 1); break;
									case SDLK_s: change_key(&input.down, 1); break;
									case SDLK_a: change_key(&input.left, 1); break;
									case SDLK_d: change_key(&input.right, 1); break;
								}
							}
							else
							{
								switch (keycode) 
								{
									case SDLK_w: change_key(&input.up, -1); break;
									case SDLK_s: change_key(&input.down, -1); break;
									case SDLK_a: change_key(&input.left, -1); break;
									case SDLK_d: change_key(&input.right, -1); break;
								}
							}
						}
					}break;
				}
			}

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
