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
// @todo make the screen correct 
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 720

// game stuff
#define CELL_COUNT 25 // horizontal and vertical cell count @note keep this uneven
#define HALF_CELL_COUNT (CELL_COUNT/2)

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
Color make_color(float r, float g, float b, float a)
{
	Color result = {r, g, b, a};
	return result;
}

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
//Vec2 vec2_div(Vec2 a, float b) {return vec2(a.x/b, a.y/b);}
Vec2 vec2_add(Vec2 a, Vec2 b)  {return vec2(a.x+b.x, a.y+b.y);}
Vec2 vec2_sub(Vec2 a, Vec2 b) {return vec2(a.x-b.x, a.y-b.y);}
//float vec2_length(Vec2 a) {return sqrtf(a.x*a.x + a.y*a.y);}
//Vec2 vec2_normalize(Vec2 a) {return vec2_div(a, vec2_length(a));}
Vec2 vec2_lerp(Vec2 from, float t, Vec2 to) {return  vec2_add(vec2_mul((1.0f -t), from), vec2_mul(t, to));}

#define MAX_SNAKE_PARTS 128
typedef struct
{
	s32 from_x, from_y;
	s32 to_x, to_y;
	float pos_t;
	float radius;
}SnakePart;
typedef struct
{
	b32 initialized;
	u32 points;
	Vec2 grid_center;
	float cell_size;
	s32 input_x;
	s32 input_y;

	u32 snake_part_count;
	SnakePart snake[MAX_SNAKE_PARTS];
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
Vec2 get_cell_pos(Game* game, s32 cell_x, s32 cell_y)
{
	Vec2 sub = vec2_mul(game->cell_size, vec2((float)cell_x, (float)cell_y));
	Vec2 result = vec2_add(game->grid_center, sub);
	return result;
}

#if 0
// @todo maybe store the cell index into the snake part instead of this.
void get_cell_from_pos(Game* game, Vec2 pos, s32* cell_x, s32* cell_y)
{
	pos = vec2_sub(pos, game->grid_center);
	Vec2 amount = vec2_div(pos, game->cell_size);
	*cell_x = (s32)roundf(amount.x);
	*cell_y = (s32)roundf(amount.y);
}
#endif

void game_tick(Pixmap* backbuffer, Game* game, Input* input, float dt)
{
	if (!game->initialized)
	{
		game->initialized = true;

		game->grid_center = vec2_mul(0.5f, vec2(WINDOW_WIDTH, WINDOW_HEIGHT));
		game->cell_size = (float)WINDOW_WIDTH/(float)CELL_COUNT;
		// snake head
		game->snake_part_count = 24;

		for (s32 si=0; si < game->snake_part_count; si++)
		{
			SnakePart* part = game->snake + si;
			part->radius = 0.7f * game->cell_size;
			part->from_x = 0;
			part->from_y = 0;
			part->to_x = 0;
			part->to_y = 0;
			part->pos_t = 0.0;
		}
		game->input_x = 1;
		game->input_y = 0;
	}
	if (is_down(input->left) && game->input_x != 1) {game->input_x = -1; game->input_y = 0;}
	if (is_down(input->right) && game->input_x != -1) {game->input_x = 1; game->input_y = 0;}
	if (is_down(input->up) && game->input_y != 1) {game->input_y = -1; game->input_x = 0;}
	if (is_down(input->down) && game->input_y != -1) {game->input_y = 1; game->input_x = 0;}

	//
	// Rendering
	//
	draw_rectangle(backbuffer, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, get_u32_color(make_color(0.4f, 0, 0.35f, 1.0f)));

	// drawing a grid
	for (s32 cell_y=-HALF_CELL_COUNT; cell_y <= HALF_CELL_COUNT; cell_y++)
	{
		for (s32 cell_x=-HALF_CELL_COUNT; cell_x <= HALF_CELL_COUNT; cell_x++)
		{
			Vec2 pos = get_cell_pos(game, cell_x, cell_y);
			Color color = make_color(0.3, 0, 0.25, 1.0);
			float margin = 2.0f;
			Vec2 offset = vec2(-0.5 * game->cell_size + margin, -0.5* game->cell_size + margin);
			pos = vec2_add(pos, offset);
			draw_rectangle(backbuffer, pos.x, pos.y, game->cell_size-2*margin,
					game->cell_size-2*margin, get_u32_color(color));
		}
	}

	// drawing the snake parts.
	for (s32 si=0; si < game->snake_part_count; si++)
	{
		SnakePart* part = game->snake + si;
		if (part->pos_t >= 1.0)
		{
			part->pos_t = 0;
			part->from_x = part->to_x;
			part->from_y = part->to_y;
			if (si == 0)
			{
				s32 cell_x = part->from_x;
				s32 cell_y = part->from_y;
				cell_x += game->input_x;
				cell_y += game->input_y;
				if (cell_x < -HALF_CELL_COUNT) cell_x = HALF_CELL_COUNT;
				if (cell_x > HALF_CELL_COUNT) cell_x = -HALF_CELL_COUNT;
				if (cell_y < -HALF_CELL_COUNT) cell_y = HALF_CELL_COUNT;
				if (cell_y > HALF_CELL_COUNT) cell_y = -HALF_CELL_COUNT;
				part->to_x = cell_x;
				part->to_y = cell_y;
			}
			else 
			{
				SnakePart* last_part = game->snake + si-1;
				part->to_x = last_part->from_x;
				part->to_y = last_part->from_y;
			}
		}

		part->pos_t += 8.2 * dt;
		Color color = make_color(0.25f, 0.5f, 0, 1);
		if (si == 0) color = make_color(0.65, 0.5, 0, 1);

		b32 on_h_mirror = false;
		b32 on_v_mirror = false;
		s32 h_value = part->to_x - part->from_x;
		s32 v_value = part->to_y - part->from_y;
		if (h_value < -1 || h_value > 1) on_h_mirror = true;
		else if (v_value < -1 || v_value > 1) on_v_mirror = true;

		if (!on_v_mirror && !on_h_mirror)
		{
			Vec2 from_pos = get_cell_pos(game, part->from_x, part->from_y);
			Vec2 to_pos = get_cell_pos(game, part->to_x, part->to_y);

			Vec2 pos = vec2_lerp(from_pos, part->pos_t, to_pos);

			draw_rectangle(backbuffer, (u32)(pos.x - 0.5*part->radius),
					(u32)(pos.y - 0.5*part->radius), part->radius, part->radius, get_u32_color(color));
		}
		else // trying to do some smooth animation when on edge mirroring. 
		{
			s32 mirror_to_x = part->to_x;
			s32 mirror_to_y = part->to_y;
			s32 mirror_from_x = part->from_x;
			s32 mirror_from_y = part->from_y;
			if (on_h_mirror)
			{
				if (h_value > 0)
				{
					mirror_to_x = part->from_x -1;
					mirror_from_x = part->to_x +1;
				}
				else
				{
					mirror_to_x = part->from_x +1;
					mirror_from_x = part->to_x -1;
				}
			}
			else
			{
				if (v_value > 0)
				{
					mirror_to_y = part->from_y -1;
					mirror_from_y = part->to_y +1;
				}
				else
				{
					mirror_to_y = part->from_y +1;
					mirror_from_y = part->to_y -1;
				}
			}

			Vec2 from_pos_a = get_cell_pos(game, part->from_x, part->from_y);
			Vec2 to_pos_a = get_cell_pos(game, mirror_to_x, mirror_to_y);

			Vec2 pos = vec2_lerp(from_pos_a, part->pos_t, to_pos_a);
			draw_rectangle(backbuffer, (u32)(pos.x - 0.5*part->radius),
					(u32)(pos.y - 0.5*part->radius), part->radius, part->radius, get_u32_color(color));

			Vec2 from_pos_b = get_cell_pos(game, mirror_from_x, mirror_from_y);
			Vec2 to_pos_b = get_cell_pos(game, part->to_x, part->to_y);

			pos = vec2_lerp(from_pos_b, part->pos_t, to_pos_b);
			draw_rectangle(backbuffer, (u32)(pos.x - 0.5*part->radius),
					(u32)(pos.y - 0.5*part->radius), part->radius, part->radius, get_u32_color(color));
		}
	}
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
