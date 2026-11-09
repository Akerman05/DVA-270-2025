#include <nrfx.h>
#include <nrf5340_application.h>
#include <nrfx_config.h>
#include <nrf.h>
#include <nrfx_uarte.h>
#include <nrf_gpio.h>
#include <nrfx_systick.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <nrfx_rtc.h>
#include <grupp17.h>

/* ─── Configuration ─── */

#define MAX_ROW        48          //defined as the max length of rows, and thus also how many columns there are
#define MAX_COL        48          //defined as the max length of columns, and thus also how many rows there are

/* ─── Defines ─── */

#define PIN_TXD 20
#define PIN_RXD 22

#define LED1 (28)
#define LED2 (29)
#define LED3 (30)
#define LED4 (31)

#define LED_ON_TIME 500

//Skapar en driver instance för UARTE:
nrfx_uarte_t instance = NRFX_UARTE_INSTANCE(0);

extern nrfx_uarte_t instance;

const nrfx_rtc_t rtc_instance = NRFX_RTC_INSTANCE(0);

/* ISR aka interrupts */
volatile bool tx_done = false;				//volatile tx_done flag and rx_ready signal flag to set up interrupts
volatile bool rx_ready = false;		
volatile uint8_t rx_buffer = 0;
volatile uint32_t uart_error_count = 0;

/* ─── UARTE ─── */

void uarte_handler(nrfx_uarte_event_t const* p_event, void* p_context)
{
	LED_on(1);
	(void)p_context;

	static uint32_t count = 0;

	switch (p_event->type)
	{
	case NRFX_UARTE_EVT_TX_DONE:

		/*The UARTE peripheral has completely finished using
		the transmission buffer. main() may start another TX.*/

		tx_done = true;
		break;
		

	case NRFX_UARTE_EVT_RX_DONE:
		/*Save the received byte for main().
		Do not immediately restart reception here if main()
		needs to consume rx_buffer first.*/
		rx_ready = true;
		break;

	case NRFX_UARTE_EVT_ERROR:
		uart_error_count++;

		/*Allow main() to observe that no transmission is active.
		 The program can then decide whether to retry or stop.*/
		tx_done = true;
		break;

	default:
		break;
	}
}

void init_uarte(void) {
	//Default Settings and give the RX/TX pins:
	const nrfx_uarte_config_t config = NRFX_UARTE_DEFAULT_CONFIG(20, 22);
	nrfx_systick_init();
	init_LED();
	LED_off(1); 
	LED_off(2);
	LED_off(3);
	LED_off(4);

	//Initiate UARTE:
	nrfx_err_t errr = nrfx_uarte_init(&instance, &config, uarte_handler);
	if (errr != NRFX_SUCCESS)
	{
		while (1)
		{
		}
	}
	if (errr != 0) {/*error handling if required*/ }
}

/* ─── UARTE Check ─── */
#if !NRFX_CHECK(NRFX_UARTE_ENABLED)

#if !NRFX_FEATURE_PRESENT(NRFX_UARTE, _ENABLED)
#error "No enabled UARTE instances. Check <nrfx_config.h>."
#endif

#endif//NRFX_CHECK(NRFX_UARTE_ENABLED)



/* ─── Typedef & Structs ─── */

typedef enum {  //NEW TOOL ACQUIRED - Enumerations are similar to structs and set up a constant selection of options. Thanks StackOverflow :3
	PLAYING,
	WIN,
	LOSE,
	MENU,
	PAUSE
} GameState;

typedef struct {
	int is_mine;
	int is_revealed;
	int is_flagged;
	int is_suspect;
	int neighbor_count;		//0-8, shows how many mines are nearby
} Cell;

/* ─── Globals ─── */

Cell      board[MAX_ROW][MAX_COL];		//2D array of Cell structs make up the board.
int       cursor_row = 0;		/*cursor row and column, important for player input commands*/
int       cursor_col = 0;
GameState state;                //Initialise enum GameState
int       mines_remaining;
uint8_t newline[] = "\r\n";		// \r\n because UART needs carriage return + newline
uint8_t clear[] = "\033[2J\033[H\033[3J";	//Clear screen shortcut escape sequence. Thanks Reddit :3
uint8_t red[] = "\033[91m";					//Escape sequence for red
uint8_t yellow[] = "\033[93m";				//Escape sequence for yellow
uint8_t blue[] = "\033[94m";				//Escape sequence for blue
uint8_t white[] = "\033[0m";				//Escape sequence for reset colour
int NUM_MINES = 10;
int ROW = 1;
int COL = 1;
bool first_click = true;	//First click guardrail so player doesn't immediately explode.
bool realistic_mode = false;

/* ─── UARTE Specific Functions ─── */

void write_char(char c)
{
	static char tx_char;

	tx_char = c;

	tx_done = false;

	nrfx_uarte_tx(
		&instance,
		(uint8_t*)&tx_char,
		1,
		0
	);

	while (!tx_done)
	{
	}
}

char read_char(void) {
	uint8_t c;
	while (nrfx_uarte_rx(&instance, &c, 1) != NRFX_SUCCESS);
	return (char)c;
}

/* ─── Function Prototypes ─── */

void init_board(void);
void place_mines(int safe_row, int safe_col);	//First click guardrail by providing place_mines with one specific safe spot.
void count_adjacent_mines(void);
void draw_board(void);
void reveal(int row, int col);
void chord(int row, int col);
void flood_fill(int row, int col);
void flag(int row, int col);
int  check_win(void);
void handle_input(void);
void reveal_all_cells(void);

/* ─── Functions ─── */

void init_board(void) {
	for (int boardx = 0; boardx < ROW; boardx++) {         //loop through rows, from zero to the amount of columns
		for (int boardy = 0; boardy < COL; boardy++) {     //loop through columns, from zero to the amount of rows
			board[boardx][boardy].is_mine = 0;
			board[boardx][boardy].is_revealed = 0;
			board[boardx][boardy].is_flagged = 0;
			board[boardx][boardy].neighbor_count = 0;
		}
	}

	cursor_col = rand() % COL;
	cursor_row = rand() % ROW;
	state = PLAYING;
	mines_remaining = NUM_MINES;
}

void place_mines(int safe_row, int safe_col) {
	int placed = 0;
	while (placed < NUM_MINES) {
		int row = rand() % ROW;
		int col = rand() % COL;
		if (row == safe_row &&
			col == safe_col)
		{
			continue;
		}
		if (!board[row][col].is_mine) {     //If cell on board with coordinates [row],[col] is not mine
			board[row][col].is_mine = 1;    //Make it a mine
			placed++;                       //Repeat next
		}
	}
}

void count_adjacent_mines(void) {
	for (int row = 0; row < ROW; row++) {
		for (int col = 0; col < COL; col++) {

			if (board[row][col].is_mine) {
				continue;           // mines don't need a count
			}

			int count = 0;
			for (int nr = -1; nr <= 1; nr++) {
				for (int nc = -1; nc <= 1; nc++) {
					if (nr == 0 && nc == 0) continue;

					int adjacent_row = row + nr;
					int adjacent_col = col + nc;

					if (adjacent_row >= 0 && adjacent_row < ROW &&
						adjacent_col >= 0 && adjacent_col < COL) {
						if (board[adjacent_row][adjacent_col].is_mine) {
							count++;
						}
					}
				}
			}
			board[row][col].neighbor_count = count;
		}
	}
}

void draw_board(void) {
	// ── Clear the screen ──
	write_string(clear);			// \033[2J clears all text on screen
	// \033[H  moves the terminal's text cursor to row 1, col 1

	// ── Header ──

	if (realistic_mode == true)
	{
		uint8_t mines_unknown_text[] = "Mines remaining: UNKNOWN";	//Don't print mines remaining so the player has no idea how many mines there are >:D
		write_string(mines_unknown_text);
	}
	else if (realistic_mode == false)
	{
		uint8_t mines_remaining_text[] = "Mines remaining: ";
		write_string(mines_remaining_text);      //Print mines remaining so the player knows how many flags they have left to place. write_int() handles the number.
		write_int(mines_remaining);
	}
	write_string(newline);
	write_string(newline);
	char icon = '*';

	for (int row = 0; row < ROW; row++) {
		for (int col = 0; col < COL; col++) {
			Cell cell = board[row][col];
			if (cell.is_revealed) {
				if (cell.is_mine) {
					write_string(red);
					icon = 'X';
				} else if (cell.neighbor_count == 0) {
					icon = ' ';
				} else {
					icon = '0' + cell.neighbor_count;	//I learned this clever trick from AI but it makes sense
				}
			} else if (cell.is_flagged == 1) {
				write_string(blue);
				icon = 'F';
			} else if (cell.is_suspect)
			{
				write_string(yellow);
				icon = '?';
			}
			else {
				icon = '#';
			}

			if (row == cursor_row && col == cursor_col) {
				write_char('[');
				write_char(icon);
				write_char(']');
			} else {
				write_char(' ');
				write_char(icon);
				write_char(' ');
			}
			write_string(white);
		}
		write_string(newline);
	}
	uint8_t controls_text[] = "WASD: movement  SPACE: reveal  F: flag\r\n";
	write_string(controls_text);

	uint8_t lose_text[] = "YOU HIT A MINE - GAME OVER - PRESS R TO RESTART\r\n";
	uint8_t win_text[] = "YOU WIN - PRESS R TO RESTART\r\n";
	
	if (state == LOSE) {
		write_string(lose_text);
	} else if (state == WIN) {
		write_string(win_text);
	}
}

void reveal(int row, int col) {
	if(first_click == true) {
		first_click = false;

		place_mines(row, col);
		count_adjacent_mines();
	}

	if (row < 0 || row >= ROW || col < 0 || col >= COL) return;

	if (board[row][col].is_flagged || board[row][col].is_revealed) {
		return;
	}

	if (board[row][col].is_mine) {
		state = LOSE;
		reveal_all_cells();
		return;
	}

	board[row][col].is_revealed = 1;

	if (board[row][col].neighbor_count == 0) {
		flood_fill(row, col);
	}
	check_win();
}

void chord(int row, int col) {
	int flag_count = 0;

	for (int nr = -1; nr <= 1; nr++) {			//Nested for-loops to count flags around clicked cell, borrowed from flood_fill
		for (int nc = -1; nc <= 1; nc++) {

			if (nr == 0 && nc == 0) continue;

			int adjacent_row = row + nr;
			int adjacent_col = col + nc;

			if (adjacent_row < 0 || adjacent_row >= ROW ||
				adjacent_col < 0 || adjacent_col >= COL) {
				continue;		// Skip, this neighbour doesn't exist
			}

			Cell* adjacent_cell = &board[adjacent_row][adjacent_col];

			if (adjacent_cell->is_flagged)
				flag_count++;
		}
	}
	Cell* current = &board[row][col];

	if (flag_count != current->neighbor_count)
	{
		return;
	}											//Checks surrounding flags to see if chording is possible.
	else {
		for (int nr = -1; nr <= 1; nr++) {			//Same as with flood fill, visits all neighbours, however this time it blows up the player if they mess up
			for (int nc = -1; nc <= 1; nc++) {

				if (nr == 0 && nc == 0) continue;

				int adjacent_row = row + nr;
				int adjacent_col = col + nc;

				if (adjacent_row < 0 || adjacent_row >= ROW ||
					adjacent_col < 0 || adjacent_col >= COL) {
					continue;
				}

				Cell* adjacent_cell = &board[adjacent_row][adjacent_col];

				if (adjacent_cell->is_flagged)
					continue;

				if (adjacent_cell->is_revealed)
					continue;

				reveal(adjacent_row, adjacent_col);		//reveal hidden cells and blow up mines :3
			}
		}
	}
}

void flood_fill(int row, int col) {
	for (int nr = -1; nr <= 1; nr++) {
		for (int nc = -1; nc <= 1; nc++) {
			if (nr == 0 && nc == 0) continue;

			int adjacent_row = row + nr;
			int adjacent_col = col + nc;

			if (adjacent_row < 0 || adjacent_row >= ROW ||		//Bounds check
				adjacent_col < 0 || adjacent_col >= COL) {
				continue;           // Skip, this neighbour doesn't exist
			}

			Cell* adjacent_cell = &board[adjacent_row][adjacent_col];

			if (adjacent_cell->is_revealed == 1) continue;   // already open, don't revisit
			if (adjacent_cell->is_flagged == 1)  continue;   // player flagged, don't reveal
			if (adjacent_cell->is_mine == 1)     continue;   // never auto-reveal a mine

			adjacent_cell->is_revealed = 1;

			if (adjacent_cell->neighbor_count == 0) {
				flood_fill(adjacent_row, adjacent_col);
			}
		}
	}
}

void flag(int row, int col)
{
	Cell* cell = &board[row][col];

	if (cell->is_revealed)
	{
		return;
	}

	// Hidden -> Flagged
	if (!cell->is_flagged && !cell->is_suspect)
	{
		cell->is_flagged = 1;
		mines_remaining--;
	}
	// Flagged -> Suspect
	else if (cell->is_flagged)		
	{
		cell->is_flagged = 0;
		cell->is_suspect = 1;
		mines_remaining++;
	}
	// Suspect -> Hidden
	else if (cell->is_suspect)
	{
		cell->is_suspect = 0;
	}
}

int check_win(void) {
	if (state == PLAYING) {
		for (int row = 0; row < ROW; row++) {
			for (int col = 0; col < COL; col++) {
				Cell cell = board[row][col];
				if (cell.is_mine == 0 && cell.is_revealed == 0) {
					return 0;
				}
			}
		}
		state = WIN;    // every non-mine cell revealed
		return 1;
	}
	else return 0;
}

void handle_input(void)
{
	char c = rx_buffer;

	rx_ready = false;

	switch (state)
	{
	case MENU:

		switch (c)
		{
		case '1':

			NUM_MINES = 10;
			ROW = 9;
			COL = 9;
			state = PLAYING;
			break;

		case '2':

			NUM_MINES = 25;
			ROW = 11;
			COL = 11;
			state = PLAYING;
			break;

		case '3':

			NUM_MINES = 60;
			ROW = 16;
			COL = 16;
			state = PLAYING;
			break;

		case '4':

			NUM_MINES = 160;
			ROW = 20;
			COL = 20;
			state = PLAYING;
			break;

		case '+':

			ROW = 10 + rand() % 39;    // 10-48
			COL = 10 + rand() % 39;    // 10-48

			int mine_density = 15 + rand() % 30;

			NUM_MINES = (ROW * COL * mine_density) / 100;
			state = PLAYING;
			break;
		}

		break;

	case PLAYING:

		switch (c) {
		case 'w':
			cursor_row--;
			break;
		case 'a':
			cursor_col--;
			break;
		case 's':
			cursor_row++;
			break;
		case 'd':
			cursor_col++;
			break;
		case 'f':
			flag(cursor_row, cursor_col);
			break;
		case ' ':

			if (board[cursor_row][cursor_col].is_revealed)
			{
				chord(cursor_row, cursor_col);
			}
			else
			{
				reveal(cursor_row, cursor_col);
			}

			break;
		}
		if (cursor_row < 0)      cursor_row = 0;
		if (cursor_row >= ROW)   cursor_row = ROW - 1;
		if (cursor_col < 0)      cursor_col = 0;
		if (cursor_col >= COL)   cursor_col = COL - 1;

		break;

	case WIN:
	case LOSE:

		if (c == 'r')
		{
			state = MENU;
		}

		break;
	}
}

void reveal_all_cells(void)
{
	for (int row = 0; row < ROW; row++)
	{
		for (int col = 0; col < COL; col++)
		{
			board[row][col].is_revealed = true;
		}
	}
	state = LOSE;
}

void menu(void)
{
	write_string(clear);

	uint8_t title[] =
		"====================\r\n"
		"     MINESWEEPER\r\n"
		"====================\r\n";

	uint8_t easy[] = "1 - Beginner (9x9, 10 mines)\r\n";
	uint8_t medium[] = "2 - Intermediate (11x11, 25 mines)\r\n";
	uint8_t hard[] = "3 - Expert (16x16, 60 mines)\r\n";
	uint8_t nightmare[] = "4 - NIGHTMARE (20x20, 160 mines)\r\n";
	uint8_t realistic[] = "5 - Realistic (Random size and number, Godspeed o7)\r\n";
	uint8_t diff[] = "Select difficulty: ";

	write_string(title);
	write_string(newline);
	write_string(easy);
	write_string(medium);
	write_string(hard);
	write_string(red);
	write_string(nightmare);
	write_string(white);
	write_string(diff);

	while (state == MENU)
	{
		rx_ready = false;

		nrfx_uarte_rx(
			&instance,
			(uint8_t*)&rx_buffer,
			1);

		while (!rx_ready)
		{
		}

		handle_input();
	}
}

void game_loop(void)
{
	draw_board();

	while (state != WIN && state != LOSE)
	{
		rx_ready = false;

		nrfx_uarte_rx(
			&instance,
			(uint8_t*)&rx_buffer,
			1);

		while (!rx_ready)
		{
		}
		
		handle_input();
		draw_board();
	}

	draw_board();

	while (state == WIN || state == LOSE)
	{
		rx_ready = false;

		nrfx_uarte_rx(
			&instance,
			(uint8_t*)&rx_buffer,
			1);

		while (!rx_ready)
		{
		}

		handle_input();
	}

	return;
}

void start_game(void)
{
	uint32_t seed =
		nrfx_rtc_counter_get(&rtc_instance);

	srand(seed);

	init_board();
	first_click = true;
	if (NUM_MINES >= ROW * COL)
	{
		NUM_MINES = ROW * COL - 1;
	}

	state = PLAYING;
}

/*uint32_t get_true_random_seed(void)				//I tried implementing genuine RNG but I couldn't get it to work :(
{
	uint32_t seed = 0;

	nrf_rng_task_trigger(NRF_RNG, NRF_RNG_TASK_START);

	for (int i = 0; i < 4; i++)
	{
		while (!nrf_rng_event_check(NRF_RNG,
			NRF_RNG_EVENT_VALRDY))
		{
		}

		seed |= nrf_rng_random_value_get(NRF_RNG) << (8 * i);

		nrf_rng_event_clear(NRF_RNG,
			NRF_RNG_EVENT_VALRDY);
	}

	nrf_rng_task_trigger(NRF_RNG, NRF_RNG_TASK_STOP);

	return seed;
}*/

/* ─── Main ─── */

int main(void) {
	init_uarte();

	IRQn_Type irq = NRFX_IRQ_NUMBER_GET(NRF_UARTE_INST_GET(0));

	NVIC_ClearPendingIRQ(irq);
	NVIC_EnableIRQ(irq);

	rx_ready = 0;
	nrfx_uarte_rx(&instance, (uint8_t*)&rx_buffer, 1);

	nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;

	nrfx_err_t err = nrfx_rtc_init(&rtc_instance, &config, NULL);
	nrfx_rtc_enable(&rtc_instance);

	while (1)
	{
		state = MENU;

		menu();

		start_game();

		game_loop();
	}
}