#include "platform.h"
#include <stdio.h>


void Entry() {

	printf("Hello World!");

	sleep_for_ms(1000);

	if (get_key_state(KEY_MOUSE_LEFT) & 0b1) { sleep_for_ms(10222); }

	sleep_for_ms(1000);

	return;

}