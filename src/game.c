#include "game.h"

#include "draw.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

#include <GL/gl.h>
#include <math.h>

typedef enum
{
	KEYPRESS_NULL = 0,
	KEYPRESS_ASCII_END = 256,
} keypress;

int key_available()
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);

	struct timeval tv = {0};

	int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
	return ret > 0 && FD_ISSET(STDIN_FILENO, &readfds);
}

keypress getkey()
{
	if (!key_available()) return KEYPRESS_NULL;

	static char buf[1];
	int _read = read(STDIN_FILENO, buf, 1);
	if (!_read) return KEYPRESS_NULL;
	// add support for fancy shit
	return buf[0];
}

typedef struct
{
	int pitch;
	double timestamp;
} note;

struct
{
	note* notes;
	size_t note_count;
	size_t note_sz;
} game = {0};

keypress note_keybinds[8] = {
	's',
	'd',
	'f',
	'g',
	'h',
	'j',
	'k',
	'l',
};

color note_colors[8] = {
    {0xf7, 0x76, 0x8e}, {0x9e, 0xce, 0x6a}, {0xe0, 0xaf, 0x68},
    {0x7a, 0xa2, 0xf7}, {0xbb, 0x9a, 0xf7}, {0x7d, 0xcf, 0xff},
    {0xa9, 0xb1, 0xd6}, {0xc0, 0xca, 0xf5},
};

#define NOTE_RADIUS 0.06
#define NOB_RADIUS 0.03
#define NOB_COLOR (color){0x19, 0x1a, 0x26}
#define NOB_LOCATION -0.8f

void handle_key(keypress key, double time)
{
	if (key > KEYPRESS_ASCII_END) return;
	int pitch = -1;
	for (int i = 0; i < 8; i++)
	{
		keypress note_key = note_keybinds[i];
		if (note_key == key) pitch = i;
	}
	if (pitch == -1) return;

	for (int i = 0; i < game.note_count; i++)
	{
		note n = game.notes[i];
		if (n.pitch != pitch) continue;
		float distance = n.timestamp - time - NOB_LOCATION;
		if (distance > 0.1) continue;
		if (distance < -0.1) continue;

		game.notes[i].pitch = -1;
	}

}

void init_game(void)
{
	game.notes = malloc(20 * sizeof(*game.notes));
	game.note_count = 9;
	game.note_sz = 20;
	game.notes[0] = (note){0, 1.0};
	game.notes[1] = (note){1, 1.5};
	game.notes[2] = (note){2, 1.8};
	game.notes[3] = (note){3, 1.9};
	game.notes[4] = (note){4, 2.0};
	game.notes[5] = (note){5, 3.0};
	game.notes[6] = (note){6, 3.2};
	game.notes[7] = (note){7, 3.4};
	game.notes[8] = (note){6, 3.6};
}

typedef struct
{
	float x, y;
} vector2;

void draw_circle(float radius, vector2 loc, color c)
{
	glColor3ub(c.r, c.g, c.b);

	glBegin(GL_TRIANGLE_FAN);
	{
		glVertex2f(loc.x, loc.y);

		const int resolution = 12;
		for (int i = 0; i <= resolution; ++i)
		{
			float angle = ((float)i / resolution) * 2.0f * M_PI;
			float x = loc.x + cosf(angle) * radius;
			float y = loc.y + sinf(angle) * radius;

			glVertex2f(x, y);
		}
	}
	glEnd();
}

float get_note_x(int pitch)
{
	float pos_x = (float)pitch / 8.0 - 0.5;
	return pos_x;
}

void draw_note(note n, double time)
{
	if (n.pitch == -1) return;
	float pos_x = get_note_x(n.pitch);
	vector2 loc = {pos_x, n.timestamp - time};
	draw_circle(NOTE_RADIUS, loc, note_colors[n.pitch]);
}

void draw_game(float time)
{
	for (int i = 0; i < game.note_count; i++)
	{
		draw_note(game.notes[i], time);
	}
	for (int i = 0; i < 8; i++)
	{
		float pos_x = get_note_x(i);
		vector2 loc = {pos_x, NOB_LOCATION};
		draw_circle(NOTE_RADIUS, loc, note_colors[i]);
		draw_circle(NOB_RADIUS, loc, NOB_COLOR);
	}
}

void game_loop(void)
{
	keypress key;
	float time = get_time();

	while((key = getkey()))
	{
		handle_key(key, time);
	}
	draw_game(time);
}
