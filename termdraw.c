#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include <termios.h>

struct screen_state_t {
	int cur_x;
	int cur_y;
	int cols;
	int rows;
	char *buf;
};

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

int
move_cursor(struct screen_state_t *screen, char direction, int n)
{
	switch (direction) {
		case 'u':
			if (screen->cur_y <= 1) break;
			printf("\033[%dA", n);
			screen->cur_y -= n;
			break;
		case 'd':
			if (screen->cur_y >= screen->rows) break;
			printf("\033[%dB", n);
			screen->cur_y += n;
			break;
		case 'l':
			if (screen->cur_x <= 1) break;
			printf("\033[%dD", n);
			screen->cur_x -= n;
			break;
		case 'r':
			if (screen->cur_x >= screen->cols) break;
			printf("\033[%dC", n);
			screen->cur_x += n;
			break;
		default:
			return -1;
	}
	fflush(stdout);
	return 0;
}

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
	screen.cols = 80;
	screen.rows = 40;
	screen.cur_x = 40;
	screen.cur_y = 20;
	fill_screen(&screen, '0');

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
		fill_screen(&screen, (char)c);
	}

	cleanup();
	return 0;
}
