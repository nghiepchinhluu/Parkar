#pragma warning(disable: 4996)

#include <assert.h>

#include "Headers/Pathfinder.h"
#include "Headers/Gen.h"

namespace Draw_Grid_Cell
{
	enum { RIGHT = 0, DOWNRIGHT, DOWN, DOWNLEFT, LEFT, UPLEFT, UP, UPRIGHT };

	struct Animation_Data
	{
		SDL_Texture *texture;
		unsigned int last_update_time;
		int num_of_frames, animation_frame;
		float animation_speed, src_width, src_height;

	};

	SDL_Texture* load_Texture(SDL_Renderer *renderer, const char *name)
	{
		SDL_Surface *tex_surface = IMG_Load(name);
		assert(tex_surface);
		SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, tex_surface);
		SDL_FreeSurface(tex_surface);
		return tex;
	}

	void init_Data(Animation_Data *d, SDL_Renderer *renderer, const char* name, int num_frames, float animation_speed, float src_width, float src_height)
	{
		d->animation_frame = 0;
		d->animation_speed = animation_speed;
		d->num_of_frames = num_frames;
		d->src_height = src_height;
		d->src_width = src_width;
		d->texture = load_Texture(renderer, name);
		d->last_update_time = SDL_GetTicks();

	}

	void update(Animation_Data *d, const unsigned int current_time)
	{
		if (current_time - d->last_update_time < d->animation_speed)return;
		d->last_update_time = current_time;

		d->animation_frame++;
		d->animation_frame %= d->num_of_frames;
	
	}

	void draw_To_Screen(Animation_Data *d, SDL_Renderer *renderer, PathFinder::coordinate* pos, PathFinder::coordinate* start_pos, int cell_size, int screen_width, int screen_height, int r, int g, int b)
	{
		int i = 0;
		while (pos[i].x != -1 && pos[i].y != -1)
		{
			if (pos[i].x == start_pos->x && pos[i].y == start_pos->y) break;
			int angle = 0;
			SDL_Rect src;
			src.x = d->animation_frame * d->src_width;
			src.y = 0;
			src.w = d->src_width;
			src.h = d->src_height;

			SDL_Rect dst;
			dst.x = pos[i].x * cell_size;
			dst.y = pos[i].y * cell_size;
			dst.w = cell_size;
			dst.h = cell_size;

			if (pos[i + 1].x > pos[i].x)
			{
				if (pos[i - 1].y > pos[i].y) angle = DOWNLEFT;

				else if (pos[i - 1].y < pos[i].y) angle = UPLEFT;

				else angle = LEFT;
			}

			else if (pos[i + 1].x < pos[i].x)
			{
				if (pos[i - 1].y > pos[i].y) angle = DOWNRIGHT;

				else if (pos[i - 1].y < pos[i].y) angle = UPRIGHT;

				else angle = RIGHT;
			}

			else if (pos[i + 1].y < pos[i].y)
			{
				if (pos[i - 1].x < pos[i].x) angle = DOWNLEFT;

				else if (pos[i - 1].x > pos[i].x) angle = DOWNRIGHT;

				else angle = DOWN;
			}

			else
			{
				if (pos[i - 1].x < pos[i].x) angle = UPLEFT;

				else if (pos[i - 1].x > pos[i].x) angle = UPRIGHT;

				else angle = UP;
			}

			SDL_SetTextureAlphaMod(d->texture, 255);
			SDL_SetTextureColorMod(d->texture, r, g, b);
			SDL_RenderCopyEx(renderer, d->texture, &src, &dst, angle * 45, NULL, SDL_FLIP_NONE);
			i++;
		}
	}
}