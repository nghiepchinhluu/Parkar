#pragma warning (disable:4996)
#include <stdlib.h>
#include <stdio.h> 

namespace PathFinder
{
	int Map_Width;
	int Map_Height;

	struct coordinate
	{
		int x, y;
	};

	coordinate Direction[4] = { {-1,0}, {0,-1}, {1,0}, {0,1} }; 
	coordinate* path1 = NULL;
	coordinate* path2 = NULL;
	coordinate* path3 = NULL;
	float* ParkingCost = NULL;
	coordinate* ParkingCoord = NULL;

	bool** AlreadyMin = NULL;
	float** CostToReach = NULL;
	int** PreviousStep = NULL;
	int DistanceThreshold;

	float drive_Cost(const int *data, int x, int y, int w, int h, int *fail)
	{
		*fail = 0;
		if (x >= w) { *fail = 1; return 0; }
		if (x < 0) { *fail = 1; return 0; }
		if (y >= h) { *fail = 1; return 0; }
		if (y < 0) { *fail = 1; return 0; }
		int type = data[y*w + x] & 0x00FF; // To get the 2 low bytes

		//if (type == 1) return 0; // red spot
		if (type == 1)
		{
			*fail = 1;
			return 0;
		} // red spot
		if (type == 3)
		{
			*fail = 2;
		} // parking spot
		
		float cost = data[y*w + x] >> 16; // To get the 2 high bytes
		return cost;
	}

	void Trace(coordinate* Series, const coordinate* endCoord, int w, int h)
	{
		coordinate currentCoord = *endCoord;
		for (int i = 0; i < w*h; i++)
		{
			Series[i].x = -1;
			Series[i].y = -1;
		}
		Series[0] = *endCoord;
		int step = 0;
		while (PreviousStep[currentCoord.y][currentCoord.x] >= 0)
		{
			coordinate tmpCoord = { currentCoord.x + Direction[(PreviousStep[currentCoord.y][currentCoord.x] + 2) % 4].x , currentCoord.y + Direction[(PreviousStep[currentCoord.y][currentCoord.x] + 2) % 4].y };
			step++;
			Series[step] = tmpCoord;
			currentCoord = Series[step];
		}
		
	}

	void FindThreeBestPaths(const int *data, int w, int h)
	{
		// Init
		for (int i = 0; i < w*h; i++)
		{
			ParkingCost[i] = 2147483647;
			ParkingCoord[i] = { -1,-1 };
		}
		int nParking = 0;

		// Take in all the parking spots
		for (int i = 0; i < h; i++)
		{
			for (int j = 0; j < w; j++)
			{
				int type = data[i*w + j] & 0x00FF; // To get the 2 low bytes
				//3 - parking
				if (type == 3 && CostToReach[i][j]< 2147483647)
				{
					ParkingCost[nParking] = CostToReach[i][j];
					ParkingCoord[nParking] = { j,i };
					nParking++;
				}
			}
		}

		// Sort
		for (int i = 0; i < nParking - 1; i++)
		{
			for (int j = i; j < nParking; j++)
			{
				if (ParkingCost[j] < ParkingCost[i])
				{
					float CostTmp = ParkingCost[i];
					ParkingCost[i] = ParkingCost[j];
					ParkingCost[j] = CostTmp;

					coordinate CoordTmp = ParkingCoord[i];
					ParkingCoord[i] = ParkingCoord[j];
					ParkingCoord[j] = CoordTmp;
				}
			}
		}

		int Path1Index = -1;
		int Path2Index = -1;
		int Path3Index = -1;

		// Find the best 3 parking spots
		if (nParking > 0) Path1Index = 0;

		for (int i = 0; i < nParking; i++)
		{
			int Distance = (ParkingCoord[i].x - ParkingCoord[Path1Index].x)*(ParkingCoord[i].x - ParkingCoord[Path1Index].x) + (ParkingCoord[i].y - ParkingCoord[Path1Index].y)* (ParkingCoord[i].y - ParkingCoord[Path1Index].y);
			if (Distance > DistanceThreshold)
			{
				Path2Index = i;
				break;
			}
		}

		for (int i = 0; i < nParking; i++)
		{
			int Distance1 = (ParkingCoord[i].x - ParkingCoord[Path1Index].x)*(ParkingCoord[i].x - ParkingCoord[Path1Index].x) + (ParkingCoord[i].y - ParkingCoord[Path1Index].y)* (ParkingCoord[i].y - ParkingCoord[Path1Index].y);
			int Distance2 = (ParkingCoord[i].x - ParkingCoord[Path2Index].x)*(ParkingCoord[i].x - ParkingCoord[Path2Index].x) + (ParkingCoord[i].y - ParkingCoord[Path2Index].y)* (ParkingCoord[i].y - ParkingCoord[Path2Index].y);

			if ((Distance1 > DistanceThreshold) && (Distance2 > DistanceThreshold))
			{
				Path3Index = i;
				break;
			}
		}

		if (Path1Index != -1) Trace(path1,&ParkingCoord[Path1Index], w, h);
		else path1[0] = { -1,-1 };
		if (Path2Index != -1) Trace(path2,&ParkingCoord[Path2Index], w, h);
		else path2[0] = { -1,-1 };
		if (Path3Index != -1) Trace(path3,&ParkingCoord[Path3Index], w, h);
		else path3[0] = { -1,-1 };
	}

	void printPath(coordinate* path, int w, int h)
	{
		if (path[0].x==-1 && path[0].y==-1) printf("NONE!");
		for (int i = 0; i < w*h; i++)
		{
			if ((path[i].x == -1) && (path[i].y == -1))  break;
			printf("%d\t%d\n", path[i].x, path[i].y);
		}
		printf("\n");
	}

	void Path_Finder(const int *data, int w, int h, const coordinate *start)
	{
		// Initialize
		for (int i = 0; i < h; i++)
		{
			for (int j = 0; j < w; j++)
			{
				AlreadyMin[i][j] = false;
				CostToReach[i][j] = 2147483647;
				PreviousStep[i][j] = -1;
			}
		}
		int fail = 0;
		drive_Cost(data, start->x, start->y, w, h, &fail);
		if (fail == 1) return;

		CostToReach[start->y][start->x] = 0;
		
		//TODO CL: FIX WHEN NO PATH IS SHOWN BUT PARKING SPOT APPEARS
		coordinate CurrentCoord = { -1,-1 };

		// Dijkstra
		for (;;)
		{
			float Min = 2147483647;
			coordinate MinCoord = { -1,-1 };
			for (int i = 0; i < h; i++)
			{
				for (int j = 0; j < w; j++)
				{
					if (AlreadyMin[i][j]) continue;
					float Cost = CostToReach[i][j];
					if (Cost < Min)
					{
						Min = Cost;
						MinCoord.x = j;
						MinCoord.y = i;
					}
				}
			}
			if (Min == 2147483647) break;
			CurrentCoord = MinCoord;
			AlreadyMin[CurrentCoord.y][CurrentCoord.x] = true;

			for (int i = 0; i < 4; i++)
			{
				coordinate ArrivingCoord = { CurrentCoord.x + Direction[i].x,CurrentCoord.y + Direction[i].y };

				int fail = 0;
				float Cost = drive_Cost(data, ArrivingCoord.x, ArrivingCoord.y,w,h, &fail);
				if (fail==1) continue;
				Cost += CostToReach[CurrentCoord.y][CurrentCoord.x];
				if (Cost < CostToReach[ArrivingCoord.y][ArrivingCoord.x])
				{
					CostToReach[ArrivingCoord.y][ArrivingCoord.x] = Cost;
					PreviousStep[ArrivingCoord.y][ArrivingCoord.x] = i;
				}
				if (fail == 2) AlreadyMin[ArrivingCoord.y][ArrivingCoord.x] = true;
			}
		}
	}

	void Init(int height, int width)
	{
		path1 = (coordinate*)malloc(sizeof(coordinate)*height*width);
		path2 = (coordinate*)malloc(sizeof(coordinate)*height*width);
		path3 = (coordinate*)malloc(sizeof(coordinate)*height*width);

		ParkingCost = (float*)malloc(sizeof(float)*height*width);
		ParkingCoord = (coordinate*)malloc(sizeof(coordinate)*height*width);

		CostToReach = (float**)malloc(sizeof(float*)*height);
		PreviousStep = (int**)malloc(sizeof(int*)*height);
		AlreadyMin = (bool**)malloc(sizeof(bool*)*height);
		for (int i = 0; i < height; i++)
		{
			PreviousStep[i] = (int*)malloc(sizeof(int*)*width);
			CostToReach[i] = (float*)malloc(sizeof(float*)*width);
			AlreadyMin[i] = (bool*)malloc(sizeof(bool*)*width);
		}

		DistanceThreshold = 0.15*width*width;
	}

	void handling(const int* data, const coordinate* start_car, int w, int h)
	{
		Path_Finder(data,w,h, start_car);
		FindThreeBestPaths(data, w,h);
	}
}