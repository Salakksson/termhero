#include "draw.h"

#include <GL/gl.h>
#include <math.h>

double get_time()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

color hsv_to_rgb(unsigned char h, unsigned char s, unsigned char v)
{
	color rgb;

	float hf = (float)h * 360.0f / 255.0f;
	float sf = (float)s / 255.0f;
	float vf = (float)v / 255.0f;

	float c = vf * sf;
	float x = c * (1.0f - fabsf(fmodf(hf / 60.0f, 2) - 1.0f));
	float m = vf - c;

	float rf, gf, bf;

	if (hf < 60.0f)
	{
		rf = c; gf = x; bf = 0;
	}
	else if (hf < 120.0f)
	{
		rf = x; gf = c; bf = 0;
	}
	else if (hf < 180.0f)
	{
		rf = 0; gf = c; bf = x;
	}
	else if (hf < 240.0f)
	{
		rf = 0; gf = x; bf = c;
	}
	else if (hf < 300.0f)
	{
		rf = x; gf = 0; bf = c;
	}
	else
	{
		rf = c; gf = 0; bf = x;
	}

	rgb.r = (char)((rf + m) * 255.0f);
	rgb.g = (char)((gf + m) * 255.0f);
	rgb.b = (char)((bf + m) * 255.0f);

	return rgb;
}

void draw_main(void)
{
	glBegin(GL_TRIANGLES);

	char hue1 = (int)(255.0 * get_time() / 4) % 0x100;
	char hue2 = (hue1 + 0xff/3);
	char hue3 = (hue2 + 0xff/3);

	color color1 = hsv_to_rgb(hue1, 0xff, 0xff);
	color color2 = hsv_to_rgb(hue2, 0xff, 0xff);
	color color3 = hsv_to_rgb(hue3, 0xff, 0xff);

	glColor3f((float)color1.r/0xff, (float)color1.g/0xff, (float)color1.b/0xff);
	glVertex2f(-0.6f, -0.6f);

	glColor3f((float)color2.r/0xff, (float)color2.g/0xff, (float)color2.b/0xff);
	glVertex2f(0.6f, -0.6f);

	glColor3f((float)color3.r/0xff, (float)color3.g/0xff, (float)color3.b/0xff);
	glVertex2f(((float)color1.r + (float)color2.b - (float)0xff) /0x400, 0.6f);

	glEnd();

}
