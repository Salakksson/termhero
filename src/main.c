#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>

#include "draw.h"
#include "game.h"

#define TARGET_FPS 200
#define TARGET_FRAME_TIME (1.0 / TARGET_FPS)

#define MAX_BYTES_PER_PIXEL 26 // estimated 21 but add 2 just to be sure
#define MAX_BYTES_PER_EOL 1
#define BYTES_PER_BEGIN_END (3 + 4 + 1)

void checkFramebuffer()
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		fprintf(stderr, "Framebuffer incomplete: 0x%x\n", status);
		exit(1);
	}
}

typedef struct
{
	int x, y;
} coord;

coord terminal_size()
{
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	return (coord){w.ws_col, w.ws_row * 2 - 4};
}

bool has_terminal_resized(coord* old_sz)
{
	coord current_sz = terminal_size();
	bool answer = (current_sz.x != old_sz->x || current_sz.y != old_sz->y);
	if (!answer) return false;

	*old_sz = current_sz;
	return true;
}

static struct termios original;

void enable_noncanonical()
{
	struct termios t;
	tcgetattr(STDIN_FILENO, &original);
	t = original;
	t.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void disable_noncanonical()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &original);
}

void bye()
{
	disable_noncanonical();
	printf("\x1b[?1049l");
	printf("\x1b[?25h");
}

void sigint_handler(int signum)
{
	bye();
	exit(0);
}

typedef struct
{
	GLuint fbo, tex;
	color* pixels;
	char* output;
} gl_buffers;

void create_buffers(gl_buffers* buffers, coord window_sz)
{
	if (buffers->tex)
		glDeleteTextures(1, &buffers->tex);
	if (buffers->fbo)
		glDeleteFramebuffers(1, &buffers->fbo);

	glGenFramebuffers(1, &buffers->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, buffers->fbo);

	glGenTextures(1, &buffers->tex);
	glBindTexture(GL_TEXTURE_2D, buffers->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_sz.x, window_sz.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffers->tex, 0);

	if (buffers->pixels) free(buffers->pixels);
	buffers->pixels = malloc(window_sz.x * window_sz.y * sizeof(color));

	if (buffers->output) free(buffers->output);
	buffers->output = malloc(window_sz.x * window_sz.y * MAX_BYTES_PER_PIXEL + window_sz.y * MAX_BYTES_PER_EOL + BYTES_PER_BEGIN_END);
}

int main()
{
	assert(sizeof(color) == 3);
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to init GLFW\n");
		return 1;
	}

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	GLFWwindow *window = glfwCreateWindow(1, 1, "Headless", NULL, NULL);
	if (!window)
	{
		fprintf(stderr, "Failed to create GLFW window\n");
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Failed to init GLEW\n");
		return 1;
	}
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	setvbuf(stdout, NULL, _IONBF, 0);
	printf("\x1b[?1049h");
	printf("\x1b[?25l");
	enable_noncanonical();
	atexit(bye);
	signal(SIGINT, sigint_handler);
	int quit = 0;
	double prev_frame = get_time();

	gl_buffers buffers = {0};
	coord window_sz = {0};
	init_game();
	while (!quit)
	{
		if (has_terminal_resized(&window_sz))
		{
			create_buffers(&buffers, window_sz);
		}
		checkFramebuffer();

		glViewport(0, 0, window_sz.x, window_sz.y);
		glClearColor(
			(float)0x19/0x100, (float)0x1b/0x100, (float)0x26/0x100, 1.0f // kitty fucks me over if its 1a1b26
		);
		glClear(GL_COLOR_BUFFER_BIT);

		/* draw_main(); */
		game_loop();

		glReadPixels(0, 0, window_sz.x, window_sz.y, GL_RGB, GL_UNSIGNED_BYTE, buffers.pixels);
		char* print_ptr = buffers.output;
		print_ptr += sprintf(print_ptr, "\x1b[H");

		color ppu = {0};
		color ppl = {0};
		for (int y = window_sz.y - 2; y >=0; y-= 2)
		{
			for (int x = 0; x < window_sz.x; x++)
			{
				int index_upper = x + window_sz.x * (y + 1);
				int index_lower = x + window_sz.x * (y + 0);
				color pu = buffers.pixels[index_upper];
				color pl = buffers.pixels[index_lower];
				if (!compare_color(ppu, pu))
					print_ptr += sprintf(print_ptr, "\x1b[38;2;%d;%d;%dm", pu.r, pu.g, pu.b);
				if (!compare_color(ppl, pl))
					print_ptr += sprintf(print_ptr, "\x1b[48;2;%d;%d;%dm", pl.r, pl.g, pl.b);
				print_ptr += sprintf(print_ptr, "▀");
			}
			print_ptr += sprintf(print_ptr, "\n");
		}
		print_ptr += sprintf(print_ptr, "\x1b[0m");
		fwrite(buffers.output, 1, print_ptr - buffers.output, stdout);
		double frame_time = get_time() - prev_frame;
		if (frame_time < TARGET_FRAME_TIME)
		{
			usleep(1e6 * (TARGET_FRAME_TIME - frame_time));
		}
		printf("fps: %.2f\n", 1 / (get_time() - prev_frame));
		printf("size: %ix%i", window_sz.x, window_sz.y);
		prev_frame = get_time();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
