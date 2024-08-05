#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN64
	#include "platform_win64.h"
#else
	#include "platform_linux.h"
#endif // _WIN64

struct window_state {
	void* window_handle;
	int window_width;
	int window_height;
};

struct point2d_int {
	int x;
	int y;
};

void show_console_window();

void hide_console_window();

void sleep_for_ms(unsigned int time_in_milliseconds);

void set_console_cursor_position(int x, int y);

double get_time();

char get_key_state(int key);

void draw_to_window(struct window_state* ws, unsigned int* buffer, int width, int height);

void* create_thread(void* address, void* args);

void close_window(struct window_state* state);

void join_thread(void* thread_handle);

struct point2d_int get_mouse_cursor_position(struct window_state* state);

struct window_state* create_window(int posx, int posy, int width, int height, unsigned char* name);

void Entry();

#endif // PLATFORM_H
