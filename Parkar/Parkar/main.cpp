//#define DEBUG
//#define PRINT

#ifdef DEBUG
#define PRINT
#include <Windows.h>
#endif

#include "Headers/DrawTexture.h"

int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_VIDEO);
	unsigned int window_w = 1280;
	unsigned int window_h = 800;
	SDL_Window* window = SDL_CreateWindow("Parkar", SDL_WINDOWPOS_CENTERED, 0, window_w, window_h, SDL_WINDOW_BORDERLESS);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	unsigned int current_time = SDL_GetTicks();
	unsigned int last_drive_update_time = 0;

	SDL_Texture* no_park_texture = Draw_Grid_Cell::load_Texture(renderer, "Images/no_park.png");
	SDL_Texture* park_here_texture = Draw_Grid_Cell::load_Texture(renderer, "Images/park_here.png");

	int mouse_x = 0;
	int mouse_y = 0;
	char w_state = 0;
	char a_state = 0;
	char s_state = 0;
	char d_state = 0;
	char space_state = 0;

	SDL_Texture* car_texture = Draw_Grid_Cell::load_Texture(renderer, "Images/car.png");
	SDL_Rect car_src;
	car_src.x = 0;
	car_src.y = 0;
	car_src.w = 64;
	car_src.h = 64;
	SDL_Rect car_dest;
	car_dest.x = 128;
	car_dest.y = 0;
	car_dest.w = 32;
	car_dest.h = 32;

	PathFinder::coordinate car_grid_pos;
	car_grid_pos.x = (car_dest.x + 16) / 32;
	car_grid_pos.y = (car_dest.y + 16) / 32;

	SDL_Texture* map_texture = Draw_Grid_Cell::load_Texture(renderer, "Images/original_map.png");
	SDL_Rect map_src;
	map_src.x = 0;
	map_src.y = 0;
	map_src.w = 1280;
	map_src.h = 800;
	SDL_Rect map_dest;
	map_dest.x = 0;
	map_dest.y = 0;
	map_dest.w = window_w;
	map_dest.h = window_h;

	SDL_Texture* heatmap_texture = Draw_Grid_Cell::load_Texture(renderer, "Images/color_map.png");
	SDL_SetTextureBlendMode(heatmap_texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(heatmap_texture, 50);

	SDL_Texture *traffic_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, window_w, window_h);
	unsigned char *traffic_pixels = (unsigned char*)malloc(window_w*window_h * 4);

	//levent init
	Gen::init(window_w, window_h);
	int n_cols, n_rows;
	Gen::get_Dimensions(&n_rows, &n_cols);


	//Chinh init
	PathFinder::Init(window_h/32, window_w/32);

	//Alex init
	Draw_Grid_Cell::Animation_Data path_data;
	Draw_Grid_Cell::init_Data(&path_data, renderer, "Images/arrow_sprite_sheet.png", 4, 150, 64, 64);

	bool continue_running = true;
	while(continue_running)
	{
		current_time = SDL_GetTicks();	
		SDL_Event event;
		while (SDL_PollEvent(&event) == 1)
		{
			SDL_GetMouseState(&mouse_x, &mouse_y);
			if (event.type == SDL_QUIT) continue_running = false;
			else if (event.type == SDL_KEYDOWN)
			{
				char key = event.key.keysym.sym;
				if (key == SDLK_ESCAPE)
				{
					continue_running = false;
				}
				else if (key == SDLK_w) w_state = 1;
				else if (key == SDLK_a) a_state = 1;
				else if (key == SDLK_s) s_state = 1;
				else if (key == SDLK_d) d_state = 1;
				else if (key == SDLK_SPACE) space_state = 1;
			}
			else if (event.type == SDL_KEYUP)
			{
				char key = event.key.keysym.sym;
				if (key == SDLK_w) w_state = 0;
				else if (key == SDLK_a) a_state = 0;
				else if (key == SDLK_s) s_state = 0;
				else if (key == SDLK_d) d_state = 0;
				else if (key == SDLK_SPACE) space_state = 0;
			}
		}

		int mx, my;
		SDL_GetMouseState(&mx, &my);

		if (current_time - last_drive_update_time >= 2)
		{
			last_drive_update_time = current_time;

			if (w_state == 1 && car_dest.y > 0) car_dest.y--;
			if (a_state == 1 && car_dest.x > 0) car_dest.x--;
			if (s_state == 1 && car_dest.y < window_h - car_dest.h) car_dest.y++;
			if (d_state == 1 && car_dest.x < window_w - car_dest.w) car_dest.x++;
			
			//TODO: overwrite
			car_dest.x = mx - 16;
			car_dest.y = my - 16;

			car_grid_pos.x = (car_dest.x + 16) / 32;
			car_grid_pos.y = (car_dest.y + 16) / 32;
			//printf("car pos grid %d %d\n", car_grid_pos.x, car_grid_pos.y);
		}

		//levent func
		const int *data = Gen::generate(current_time);
		//Chinh func
		PathFinder::handling(data, &car_grid_pos,n_cols,n_rows);

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_RenderCopyEx(renderer, map_texture, &map_src, &map_dest, 0, NULL, SDL_FLIP_NONE);
		if (space_state == 1) SDL_RenderCopyEx(renderer, heatmap_texture, &map_src, &map_dest, 0, NULL, SDL_FLIP_NONE);

		SDL_SetTextureBlendMode(traffic_texture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureAlphaMod(traffic_texture, 255);
		int pitch = window_w * 4;
		SDL_LockTexture(traffic_texture, NULL, (void**)&traffic_pixels, &pitch);

		for (int i = 0; i <= window_h - 32; i += 32)
		{
			for (int j = 0; j <= window_w - 32; j += 32)
			{
				for (int y = 0; y < 32; y++)
				{
					for (int x = 0; x < 32; x++)
					{
						int index = (y + i)*window_w + j + x;
						if (Gen::internal::Demo::heatmap_current.data[(i*Gen::internal::Demo::heatmap_current.n_cols + j) / 32] < 0.99)
						{
							traffic_pixels[index * 4] = 255 - 255 * Gen::internal::Demo::heatmap_current.data[(i*Gen::internal::Demo::heatmap_current.n_cols + j) / 32];
							traffic_pixels[index * 4 + 1] = 0;
							traffic_pixels[index * 4 + 2] = 0;
							traffic_pixels[index * 4 + 3] = 150;
						}
						else
						{
							traffic_pixels[index * 4 + 3] = 0;
						}

					}
				}
			}
		}

		SDL_UnlockTexture(traffic_texture);
		SDL_RenderCopyEx(renderer, traffic_texture, NULL, NULL, 0, NULL, SDL_FLIP_NONE);

		Draw_Grid_Cell::update(&path_data, current_time);
		Draw_Grid_Cell::draw_To_Screen(&path_data, renderer, PathFinder::path3, &car_grid_pos, 32, window_w, window_h, 255,50,50);
		Draw_Grid_Cell::draw_To_Screen(&path_data, renderer, PathFinder::path2, &car_grid_pos, 32, window_w, window_h, 255, 255, 50);
		Draw_Grid_Cell::draw_To_Screen(&path_data, renderer, PathFinder::path1, &car_grid_pos, 32, window_w, window_h, 50, 255, 50);

		for (int i = 0; i < Gen::internal::Demo::n_parking; i++)
		{
			int y = Gen::internal::Demo::park_index_row[i];
			int x = Gen::internal::Demo::park_index_col[i];
			int index = y * Gen::internal::Demo::legend_current.n_cols + x;
			if (Gen::internal::Demo::legend_current.data[index] == 1)
			{
				SDL_Rect dest = { x * 32,y * 32, 32, 32 };
				SDL_RenderCopyEx(renderer, no_park_texture, NULL, &dest, 0.0, NULL, SDL_FLIP_NONE);
			}
		}

		{
			if (PathFinder::path1[0].x >= 0 && PathFinder::path1[0].y >= 0)
			{
				SDL_Rect dest = { PathFinder::path1[0].x * 32,PathFinder::path1[0].y * 32, 32, 32 };
				SDL_RenderCopyEx(renderer, park_here_texture, NULL, &dest, 0.0, NULL, SDL_FLIP_NONE);
			}
			if (PathFinder::path2[0].x >= 0 && PathFinder::path2[0].y >= 0)
			{
				SDL_Rect dest = { PathFinder::path2[0].x * 32,PathFinder::path2[0].y * 32, 32, 32 };
				SDL_RenderCopyEx(renderer, park_here_texture, NULL, &dest, 0.0, NULL, SDL_FLIP_NONE);
			}
			if (PathFinder::path3[0].x >= 0 && PathFinder::path3[0].y >= 0)
			{
				SDL_Rect dest = { PathFinder::path3[0].x * 32,PathFinder::path3[0].y * 32, 32, 32 };
				SDL_RenderCopyEx(renderer, park_here_texture, NULL, &dest, 0.0, NULL, SDL_FLIP_NONE);
			}
		}
		SDL_RenderCopyEx(renderer, car_texture, &car_src, &car_dest, 0, NULL, SDL_FLIP_NONE);

		SDL_RenderPresent(renderer);
	}

	#ifdef DEBUG
	printf("System closed. ");
	system("pause");
	#endif

	return 0;
}