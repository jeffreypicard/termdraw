/*
 * termdraw.c
 *
 * Author: Jeffrey Picard
 */
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include <stdlib.h>

#include <termios.h>

struct screen_state_t {
	/* The current location of the cursor in x, y coords */
	int cur_x;
	int cur_y;
	/* Size of the screen in cols, rows */
	int cols;
	int rows;
	/* pointer into the state buffer that is 
	 * positioned at the cursor location */
	char *cur_ptr;
	/* buffer holder the current screen state */
	char *buf;
};

#define CURSOR_UP_N(n) printf("\033[%dA", n)
#define CURSOR_DOWN_N(n) printf("\033[%dB", n)
#define CURSOR_LEFT_N(n) printf("\033[%dD", n)
#define CURSOR_RIGHT_N(n) printf("\033[%dC", n)
#define CURSOR_SET(x, y) printf("\033[%d;%dH", y, x)

int
init_screen(struct screen_state_t *screen, int x, int y)
{
	int i;

	screen->cols = x;
	screen->rows = y;
	screen->cur_x = 1;
	screen->cur_y = 1;

	screen->buf = calloc( x * y, sizeof(char));
	if (!screen->buf)
		return -1;
	for (i = 0; i < x * y; i++)
		screen->buf[i] = '&';

	screen->cur_ptr = screen->buf;

	return 0;
}

void
draw_screen(struct screen_state_t *screen)
{
	int i, j;
	char *cur = screen->buf;
	CURSOR_SET(1, 1);
	for (i = 0; i < screen->rows; i++) {
		for (j = 0; j < screen->cols; j++) {
			printf("%c", *cur++);
		}
		printf("\n");
	}
	CURSOR_SET(screen->cur_x, screen->cur_y);
	fflush(stdout);
}

/*
 * set_term_environ
 *
 * Here we set the terminal environment through the termios struct.
 * We turn off canonical input, so we do *not* wait for '\n' before
 * we get input data. We also turn off echoing, so the typed input
 * it not written to the screen. Lastly we set c_cc[VMIN] = 1 so that
 * we return with input as soon as one character is available and we
 * set c_cc[VTIME] = 0 so we never time out but wait indefinitely for
 * the user to provide input.
 */
int
set_term_environ(void)
{
	struct termios old = {0};
	if (tcgetattr(0, &old) < 0) {
		perror("tcgetattr()");
		return -1;
	}
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &old) < 0) {
		perror("tcsetattr ICANON");
		return -1;
	}
	return 0;
}

/*
 * cleanup
 *
 * Be a well behaved program and leave free memory, reset termios, etc.
 * Is this really needed though?
 */
int
cleanup(void) 
{
	struct termios old = {0};
	if (tcgetattr(0, &old) < 0) {
		perror("tcsetattr()");
		return -1;
	}
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	if (tcsetattr(0, TCSADRAIN, &old) < 0) {
		perror ("tcsetattr ~ICANON");
		return -1;
	}
	return 0;
}

/*
 * move_cursor
 * screen: screen state struct
 * direction: direction of movement
 * n: number of places to move
 *
 * Moves the cursor n places up, down, left or right. Updates
 * the current location of the cursor in the screen struct as
 * a side effect. Does not allow movement outside of the screen
 * window.
 */
int
move_cursor(struct screen_state_t *screen, char direction, int n)
{
	switch (direction) {
		case 'u':
			if (screen->cur_y <= 1) break;
			CURSOR_UP_N(n);
			screen->cur_y -= n;
			break;
		case 'd':
			if (screen->cur_y >= screen->rows) break;
			CURSOR_DOWN_N(n);
			screen->cur_y += n;
			break;
		case 'l':
			if (screen->cur_x <= 1) break;
			CURSOR_LEFT_N(n);
			screen->cur_x -= n;
			break;
		case 'r':
			if (screen->cur_x >= screen->cols) break;
			CURSOR_RIGHT_N(n);
			screen->cur_x += n;
			break;
		default:
			return -1;
	}
	fflush(stdout);
	return 0;
}

/*
 * getch
 *
 * Get one character from stdin
 */
int
getch()
{
	char buf = 0;
	if (read(0, &buf, 1) < 0) {
		perror ("read()");
		return 0;
	}
	return buf;
}

void
draw_diag_line(struct screen_state_t *screen,
		int x, int y, int rise, int run, int length, char c)
{
	int i, j;

	/* position cursor at starting point */
	printf("\033[%d;%dH", x, y);
	
	for (i = 0; i < length; i++) {
		for(j = 0; j < rise; j++) {
			printf("%c", c);
			CURSOR_LEFT_N(1);
			CURSOR_UP_N(1);
		}
		CURSOR_RIGHT_N(1);
		for(j = 0; j < run; j++) {
			printf("%c", c);
		}
		CURSOR_UP_N(1);
	}

	printf("\033[%d;%dH", screen->cur_y, screen->cur_x);
	fflush(stdout);
}

void
draw_square(struct screen_state_t *screen, 
		int x, int y, int rows, int cols, char c)
{
	int i;

	/* position cursor at upper left corner */
	printf("\033[%d;%dH", x, y);
	fflush(stdout);

	/* draw top edge */
	for (i = 0; i < rows; i++)
		printf("%c", c);
	CURSOR_LEFT_N(1);

	/* draw right edge */
	for (i = 0; i < cols; i++) {
		CURSOR_DOWN_N(1);
		printf("%c", c);
		CURSOR_LEFT_N(1);
	}

	/* reposition at upper left corner */
	printf("\033[%d;%dH", x, y);
	fflush(stdout);

	/* draw left edge */
	for (i = 0; i < cols; i++) {
		CURSOR_DOWN_N(1);
		printf("%c", c);
		CURSOR_LEFT_N(1);
	}

	/*printf("\033[%d;%dH", x, y + rows);*/

	/* Draw bottom edge */
	for (i = 0; i < rows; i++)
		printf("%c", c);

	//printf("\033[%d;%dH", w.ws_row / 2, w.ws_col / 2);
	printf("\033[%d;%dH", screen->cur_y, screen->cur_x);
	fflush(stdout);
}

/*
 * fill_screen
 *
 * screen: screen state struct
 * c: character to fill the screen with
 *
 * Fill the screen with a given character. moves the cursor
 * back to the previous location after the writing is finished.
 */
void
fill_screen(struct screen_state_t *screen, char c)
{
	struct winsize w;
	int i, j;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	printf("\033[0;0H");
	fflush(stdout);

	for (i = 0; i < screen->rows; i++) {
		for (j = 0; j < screen->cols; j++) {
			printf("%c", c);
		}
		printf("\n");
	}

	//printf("\033[%d;%dH", w.ws_row / 2, w.ws_col / 2);
	printf("\033[%d;%dH", screen->cur_y, screen->cur_x);
	fflush(stdout);
}

int main (int argc, char **argv)
{
	int c;
	struct screen_state_t screen;
	/* Clear the entire screen intially */
	printf("\033[2J");
	set_term_environ();

	init_screen(&screen, 80, 40);
	//draw_screen(&screen);

	screen.cur_x = 40;
	screen.cur_y = 20;

	draw_screen(&screen);

	//fill_screen(&screen, '0');
	//draw_square(&screen, 1, 1, 80, 40, '0');

	//draw_diag_line(&screen, 40, 1, 1, 3, 10, '0');

	while (1) {
		c = getch();
		if (isspace(c))
			continue;
		switch (c) {
			case 'h':
				move_cursor(&screen, 'l', 1);
				break;
			case 'j':
				move_cursor(&screen, 'd', 1);
				break;
			case 'k':
				move_cursor(&screen, 'u', 1);
				break;
			case 'l':
				move_cursor(&screen, 'r', 1);
				break;
			default:
				break;
		}
		draw_square(&screen, 1, 1, 80, 40, (char)c);
		//fill_screen(&screen, (char)c);
	}

	cleanup();
	return 0;
}
