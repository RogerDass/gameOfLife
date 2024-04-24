#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

// kbhit() is only on windows, so we'll implement our own for unix-based...
#if defined(_WIN32) || defined(_WIN64)
	#include <conio.h>
#else
	#include <termios.h>
	#include <fcntl.h>

	int kbhit(void)
	{
		struct termios oldt, newt;
		int ch;
		int oldf;

		tcgetattr(STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
		oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
		fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

		ch = getchar();

		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
		fcntl(STDIN_FILENO, F_SETFL, oldf);

		if (ch != EOF)
		{
			ungetc(ch, stdin);
			return 1;
		}

		return 0;
	}
#endif

#define DEFAULT_WIDTH 8
#define DEFAULT_HEIGHT 8
#define DEFAULT_DELAY 1000
#define DEAD '.'
#define LIVE 'X'

unsigned int delayMilliseconds = DEFAULT_DELAY;
unsigned int currentGeneration = 0;
unsigned int maxGenerations = 0;
unsigned int shouldPrintSequence = 0;

typedef enum
{
	NONE,
	RANDOM,
	BEACON,
	BLINKER,
	TOAD
} Pattern;

typedef struct
{
	unsigned int width;
	unsigned int height;
	int **data;
} Grid;

void initGridDataWithPattern(Grid *grid, Pattern p)
{
	int width = grid->width;
	int height = grid->height;

	grid->data = malloc(height * sizeof(int*));
	if (grid->data == NULL)
	{
		printf("error: failed to allocate memory for grid rows");
		exit(EXIT_FAILURE);
	}

	for (int i=0; i < height; i++)
	{
		grid->data[i] = calloc(width, sizeof(int));
		if (grid->data[i] == NULL)
		{
			printf("error: failed to allocate memory for grid columns");
			for (int j = 0; j < i; j++)
			{
				free(grid->data[j]); // free previously alloc'd memory
			}
			free(grid->data);
			exit(EXIT_FAILURE);
		}
	}

	int **data = grid->data;
	switch (p)
	{
		case BLINKER:
			data[height/2 - 1][width/2] = 1;
			data[height/2]    [width/2] = 1;
			data[height/2 + 1][width/2] = 1;
			break;

		case BEACON:
			data[height/2][width/2] = 1;
			data[height/2][width/2 - 1] = 1;
			data[height/2 - 1][width/2] = 1;
			data[height/2 - 1][width/2 - 1] = 1;
			data[height/2 - 2][width/2 + 1] = 1;
			data[height/2 - 2][width/2 + 2] = 1;
			data[height/2 - 3][width/2 + 1] = 1;
			data[height/2 - 3][width/2 + 2] = 1;
			break;

		case TOAD:
			data[height/2][width/2 - 2] = 1;
			data[height/2][width/2 - 1] = 1;
			data[height/2][width/2] = 1;
			data[height/2 - 1][width/2 - 1] = 1;
			data[height/2 - 1][width/2] = 1;
			data[height/2 - 1][width/2 + 1] = 1;
			break;

		case RANDOM:
			srand(time(NULL));
			for (int i=0; i < height; i++)
			{
				for (int j = 0; j < width; j++)
				{
					data[i][j] = rand() % 2; // randomly 0 or 1
				}
			}
			break;

		case NONE: break;
	}
}

void initGridData(Grid *grid)
{
	Pattern p = NONE;
	initGridDataWithPattern(grid, p);
}

void freeGridData(Grid grid)
{
	for (int i = 0; i < grid.height; i++)
	{
		free(grid.data[i]);
		grid.data[i] = NULL;
	}
	free(grid.data);
	grid.data = NULL;
}

void clearScreen()
{
	#if defined(_WIN32) || defined(_WIN64)
		system("cls");
	#else
		system("clear");
	#endif
}

void display(Grid grid)
{
	if (!grid.data)
	{
		printf("error: grid data not initialized");
		exit(EXIT_FAILURE);
	}

	if (!shouldPrintSequence)
	{
		clearScreen();
	}
	printf("\nCurrent generation: %u\n", currentGeneration);

	for (int i = 0; i < grid.height; i++)
	{
		for (int j = 0; j < grid.width; j++)
		{
			printf(" %c", grid.data[i][j] ? LIVE : DEAD);
		}
		printf("\n");
	}

	if (!shouldPrintSequence)
	{
		printf("(press any key to quit)\n");
	}
}

void update(Grid grid)
{
	int width = grid.width;
	int height = grid.height;

	Grid newGrid = {.width = width, .height = height, .data = NULL};
	initGridData(&newGrid);

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int liveNeighbors = 0;
			for (int y = -1; y <= 1; y++)
			{
				for (int x = -1; x <= 1; x++)
				{
					if (x == 0 && y == 0) continue;
					int ni = (i + y + height) % height;
					int nj = (j + x + width) % width;
					if (grid.data[ni][nj]) liveNeighbors++;
				}
			}

			if (grid.data[i][j] && (liveNeighbors == 2 || liveNeighbors == 3))
			{
				newGrid.data[i][j] = 1;
			}
			else if (!grid.data[i][j] && liveNeighbors == 3)
			{
				newGrid.data[i][j] = 1;
			}
			else
			{
				newGrid.data[i][j] = 0;
			}
		}
	}

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			grid.data[i][j] = newGrid.data[i][j];
		}
	}

	freeGridData(newGrid);
}


int main(int argc, char **argv)
{
	Grid grid = {.width = DEFAULT_WIDTH, .height = DEFAULT_HEIGHT, .data = NULL};
	Pattern pattern = RANDOM;

	// parse command line arguments
	if (argc == 2 && (!strcmp(argv[1], "-?") || !strcmp(argv[1], "--help")))
	{
		printf("\nUsage:\n gameOfLife -(w|h|d|g|p) <value> \n options: ");
		printf("\n -w / --width (min = 8) = width\n -h / --height (min = 8) = height\n -d / --delay = delay between updates (in ms)");
		printf("\n -g / --generations = number of generations to quit after\n -p / --pattern = initial pattern (blinker|beacon|random|toad)\n");
		exit(0);
	}

	for (int i = 1; i < argc; i++)
	{
		if ((!strcmp(argv[i], "-w") || !strcmp(argv[i], "--width")) && i + 1 < argc)
		{
			grid.width = atoi(argv[i+1]);
			if (grid.width < 8)
			{
				grid.width = 8;
			}
		}
		else if ((!strcmp(argv[i], "-h") || !strcmp(argv[i], "--height"))  && i + 1 < argc)
		{
			grid.height = atoi(argv[i+1]);
			if (grid.height < 8)
			{
				grid.height = 8;
			}
		}
		else if ((!strcmp(argv[i], "-d") || !strcmp(argv[i],"--delay")) && i + 1 < argc)
		{
			delayMilliseconds = atoi(argv[i+1]);
		}
		else if ((!strcmp(argv[i], "-g") || !strcmp(argv[i],"--generations")) && i + 1 < argc)
		{
			maxGenerations = atoi(argv[i+1]);
		}
		else if ((!strcmp(argv[i], "-p") || !strcmp(argv[i], "--pattern")) && i + 1 < argc)
		{
			if (strcmp(argv[i+1], "blinker") == 0)
			{
				pattern = BLINKER;
			}
			else if (strcmp(argv[i+1], "beacon") == 0)
			{
				pattern = BEACON;
			}
			else if (strcmp(argv[i+1], "toad") == 0)
			{
				pattern = TOAD;
			}
		}
		else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--sequence"))
		{
			shouldPrintSequence = 1;
		}
	}

	initGridDataWithPattern(&grid, pattern);

	while (!maxGenerations || maxGenerations && currentGeneration < maxGenerations)
	{
		display(grid);
		update(grid);
		usleep(delayMilliseconds*1000);

		if (kbhit())
		{
			break; // exit when user hits a key
		}

		currentGeneration++;
	}

	freeGridData(grid);

	return 0;
}
