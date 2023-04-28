#include <ncurses.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <vector>
#include <time.h>
#include <math.h>
using namespace std;

int exitFlag = 0;
int yMax, xMax;

// DEFINE OVERALL BOARD ARRAY (2D ARRAY)
#define BOARD_HEIGHT 25
#define BOARD_WIDTH 12
unsigned char *board = nullptr;

// DEFINE WINDOW SCREEN SIZE
#define SCREEN_HEIGHT 25
#define SCREEN_WIDTH 24

// DEFINE TETROMINO
wstring tetromino[7];

void create_tetromino()
{
	tetromino[0].append(L"..X...X...X...X.");     // Tetronimos 4x4
	tetromino[1].append(L"..X..XX...X.....");
	tetromino[2].append(L".....XX..XX.....");
	tetromino[3].append(L"..X..XX..X......");
	tetromino[4].append(L".X...XX...X.....");
	tetromino[5].append(L".X...X...XX.....");
	tetromino[6].append(L"..X...X..XX.....");
	tetromino[7].append(L"................");
}

void start_ncurses()
{
	initscr();
	cbreak();
        noecho();
	curs_set(0);
}

void menu_screen()
{
	// get Screen Size
	getmaxyx(stdscr, yMax, xMax);

	// Create a window for our input
	WINDOW *inputwin = newwin(4, 15, 3, xMax/2 - SCREEN_WIDTH);
	WINDOW *instruction = newwin(15, 20, 3, xMax/2 - SCREEN_WIDTH + 16);
	box(inputwin, 0, 0);
	box(instruction, 0, 0);

	// PRINT INSTRUCTIONS
	wattron(instruction, A_BOLD);
	mvwprintw(instruction, 1, 1, "MENU CONTROLS");
	wattroff(instruction, A_BOLD);
	mvwprintw(instruction, 3, 1, "SELECT: UP/DOWN");
	mvwprintw(instruction, 4, 1, "CHOOSE: Z/X");
	for (int i = 1; i < 20 - 1; i += 1) mvwprintw(instruction, 5, i, "=");
	wattron(instruction, A_BOLD);
	mvwprintw(instruction, 6,1, "IN GAME CONTROLS");
	wattroff(instruction, A_BOLD);
	mvwprintw(instruction, 8,1, "MOVE: ARROW KEYS");
	mvwprintw(instruction, 9,1, "ROTATE L|R: Z/X");
	mvwprintw(instruction, 10,1, "HARD DROP: SPACE");
	mvwprintw(instruction, 11,1, "HOLD: C");

	refresh();
	wrefresh(inputwin);
	wrefresh(instruction);
        
	// Allow arrow-keys
	keypad(inputwin, true);
    
	// MENU FUNCTIONS
	string choices[2] = {"Start Game", "Exit"};
	int choice;
	int highlight = 0;

	while(1)
	{
		for(int i = 0; i < 2; i++)
		{
			if (i == highlight)
			{
				wattron(inputwin, A_REVERSE);
			}
			mvwprintw(inputwin, i+1, 1, choices[i].c_str());
			wattroff(inputwin, A_REVERSE);
		}
		choice = wgetch(inputwin);

		switch(choice)
		{
			case KEY_UP:
				if (highlight != 0) highlight--;
				break;
			case KEY_DOWN:
				if (highlight != 1) highlight++;
				break;
			default:
				break;
		}

		if (choice == 120 || choice == 122)
			break;
	}

    clear();

	if (highlight == 0) 
	{
		mvprintw(5, xMax/2 - SCREEN_WIDTH, "Starting Game... Press any key to continue");
	}
	else {
		mvprintw(5, xMax/2 - SCREEN_WIDTH, "Exiting.... Press any key to continue");
		exitFlag = 1;
	}
	getch();
}

void initialise_board()
{
	board = new unsigned char[BOARD_HEIGHT * BOARD_WIDTH];
	for (int x = 0; x < BOARD_WIDTH; x += 1)
	{
		for (int y = 0; y < BOARD_HEIGHT; y += 1)
		{
			board[(y * BOARD_WIDTH) + x] = (x == 0 || x == BOARD_WIDTH - 1 || y == BOARD_HEIGHT - 1) ? 9 : 0;
		}
	}
}

void initialise_color()
{
	init_pair(1, COLOR_BLACK, COLOR_RED);
	init_pair(2, COLOR_BLACK, COLOR_CYAN);
	init_pair(3, COLOR_BLACK, COLOR_BLUE);
	init_pair(4, COLOR_BLACK, COLOR_YELLOW);
	init_pair(5, COLOR_BLACK, COLOR_WHITE);
	init_pair(6, COLOR_BLACK, COLOR_GREEN);
	init_pair(7, COLOR_BLACK, COLOR_MAGENTA);
	init_pair(8, COLOR_BLACK, COLOR_BLACK);
}

int rotate(int pos_x, int pos_y, int rota)
{
	int index = 0;
	switch(abs(rota) % 4)
	{
		case 0:
			index = pos_y * 4 + pos_x;
			break;
		case 1:
			index = 12 + pos_y - (pos_x *4);
			break;
		case 2:
			index = 15 - (pos_y * 4) - (pos_x);
			break;
		case 3:
			index = 3 - pos_y + (pos_x * 4);
			break;
	}
	return index;
}

bool checkFit(int t_piece, int rota, int field_x, int field_y)
{
	// Check every index of the 4x4 array tetromino 
	for (int pos_x = 0; pos_x < 4; pos_x += 1)
	{
		for (int pos_y = 0; pos_y < 4; pos_y += 1)
		{
            // Index within the tetromino piece
			int pi = rotate(pos_x, pos_y, rota);

			// The specific index of the tetromino in the game field
			int fi = (field_y + pos_y) * BOARD_WIDTH + (field_x + pos_x);

			if (field_x + pos_x >= 0 && field_x + pos_x < BOARD_WIDTH)
			{
				if (field_y + pos_y >= 0 && field_y + pos_y < BOARD_HEIGHT)
				{
					if (tetromino[t_piece][pi] != L'.' && board[fi] != 0)
					{
						return false;
					}
				}
			}
		}
	}
	return true;
}

void draw_tetromino(WINDOW *win, int tetris_piece)
{
	for (int x = 0; x < 16; x += 1)
	{
		int px = x / 4;
		int py = x % 4;
		int n;
		if (tetromino[tetris_piece][x] != L'.')
		{
			n = (tetris_piece == 0 || tetris_piece == 2) ? 2:3;
			wattron(win, COLOR_PAIR(tetris_piece + 1));
			mvwprintw(win, 2 + py, n + (px * 2), "  ");
			wattroff(win, COLOR_PAIR(tetris_piece + 1));
		}
	}
}

int main()
{
	start_ncurses();
	create_tetromino();
	start_color();
	initialise_color();

	while (!exitFlag)
	{
		// MENU LOOP 
                clear();
		menu_screen();

		if (exitFlag == 1)
		{
			clear();
			endwin();
			return 0;
		}

		// MAIN GAME LOOP 

		// SETUP FOR DISPLAY (GAMEPLAY + SCORE BOARD + NEXT PIECE)
		clear();
	        initialise_board();
		WINDOW *game = newwin(SCREEN_HEIGHT, SCREEN_WIDTH - 2, 3, xMax/2 - SCREEN_WIDTH);
		WINDOW *score = newwin(SCREEN_HEIGHT / 5, SCREEN_WIDTH, 3, xMax/2 - 2);
		WINDOW *next_piece = newwin(SCREEN_HEIGHT / 5 + 2, SCREEN_WIDTH / 2, 3, xMax/2 - SCREEN_WIDTH - (SCREEN_WIDTH / 2));
		WINDOW *hold_piece = newwin(SCREEN_HEIGHT / 5 + 2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 5 + 6, xMax/2 - SCREEN_WIDTH - (SCREEN_WIDTH / 2));
		box(game, 0, 0);
		box(score, 0, 0);
		box(next_piece, 0, 0);
		box(hold_piece, 0, 0);
		keypad(game, game);
		nodelay(game, TRUE);
		refresh();
		wrefresh(game);
		wrefresh(score);
		wrefresh(next_piece);
		wrefresh(hold_piece);
		srand(time(0));

                long points = 0;
		long level = 1;
		long num_lines = 0;

		int current_piece = rand() % 7;
		int succeeding_piece = rand() % 7;
		int hold = 7;
		bool holdFlag = false;

		int current_rotation = 0;
		int currentX = BOARD_WIDTH / 3;
		int currentY = 0;
		bool forcedown = false;
		int speedcounter = 0;
		int speed = 1000;

		vector<int> vLines;
		bool gameover = false;

		while (!gameover)
		{
			// PRINT SCORE
			mvwprintw(score, 1, 1, "LEVEL: %ld", level);
                        mvwprintw(score, 2, 1, "SCORE: %ld", points);
			mvwprintw(score, 3, 1, "LINES: %ld", num_lines);
			wrefresh(score);
            
			// PRINT NEXT PIECE
			box(next_piece, 0, 0);
			mvwprintw(next_piece, 1, 1, "NEXT:");
			draw_tetromino(next_piece, succeeding_piece);
			wrefresh(next_piece);

			// PRINT HOLD PIECE
			box(hold_piece, 0, 0);
			mvwprintw(hold_piece, 1, 1, "HOLD:");
			draw_tetromino(hold_piece, hold);
			wrefresh(hold_piece);

			// TIMING
			this_thread::sleep_for(1ms);
			speedcounter += 1;
			forcedown = (speedcounter == speed);

			// PLAYER MOVEMENT
			int ch = wgetch(game);
			if (ch != ERR)
			{
				currentX += (ch == KEY_RIGHT && checkFit(current_piece, current_rotation, currentX + 1, currentY));
				currentX -= (ch == KEY_LEFT && checkFit(current_piece, current_rotation, currentX - 1, currentY));
				currentY += (ch == KEY_DOWN && checkFit(current_piece, current_rotation, currentX, currentY + 1));
				current_rotation += (ch == 'x') && (checkFit(current_piece, current_rotation + 1, currentX, currentY));
				current_rotation -= (ch == 'z') && (checkFit(current_piece, current_rotation - 1, currentX, currentY));
	
				if (ch == ' ' || ch == KEY_UP)
				{
					while (checkFit(current_piece, current_rotation, currentX, currentY + 1))
					{
						currentY += 1;
					}
					forcedown = true;
				}

				if (holdFlag == false && ch == 'c')
				{
					holdFlag = true;
					int temp_piece = current_piece;
					current_piece = hold;
					hold = temp_piece;

					if (current_piece == 7)
					{
						current_piece = succeeding_piece;
						succeeding_piece = rand() % 7;
					} 
					currentX = BOARD_WIDTH / 3;
					currentY = 0;
					current_rotation = 0;

					speedcounter = 0;
					werase(next_piece);
					werase(hold_piece);
				}
			}

			flushinp();

			if (forcedown)
			{
				speedcounter = 0;

				if (checkFit(current_piece, current_rotation, currentX, currentY + 1))
				{
					currentY += 1;
				}
				else
				{
					// SLIGHT DELAY TO MOVE LEFT/RIGHT TO IMPLEMENT IN THE FUTURE I GUESS
                                        

					// LOCK THE PIECES ON BOARD
					for (int px = 0; px < 4; px += 1)
					{
						for (int py = 0; py < 4; py += 1)
						{
							if (tetromino[current_piece][rotate(px, py, current_rotation)] != L'.')
							{
								board[(currentY + py) * BOARD_WIDTH + (currentX + px)] = current_piece + 1;
							}
						}
					}

					// CHECK FOR LINE CLEARS
					for (int py = 0; py < 4; py++)
					{
						if (currentY + py < (int)BOARD_HEIGHT - 1)
						{
							bool line_clr = true;
							for (int px = 1; px < BOARD_WIDTH; px += 1)
							{
								if (board[(currentY + py) * BOARD_WIDTH + px] == 0)
								{
									line_clr = false;
								}
							}
							if (line_clr)
							{
								vLines.push_back(currentY + py);
							}
						}
					}
                    
					// CALCULATE SCORES 
					points += 8;
					if (!vLines.empty())
					{
						int line_cnt = vLines.size();
						switch (line_cnt)
						{
						case 1:
							points += (100 * level);
							num_lines += 1;
							break;
						case 2:
							points += (300 * level);
							num_lines += 2;
							break;
						case 3:
							points += (500 * level);
							num_lines += 3;
							break;
						case 4:
							points += (800 * level);
							num_lines += 4;
							break;
						default:
							break;
						}
					}

					level = (num_lines / 10) + 1;

					if (level <= 10) 
					{
						speed = 1000 - (90 * level);
					}
					else 
					{
						speed = 50;
					}

					// GET NEXT TETROMINO PIECE
					current_piece = succeeding_piece;
					currentX = BOARD_WIDTH / 3;
					currentY = 0;
					current_rotation = 0;
					succeeding_piece = rand() % 7;
					if (succeeding_piece == current_piece)
					{
						succeeding_piece = rand() % 7;
					}
					werase(next_piece);
					holdFlag = false;

					gameover = !checkFit(current_piece, current_rotation, currentX, currentY);
				}
			}

			// REMOVE CLEARED LINES
			if (!vLines.empty())
			{
				for (auto &v : vLines)
					for (int px = 1; px < BOARD_WIDTH - 1; px += 1)
					{
						for (int py = v; py > 0; py -= 1)
							board[py * BOARD_WIDTH + px] = board[(py - 1) * BOARD_WIDTH + px];
						board[px] = 0;
					}

				vLines.clear();
			}

			// DRAW GAME FIELD TO SCREEN
			for (int x = 0; x < BOARD_WIDTH; x += 1)
			{
				for (int y = 0; y < BOARD_HEIGHT; y += 1)
				{
					unsigned char value = board[y * BOARD_WIDTH + x];
					if (value == 0)
					{
						wattron(game, COLOR_PAIR(8));
						mvwprintw(game, y, x * 2 - 1, "  ");
						wattroff(game, COLOR_PAIR(8));
					}
					else if (value != 9 && value != 8)
					{
						wattron(game, COLOR_PAIR(value));
						mvwprintw(game, y, x * 2 - 1, "  ");
						wattroff(game, COLOR_PAIR(value));
					}
				}
			}

			// DRAW GAME PIECE && GHOST PIECE TO SCREEN
			int ghostY = currentY;

			while (checkFit(current_piece, current_rotation, currentX, ghostY + 1))
			{
				ghostY += 1;
			}

			for (int px = 0; px < 4; px += 1)
			{
				for (int py = 0; py < 4; py += 1)
				{
					if (tetromino[current_piece][rotate(px, py, current_rotation)] != L'.')
					{
						wattron(game, A_DIM);
						mvwprintw(game, ghostY + py, (currentX + px) * 2 - 1, "XX");
						wattroff(game, A_DIM);

						wattron(game, COLOR_PAIR(current_piece + 1));
						mvwprintw(game, currentY + py, (currentX + px) * 2 - 1, "  ");
						wattroff(game, COLOR_PAIR(current_piece + 1));
					}
				}
			}
			wrefresh(game);
		}
		clear();
		nodelay(game, FALSE);
		mvprintw(yMax/2 - 4, xMax/2 - 15, "Game Over");
		mvprintw(yMax/2 - 3, xMax/2 - 15, "Final Score: %ld", points);
		refresh();
		getch();
	}
	// Make sure program waits before exiting
		getch();
		endwin();
		return 0;
}
