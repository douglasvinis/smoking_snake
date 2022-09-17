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
// @todo make the screen resizeable and aspect ratio correct.
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

// Math.
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

// Game
typedef struct
{
	Key up;
	Key left;
	Key right;
	Key down;
}Input;
typedef struct
{
	s32 x;
	s32 y;
}GridPos;
GridPos grid_pos(s32 x, s32 y)
{
	GridPos result = {x, y};
	return result;
}
b32 is_grid_pos_equal(GridPos a, GridPos b)
{
	b32 result = (a.x == b.x) && (a.y == b.y);
	return result;
}

typedef struct
{
	GridPos pos;
	float timer;
	b32 eaten;
	b32 active;
}Food;
typedef struct
{
	GridPos from_pos;
	GridPos to_pos;
	float pos_t;
	float radius;
}SnakePart;
#define MAX_SNAKE_PARTS 128
#define MAX_FOOD_COUNT 16
#define MAX_INPUT_QUEUE 3
#define RESTART_TIME 3.0f
enum
{
	Input_None,
	Input_Left,
	Input_Right,
	Input_Up,
	Input_Down,
};
typedef struct
{
	b32 initialized;

	float restart_timer;
	b32 game_over;
	u32 points;
	Vec2 grid_center;
	float cell_size;

	u32 input_queue_count;
	u32 input_queue[MAX_INPUT_QUEUE];

	GridPos snake_dir;

	Food food_list[MAX_FOOD_COUNT];

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
void spawn_food(Game* game)
{
	Food* chosen = 0;
	for (s32 fi=0; fi < MAX_FOOD_COUNT; fi++)
	{
		Food* food = game->food_list + fi;
		if (!food->active)
		{
			chosen = food;
			break;
		}
	}
	chosen->active = true;
	chosen->eaten = false;
	chosen->timer = 30;
	chosen->pos.x = (rand() % 25) - HALF_CELL_COUNT;
	chosen->pos.y = (rand() % 25) - HALF_CELL_COUNT;
}

SnakePart* grow_snake(Game* game, GridPos pos)
{
	SnakePart* part = game->snake + game->snake_part_count++;
	part->radius = 0.7f * game->cell_size;
	part->from_pos = pos;
	part->to_pos = pos;
	part->pos_t = 0.0;
	return part;
}

Vec2 get_cell_pos(Game* game, GridPos pos)
{
	Vec2 sub = vec2_mul(game->cell_size, vec2((float)pos.x, (float)pos.y));
	Vec2 result = vec2_add(game->grid_center, sub);
	return result;
}

void game_tick(Pixmap* backbuffer, Game* game, Input* input, float dt)
{
	if (!game->initialized)
	{
		game->initialized = true;

		game->grid_center = vec2_mul(0.5f, vec2(WINDOW_WIDTH, WINDOW_HEIGHT));
		game->cell_size = (float)WINDOW_WIDTH/(float)CELL_COUNT;

		game->game_over = false;
		game->restart_timer = 0;
		game->points = 0;
		game->input_queue_count = 0;

		// initializing the food list
		for (s32 fi=0; fi < MAX_FOOD_COUNT; fi++)
		{
			Food* food = game->food_list + fi;
			food->pos = grid_pos(0, 0);
			food->timer = 0;
			food->eaten = false;
			food->active = false;
		}
		// snake head
		game->snake_part_count = 0;
		SnakePart* head = grow_snake(game, grid_pos(0, 0));
		head->to_pos = game->snake_dir = grid_pos(1, 0);

		// make the snake big at the start.
#if 1
		for (s32 si=1; si < 2; si++)
		{
			SnakePart* last_part = game->snake + (si-1);
			GridPos pos =last_part->from_pos;
			pos.x -= 1;
			SnakePart* part = grow_snake(game, pos);
			part->to_pos = last_part->from_pos;
		}
#endif
		spawn_food(game);
	}

	u32 input_dir = Input_None;
	if (is_down(input->left)) input_dir = Input_Left;
	else if (is_down(input->right)) input_dir = Input_Right;
	else if (is_down(input->up)) input_dir = Input_Up;
	else if (is_down(input->down)) input_dir = Input_Down;

	b32 equal_to_the_last = false;
	if (game->input_queue_count > 0)
	{
		equal_to_the_last = game->input_queue[game->input_queue_count-1] == input_dir;
	}
	if (game->input_queue_count < MAX_INPUT_QUEUE)
	{
		if (input_dir != Input_None && !equal_to_the_last)
		{
			game->input_queue[game->input_queue_count++] = input_dir;
		}
	}

	// clearing the screen
	draw_rectangle(backbuffer, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, get_u32_color(make_color(0.4f, 0, 0.35f, 1.0f)));

#if 1
	// drawing a grid
	for (s32 cell_y=-HALF_CELL_COUNT; cell_y <= HALF_CELL_COUNT; cell_y++)
	{
		for (s32 cell_x=-HALF_CELL_COUNT; cell_x <= HALF_CELL_COUNT; cell_x++)
		{
			Vec2 pos = get_cell_pos(game, grid_pos(cell_x, cell_y));
			Color color = make_color(0.3, 0, 0.25, 1.0);
			float margin = 2.0f;
			Vec2 offset = vec2(-0.5 * game->cell_size + margin, -0.5* game->cell_size + margin);
			pos = vec2_add(pos, offset);
			draw_rectangle(backbuffer, pos.x, pos.y, game->cell_size-2*margin,
					game->cell_size-2*margin, get_u32_color(color));
		}
	}
#endif
	if (!game->game_over)
	{
		// detecting collision.
		SnakePart* head = game->snake;
		for (s32 fi=0; fi < MAX_FOOD_COUNT; fi++)
		{
			// eating food
			Food* food = game->food_list + fi;
			if (food->active && !food->eaten && is_grid_pos_equal(head->to_pos, food->pos))
			{
				food->eaten = true;
			}
		}

		for (s32 si=1; si < game->snake_part_count; si++)
		{
			SnakePart* part = game->snake + si;
			if (is_grid_pos_equal(head->to_pos, part->from_pos))
			{
				game->game_over = true;
				game->restart_timer = RESTART_TIME;
				break;
			}
		}
	}

	// drawing all the food
	for (s32 fi=0; fi < MAX_FOOD_COUNT; fi++)
	{
		Food* food = game->food_list + fi;
		if (food->active && !food->eaten)
		{
			Vec2 food_pos = get_cell_pos(game, food->pos);
			float size = 0.4 * game->cell_size;
			draw_rectangle(backbuffer, (u32)food_pos.x-0.5*size, (u32)food_pos.y-0.5*size,
					size, size, get_u32_color(make_color(0.8f, 0, 0, 1.0f)));
			food->timer -= dt;
			if (food->timer < 0) food->active = false;
		}
	}

	// drawing the snake parts.
	for (s32 si=0; si < game->snake_part_count; si++)
	{
		SnakePart* part = game->snake + si;
		if (part->pos_t >= 1.0)
		{
			part->pos_t = 0;
			part->from_pos = part->to_pos;
			if (si == 0)
			{
				GridPos input;
				input = game->snake_dir;
				if (game->input_queue_count > 0)
				{
					u32 input_dir = game->input_queue[0];
					if (game->input_queue_count > 1)
					{
						for (s32 ii=1; ii < MAX_INPUT_QUEUE; ii++)
						{
							game->input_queue[ii-1] = game->input_queue[ii];
						}
					}
					game->input_queue_count--;
					if (input_dir == Input_Left) input = grid_pos(-1, 0);
					if (input_dir == Input_Right) input = grid_pos(1, 0);
					if (input_dir == Input_Up) input = grid_pos(0, -1);
					if (input_dir == Input_Down) input = grid_pos(0, 1);
				}
				if ((input.x + game->snake_dir.x) == 0 || (input.y + game->snake_dir.y) == 0)
				{
					input = game->snake_dir;
				}
				s32 cell_x = part->from_pos.x + input.x;
				s32 cell_y = part->from_pos.y + input.y;
				game->snake_dir = input;

				// mirroring the edges
				if (cell_x < -HALF_CELL_COUNT) cell_x = HALF_CELL_COUNT;
				if (cell_x > HALF_CELL_COUNT) cell_x = -HALF_CELL_COUNT;
				if (cell_y < -HALF_CELL_COUNT) cell_y = HALF_CELL_COUNT;
				if (cell_y > HALF_CELL_COUNT) cell_y = -HALF_CELL_COUNT;
				part->to_pos = grid_pos(cell_x, cell_y);
			}
			else 
			{
				SnakePart* last_part = game->snake + si-1;
				part->to_pos = last_part->from_pos;
			}
		}

		// checking for a full belly
		float belly_full = 0;
		for (s32 fi=0; fi < MAX_FOOD_COUNT; fi++)
		{
			Food* food = game->food_list + fi;
			if (food->active && food->eaten)
			{
				if (is_grid_pos_equal(food->pos, part->from_pos))
				{
					belly_full = 1.0 - part->pos_t;
				}
				else if (is_grid_pos_equal(food->pos, part->to_pos))
				{
					belly_full = part->pos_t;
				}
				if ((si == game->snake_part_count -1) && belly_full == 1.0f)
				{
					grow_snake(game, part->from_pos);
					food->eaten = false;
				}
			}
		}
		float part_size = part->radius + belly_full * 10;

		if (game->game_over)
		{
			game->restart_timer -= dt;
			if (game->restart_timer <= 0) game->initialized = false;
		}
		else part->pos_t += 5.2 * dt;
		
		Color color = make_color(0.25f, 0.5f, 0, 1);
		if (si == 0) color = make_color(0.65, 0.5, 0, 1);

		// outside the screen check.
		b32 on_h_mirror = false;
		b32 on_v_mirror = false;
		s32 h_value = part->to_pos.x - part->from_pos.x;
		s32 v_value = part->to_pos.y - part->from_pos.y;
		if (h_value < -1 || h_value > 1) on_h_mirror = true;
		else if (v_value < -1 || v_value > 1) on_v_mirror = true;

		if (!on_v_mirror && !on_h_mirror)
		{
			Vec2 from_pos = get_cell_pos(game, part->from_pos);
			Vec2 to_pos = get_cell_pos(game, part->to_pos);

			Vec2 pos = vec2_lerp(from_pos, part->pos_t, to_pos);

			draw_rectangle(backbuffer, (u32)(pos.x - 0.5*part_size),
					(u32)(pos.y - 0.5*part_size), part_size, part_size, get_u32_color(color));
		}
		else // drawing two times to make a nice smooth animation when mirroring.
		{
			GridPos mirror_to = part->to_pos;
			GridPos mirror_from = part->from_pos;
			if (on_h_mirror)
			{
				if (h_value > 0)
				{
					mirror_to.x = part->from_pos.x -1;
					mirror_from.x = part->to_pos.x +1;
				}
				else
				{
					mirror_to.x = part->from_pos.x +1;
					mirror_from.x = part->to_pos.x -1;
				}
			}
			else
			{
				if (v_value > 0)
				{
					mirror_to.y = part->from_pos.y -1;
					mirror_from.y = part->to_pos.y +1;
				}
				else
				{
					mirror_to.y = part->from_pos.y +1;
					mirror_from.y = part->to_pos.y -1;
				}
			}

			Vec2 from_pos_a = get_cell_pos(game, part->from_pos);
			Vec2 to_pos_a = get_cell_pos(game, mirror_to);

			Vec2 pos = vec2_lerp(from_pos_a, part->pos_t, to_pos_a);
			draw_rectangle(backbuffer, (u32)(pos.x - 0.5*part_size),
					(u32)(pos.y - 0.5*part_size), part_size, part_size, get_u32_color(color));

			Vec2 from_pos_b = get_cell_pos(game, mirror_from);
			Vec2 to_pos_b = get_cell_pos(game, part->to_pos);

			pos = vec2_lerp(from_pos_b, part->pos_t, to_pos_b);
			draw_rectangle(backbuffer, (u32)(pos.x - 0.5*part_size),
					(u32)(pos.y - 0.5*part_size), part_size, part_size, get_u32_color(color));
		}
	}
}

//
// SDL part
//
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
