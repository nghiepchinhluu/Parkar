#pragma once
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef PRINT
#include <stdio.h>
#pragma comment(linker,"/subsystem:console")
#else
#pragma comment(linker,"/subsystem:windows")
#endif

#include "Headers/SDL2-2.0.8/include/SDL.h"
#include "Headers/SDL2-2.0.8/include/SDL_image.h"
#pragma comment(lib,"Headers\\SDL2-2.0.8\\lib\\x86\\SDL2.lib")
#pragma comment(lib,"Headers\\SDL2-2.0.8\\lib\\x86\\SDL2main.lib")
#pragma comment(lib,"Headers\\SDL2-2.0.8\\lib\\x86\\SDL2_image.lib")

#pragma warning(disable:4996)
namespace Gen
{
	int screen_w = 1280;
	int screen_h = 800;

	namespace internal
	{
		struct RGB
		{
			unsigned char b, g, r;
		};

		struct Legend_Map
		{
			int n_rows;
			int n_cols;
			int *data;
		};

		struct Heatmap
		{
			int n_rows;
			int n_cols;
			float *data;
		};

		void interpolate_Heatmap(Heatmap *dest, const Heatmap *src, float alpha)
		{
			int len = src->n_cols*src->n_rows;
			for (int i = 0; i < len; i++)
			{
				dest->data[i] = dest->data[i] * alpha + src->data[i] * (1.0-alpha);
			}
		}

		void load_Legend_Map(Legend_Map *map, const unsigned char *pixels, int img_w, int img_h, int grid_size)
		{
			map->n_rows = img_h / grid_size;
			map->n_cols = img_w / grid_size;
			map->data = (int*)malloc(sizeof(int)*map->n_rows*map->n_cols);

			RGB *s = (RGB*)pixels;

			for (int i = 0; i <= img_h - grid_size; i += grid_size)
			{
				for (int j = 0; j <= img_w - grid_size; j += grid_size)
				{
					float col[3] = { 0 };
					for (int y = 0; y < grid_size; y++)
					{
						for (int x = 0; x < grid_size; x++)
						{
							int index = (y + i)*img_w + j + x;
							col[0] += s[index].r;
							col[1] += s[index].g;
							col[2] += s[index].b;

						}
					}

					int _max_i = 0;
					float col_max = col[0];
					for (int k = 1; k < 3; k++) if (col[k] > col_max) { col_max = col[k]; _max_i = k; }

					if (_max_i == 0)
					{
						map->data[(i*map->n_cols + j) / grid_size] = 1;
					}
					else if (_max_i == 1)
					{
						map->data[(i*map->n_cols + j) / grid_size] = 2;
					}
					else if (_max_i == 2)
					{
						map->data[(i*map->n_cols + j) / grid_size] = 3;
					}
				}
			}

		}

		void load_Heatmap(Heatmap *map, const unsigned char *pixels, int img_w, int img_h, int grid_size)
		{
			map->n_rows = img_h / grid_size;
			map->n_cols = img_w / grid_size;
			map->data = (float*)malloc(sizeof(float)*map->n_rows*map->n_cols);
			RGB *s = (RGB*)pixels;
			int len = map->n_rows*map->n_cols;

			for (int i = 0; i <= img_h - grid_size; i += grid_size)
			{
				for (int j = 0; j <= img_w - grid_size; j += grid_size)
				{
					float avg = 0.0;
					for (int y = 0; y < grid_size; y++)
					{
						for (int x = 0; x < grid_size; x++)
						{
							int index = (y + i)*img_w + j + x;
							avg += s[index].r;
							avg += s[index].g;
							avg += s[index].b;
						}
					}
					avg /= grid_size * grid_size*255.0*3.0;
					map->data[(i*map->n_cols + j) / grid_size] = avg;
				}
			}

		}

		namespace Demo
		{
			Heatmap heatmaps_original[100];
			int n_heatmaps = 0;
			int target_heatmap = 0;
			float heatmap_alpha = 1.0;
			float heatmap_alpha_decay = 0.999;
			float p_heatmap_target_change = 0.5;

			Heatmap heatmap_current;
			Legend_Map legend_original;
			Legend_Map legend_current;

			int *park_index_row = NULL;
			int *park_index_col = NULL;
			int n_parking = 0;

			unsigned int freq_park_state_change = 200;
			unsigned int freq_traffic_state_change = 2000;
			unsigned int last_time_park_state_changed = 0;
			unsigned int last_time_traffic_state_changed = 0;
			float p_park_state_change = 0.64;

			int *demo_packaged = NULL;

			//init loading
			void init()
			{
				SDL_Surface *surface = IMG_Load("Images/color_map.bmp");

				load_Legend_Map(&legend_original, (unsigned char*)surface->pixels, surface->w, surface->h, 32);
				load_Legend_Map(&legend_current, (unsigned char*)surface->pixels, surface->w, surface->h, 32);
				SDL_FreeSurface(surface);

				n_heatmaps = 0;
				surface = IMG_Load("Images/traffic_map0.bmp");
				load_Heatmap(&heatmaps_original[n_heatmaps++], (unsigned char*)surface->pixels, surface->w, surface->h, 32);
				SDL_FreeSurface(surface);

				surface = IMG_Load("Images/traffic_map1.bmp");
				load_Heatmap(&heatmaps_original[n_heatmaps++], (unsigned char*)surface->pixels, surface->w, surface->h, 32);
				SDL_FreeSurface(surface);

				surface = IMG_Load("Images/traffic_map2.bmp");
				load_Heatmap(&heatmaps_original[n_heatmaps++], (unsigned char*)surface->pixels, surface->w, surface->h, 32);
				load_Heatmap(&heatmap_current, (unsigned char*)surface->pixels, surface->w, surface->h, 32);
				SDL_FreeSurface(surface);

				demo_packaged = (int*)malloc(sizeof(int)*legend_current.n_rows*legend_current.n_cols);
				park_index_col = (int*)malloc(sizeof(int)*legend_current.n_rows*legend_current.n_cols);
				park_index_row = (int*)malloc(sizeof(int)*legend_current.n_rows*legend_current.n_cols);

				n_parking = 0;
				//fill in park index
				int len = legend_current.n_cols*legend_current.n_rows;
				for (int i = 0; i < len; i++)
				{
					if (legend_current.data[i] == 3)
					{
						park_index_col[n_parking] = i % legend_current.n_cols;
						park_index_row[n_parking] = i / legend_current.n_cols;
						n_parking++;
					}
				}

			}

			//grid nrows and ncols
			void get_Dimensions(int *n_rows, int *n_cols)
			{
				*n_rows = heatmap_current.n_rows;
				*n_cols = heatmap_current.n_cols;
			}
			//generate from current heatmap and current legend
			void pack(int *data, const float *heat, const int *legend, int w, int h)
			{
				int len = w * h;
				for (int i = 0; i < len; i++)
				{
					int v = (legend[i] & 0x00FF) | (((int)(heat[i] * SHRT_MAX)) << 16);
					data[i] = v;
				}
			}

			void unpack(float *heat, int *legend, const int *data, int w, int h)
			{
				int len = w * h;
				for (int i = 0; i < len; i++)
				{
					heat[i] = (float)(data[i] >> 16) / SHRT_MAX;
					legend[i] = data[i] & 0x00FF;
				}
			}

			void step(unsigned int current_time)
			{
				
				if (current_time - last_time_park_state_changed > freq_park_state_change)
				{
					last_time_park_state_changed = current_time;
					float p = (float)rand() / RAND_MAX;
					if (p <= p_park_state_change)
					{
						//flip parking to avail or unavail
						int which = rand() % n_parking;
						int x = park_index_col[which];
						int y = park_index_row[which];
						if (legend_current.data[y*legend_current.n_cols + x] == 3)
						{
							#ifdef PRINT
								printf("someone parked!\n");
							#endif
							legend_current.data[y*legend_current.n_cols + x] = 1;
						}
						else
						{
							#ifdef PRINT
								printf("someone left!\n");
							#endif
							legend_current.data[y*legend_current.n_cols + x] = 3;
						}
					}
				}

				if (current_time - last_time_traffic_state_changed > freq_traffic_state_change)
				{
					last_time_traffic_state_changed = current_time;
					//get next interpolation
					float p = (float)rand() / RAND_MAX;
					if (p <= p_heatmap_target_change)
					{
						#ifdef PRINT
							printf("heatmap interpolation target changed!\n");
						#endif
						target_heatmap = rand() % n_heatmaps;
						heatmap_alpha = 1.0;
					}

				}

				interpolate_Heatmap(&heatmap_current, &heatmaps_original[target_heatmap], heatmap_alpha);
				heatmap_alpha *= heatmap_alpha_decay;
				//printf("current alpha: %f\n", heatmap_alpha);
				//getchar();
				
			}
		}
	}

	void init(int w, int h)
	{
		screen_w = w;
		screen_h = h;
		srand(time(0));
		internal::Demo::init();
	}
	void get_Dimensions(int *n_rows, int *n_cols)
	{
		internal::Demo::get_Dimensions(n_rows, n_cols);
	}

	const int* generate(unsigned int current_time)
	{
		internal::Demo::step(current_time);
		int h, w;
		get_Dimensions(&h, &w);
		internal::Demo::pack(internal::Demo::demo_packaged, internal::Demo::heatmap_current.data, internal::Demo::legend_current.data, w, h);
		return internal::Demo::demo_packaged;
	}

}

