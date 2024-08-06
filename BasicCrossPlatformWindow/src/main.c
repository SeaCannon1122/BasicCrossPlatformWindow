#include "platform.h"
#include <stdio.h>


void Entry() {

	int width = 500;
	int height = 300;

	struct window_state* w2 = create_window(300, 200, 700, 700, "NAME");
	struct window_state* w1 = create_window(100, 100, width, height, "NAME");

	unsigned int* pixels = malloc(sizeof(unsigned int) * w1->window_height * w1->window_width);

	bool console_shown = true;

	while (get_key_state('C') == 0) {

		if (width != w1->window_width || height != w1->window_height) {
			height = w1->window_height;
			width = w1->window_width;
			free(pixels);
			pixels = malloc(sizeof(unsigned int) * height * width);
			for (int i = 0; i < height * width; i++) pixels[i] = 0x12345;
		}

		struct point2d_int p = get_mouse_cursor_position(w1);

		if (p.x >= 0 && p.x < width && p.y >= 0 && p.y < height && get_key_state(KEY_MOUSE_LEFT) & 0b1) pixels[p.x + width * p.y] = 0xff0000;

		draw_to_window(w1, pixels, width, height);

		if ((get_key_state('S') & 0b11) == 0b11) {
			if (console_shown) {
				console_shown = false;
				hide_console_window();
			}

			else {
				console_shown = true;
				show_console_window();
			}
		}
		sleep_for_ms(10);


	}

	close_window(w1);
	close_window(w2);

	return;

}