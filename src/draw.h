#ifndef DRAW_H_
#define DRAW_H_

#include <string.h>

void draw_main(void);

typedef struct [[gnu::packed]]
{
	unsigned char r, g, b;
} color;

inline bool compare_color(color a, color b)
{
	return !memcmp(&a, &b, 3);
}

#include <time.h>

double get_time();

#endif
