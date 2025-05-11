#ifndef DRAW_H_
#define DRAW_H_

void draw_main(void);

typedef struct [[gnu::packed]]
{
	unsigned char r, g, b;
} color;

#include <time.h>

double get_time();

#endif
