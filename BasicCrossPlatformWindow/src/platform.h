#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>

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

bool is_window_active(struct window_state* state);

void draw_to_window(struct window_state* ws, unsigned int* buffer, int width, int height);

void* create_thread(void* address, void* args);

void close_window(struct window_state* state);

void join_thread(void* thread_handle);

struct point2d_int get_mouse_cursor_position(struct window_state* state);

struct window_state* create_window(int posx, int posy, int width, int height, unsigned char* name);

void Entry();

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN64)

#include <windows.h>

#define KEY_SPACE VK_SPACE
#define KEY_SHIFT VK_SHIFT
#define KEY_ARROW_LEFT VK_LEFT
#define KEY_ARROW_RIGHT VK_RIGHT
#define KEY_ARROW_UP VK_UP
#define KEY_ARROW_DOWN VK_DOWN
#define KEY_MOUSE_LEFT VK_LBUTTON
#define KEY_MOUSE_RIGHT VK_RBUTTON

struct window_info {
	HWND hwnd;
	BITMAPINFO bitmapInfo;
	HDC hdc;
	bool active;
	struct window_state state;
};

struct window_to_create {
	int posx;
	int posy;
	int width;
	int height;
	unsigned char* name;
	bool done_flag;
	struct window_state* return_state;
};

struct window_info** window_infos;

int window_infos_length = 0;
int max_window_infos = 256;

HINSTANCE HInstance;
WNDCLASS wc;

LARGE_INTEGER frequency;
LARGE_INTEGER startTime;

bool keyStates[256] = { 0 };

bool running = true;
bool window_infos_reorder = false;
bool msg_check = false;

struct window_to_create next_window;


void show_console_window() {
	HWND hwndConsole = GetConsoleWindow();
	if (hwndConsole != NULL) {
		ShowWindow(hwndConsole, SW_SHOW);
	}
}

void hide_console_window() {
	HWND hwndConsole = GetConsoleWindow();
	if (hwndConsole != NULL) {
		ShowWindow(hwndConsole, SW_HIDE);
	}
}

void sleep_for_ms(unsigned int _time_in_milliseconds) {
	Sleep(_time_in_milliseconds);
}

double get_time() {
	LARGE_INTEGER current_time;
	QueryPerformanceCounter(&current_time);
	return (double)(current_time.QuadPart - startTime.QuadPart) * 1000 / (double)frequency.QuadPart;
}

void* create_thread(void* address, void* args) {
	return CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)address, args, 0, NULL);
}

void join_thread(void* thread_handle) {
	WaitForSingleObject(thread_handle, INFINITE);
	CloseHandle(thread_handle);
}

void set_console_cursor_position(int x, int y) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(hConsole, (COORD) { (SHORT)x, (SHORT)y });
}

bool is_window_active(struct window_state* state) {
	return ((struct window_info*)state->window_handle)->active;
}

void draw_to_window(struct window_state* state, unsigned int* buffer, int width, int height) {
	if (is_window_active(state) == false) return;
	SetDIBitsToDevice(((struct window_info*)state->window_handle)->hdc, 0, 0, width, height, 0, 0, 0, height, buffer, &(((struct window_info*)state->window_handle)->bitmapInfo), DIB_RGB_COLORS);
}

struct point2d_int get_mouse_cursor_position(struct window_state* state) {
	POINT position;
	GetCursorPos(&position);
	RECT window_rect;
	GetWindowRect(((struct window_info*)state->window_handle)->hwnd, &window_rect);

	struct point2d_int pos = { position.x - window_rect.left - 7, window_rect.bottom - position.y - 9 };
	return pos;

}

char get_key_state(int key) {

	char keyState = 0;

	SHORT currentKeyState = GetKeyState(key);

	if (currentKeyState & 0x8000) keyState |= 0b0001;

	if ((currentKeyState & 0x8000 ? 0x1 : 0x0) != keyStates[key]) keyState |= 0b0010;

	if (currentKeyState & 0x01) keyState |= 0b0100;

	keyStates[key] = (currentKeyState & 0x8000 ? 0x1 : 0x0);

	return keyState;
}

struct window_state* create_window(int posx, int posy, int width, int height, unsigned char* name) {

	next_window = (struct window_to_create){
		posx,
		posy,
		width,
		height,
		name,
		false,
		NULL
	};

	while (next_window.done_flag == false) Sleep(1);

	return next_window.return_state;
}

void close_window(struct window_state* state) {
	if (is_window_active(state)) SendMessage(((struct window_info*)state->window_handle)->hwnd, WM_CLOSE, 0, 0);
	while (((struct window_info*)state->window_handle)->active) sleep_for_ms(1);

	window_infos_reorder = true;

	while (msg_check) Sleep(1);

	int index = 0;

	for (; index < window_infos_length && window_infos[index] != state->window_handle; index++);

	free(window_infos[index]);

	window_infos_length--;

	for (int i = index; i < window_infos_length; i++) {
		window_infos[i] = window_infos[i + 1];
	}

	window_infos_reorder = false;

}

void WindowControl() {
	while (running) {

		msg_check = true;

		while (window_infos_reorder) Sleep(1);

		for (int i = 0; i < window_infos_length; i++) {

			if (window_infos[i]->active) {
				MSG message;
				while (PeekMessageW(&message, window_infos[i]->hwnd, 0, 0, PM_REMOVE)) {
					TranslateMessage(&message);
					DispatchMessageW(&message);
				}
			}
		}

		msg_check = false;

		//creating window

		if (next_window.done_flag == false) {
			int name_length = 0;

			for (; next_window.name[name_length] != '\0'; name_length++);
			name_length++;

			unsigned short* name_short = calloc(name_length, sizeof(unsigned short));

			for (int i = 0; i < name_length; i++) *((char*)name_short + i * sizeof(unsigned short)) = next_window.name[i];

			if (window_infos_length == max_window_infos) {
				struct window_info** temp = window_infos;
				window_infos = malloc(sizeof(void*) * (max_window_infos + 256));
				for (int i = 0; i < max_window_infos; i++) window_infos[i] = temp[i];
				max_window_infos += 256;
				free(temp);
			}

			window_infos[window_infos_length] = (struct window_info*)malloc(sizeof(struct window_info));


			HWND window = CreateWindowExW(
				0,
				wc.lpszClassName,
				name_short,
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				next_window.posx,
				next_window.posy,
				next_window.width,
				next_window.height,
				NULL,
				NULL,
				HInstance,
				NULL
			);

			*window_infos[window_infos_length] = (struct window_info){
				window,
				{0},
				GetDC(window),
				true,
				(struct window_state) {
					window_infos[window_infos_length], next_window.width, next_window.height
				}
			};

			next_window.return_state = &(window_infos[window_infos_length]->state);

			window_infos_length++;

			next_window.done_flag = true;

			SendMessage(((struct window_info*)next_window.return_state->window_handle)->hwnd, WM_SIZE, 0, 0);

		}

		Sleep(10);
	}

	return;
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	for (int i = 0; i < window_infos_length; i++) {
		if (window_infos[i]->hwnd == hwnd) {
			LRESULT result = 0;
			switch (uMsg) {
			case WM_CLOSE:
				DestroyWindow(hwnd);
			case WM_DESTROY: {
				PostQuitMessage(0);
				window_infos[i]->active = false;

			} break;

			case WM_SIZE: {
				RECT rect;
				GetClientRect(hwnd, &rect);
				window_infos[i]->state.window_width = rect.right - rect.left;
				window_infos[i]->state.window_height = rect.bottom - rect.top;

				window_infos[i]->bitmapInfo.bmiHeader.biSize = sizeof(window_infos[i]->bitmapInfo);
				window_infos[i]->bitmapInfo.bmiHeader.biWidth = window_infos[i]->state.window_width;
				window_infos[i]->bitmapInfo.bmiHeader.biHeight = window_infos[i]->state.window_height;
				window_infos[i]->bitmapInfo.bmiHeader.biPlanes = 1;
				window_infos[i]->bitmapInfo.bmiHeader.biBitCount = 32;
				window_infos[i]->bitmapInfo.bmiHeader.biCompression = BI_RGB;
			} break;

			default:
				result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
			}

			return result;
		}
	}

	return DefWindowProcW(hwnd, uMsg, wParam, lParam);

}

void Entry_thread_function() {
	Entry();
	running = false;
}

int WINAPI WinMain(
	_In_	 HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_     LPSTR     lpCmdLine,
	_In_     int       nShowCmd
)

{
	(void)hInstance;
	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nShowCmd;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&startTime);

	HInstance = hInstance;

	wc = (WNDCLASS){
		CS_HREDRAW | CS_VREDRAW | CS_CLASSDC,
		WinProc,
		0,
		0,
		hInstance,
		NULL,
		LoadCursorW(NULL, IDC_ARROW),
		NULL,
		NULL,
		L"BasicWindowClass"
	};

	RegisterClassW(&wc);

	AllocConsole();

	FILE* fstdout;
	freopen_s(&fstdout, "CONOUT$", "w", stdout);
	FILE* fstderr;
	freopen_s(&fstderr, "CONOUT$", "w", stderr);
	FILE* fstdin;
	freopen_s(&fstdin, "CONIN$", "r", stdin);

	fflush(stdout);
	fflush(stderr);
	fflush(stdin);

	window_infos = (struct window_info**)malloc(sizeof(void*) * 256);

	next_window.done_flag = true;

	void* main_thread = create_thread(Entry_thread_function, NULL);

	WindowControl();

	join_thread(main_thread);

	return 0;
}

#elif defined(__linux__)

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define KEY_SPACE XK_space
#define KEY_SHIFT XK_Shift_L
#define KEY_ARROW_LEFT XK_Left
#define KEY_ARROW_RIGHT XK_Right
#define KEY_ARROW_UP XK_Up
#define KEY_ARROW_DOWN XK_Down
#define KEY_MOUSE_LEFT 0x1234
#define KEY_MOUSE_RIGHT 0x1236

//char get_key_state(int key);

struct window_info {
	Window window;
	bool active;
	struct window_state state;
};

struct window_info** window_infos;

int window_infos_length = 0;
int max_window_infos = 256;

Display* display;
int screen;
Atom wm_delete_window;

bool msg_check = false;
bool window_infos_reorder = false;
bool running;

bool keyStates[256] = { 0 };

void show_console_window() {
	return;
}

void hide_console_window() {
	return;
}

void sleep_for_ms(unsigned int _time_in_milliseconds) {
	usleep(_time_in_milliseconds * 1000);
}

double get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
}

void* create_thread(void* address, void* args) {
	pthread_t* thread = malloc(sizeof(pthread_t));
	pthread_create(thread, NULL, (void* (*)(void*))address, args);
	return thread;
}

void join_thread(void* thread_handle) {
	pthread_join(*(pthread_t*)thread_handle, NULL);
	free(thread_handle);
}

void set_console_cursor_position(int x, int y) {
	// X11 does not have direct console cursor manipulation; this is a placeholder
}

void draw_to_window(struct window_state* ws, unsigned int* buffer, int width, int height) {

	XImage* image = XCreateImage(display, DefaultVisual(display, screen), DefaultDepth(display, screen), ZPixmap, 0, (char*)buffer, width, height, 32, 0);
	XPutImage(display, ((struct window_info*) ws->window_handle)->window, DefaultGC(display, screen), image, 0, 0, 0, 0, width, height);
	XDestroyImage(image);
}

bool is_window_active(struct window_state* state) {
	return ((struct window_info*)state->window_handle)->active;
}

char get_key_state(int key) {
	char keys[32];
	XQueryKeymap(display, keys);

	KeySym keysym = (KeySym)key;
	KeyCode keycode = XKeysymToKeycode(display, keysym);

	int byteIndex = keycode / 8;
	int bitIndex = keycode % 8;

	return (keys[byteIndex] & (1 << bitIndex) ? 0b0001 : 0b0000);
}

struct point2d_int get_mouse_cursor_position(struct window_state* state) {
	Window root, child;
	int root_x, root_y;
	int win_x, win_y;
	unsigned int mask;

	XQueryPointer(display, ((struct window_info*)state->window_handle)->window, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);

	struct point2d_int pos = {win_x, state->window_height - win_y - 1}; 

	return pos;
}

struct window_state* create_window(int posx, int posy, int width, int height, unsigned char* name) {

	if (window_infos_length == max_window_infos) {
		struct window_info** temp = window_infos;
		window_infos = malloc(sizeof(void*) * (max_window_infos + 256));
		for (int i = 0; i < max_window_infos; i++) window_infos[i] = temp[i];
		max_window_infos += 256;
		free(temp);
	}

	window_infos[window_infos_length] = (struct window_info*)malloc(sizeof(struct window_info));

	Window window = XCreateSimpleWindow(display, RootWindow(display, screen), posx, posy, width, height, 1, BlackPixel(display, screen), WhitePixel(display, screen));

	*window_infos[window_infos_length] = (struct window_info) {
		window,
		true,
		(struct window_state) {
			window_infos[window_infos_length], width, height
		}
	};

	XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask);
	XStoreName(display, window, name);
	XSetWMProtocols(display, window, &wm_delete_window, 1);
	XMapWindow(display, window);
	window_infos_length++;

	return &(window_infos[window_infos_length - 1]->state);
}

void close_window(struct window_state* state) {
	if (is_window_active(state)) XDestroyWindow(display, ((struct window_info*) state->window_handle)->window);
	while (((struct window_info*)state->window_handle)->active) sleep_for_ms(1);

	window_infos_reorder = true;

	while (msg_check) usleep(10);

	int index = 0;

	for (; index < window_infos_length && window_infos[index] != state->window_handle; index++);

	free(window_infos[index]);

	window_infos_length--;

	for (int i = index; i < window_infos_length; i++) {
		window_infos[i] = window_infos[i + 1];
	}

	window_infos_reorder = false;
}

void WindowControl() {
	XEvent event;
	while (running) {

		while (XPending(display)) {

			msg_check = true;

			while (window_infos_reorder) usleep(10);

			XNextEvent(display, &event);
			
			int index = 0;

			for(; index < window_infos_length && window_infos[index]->window != event.xany.window; index++);
			
			switch (event.type) {
				case ConfigureNotify:
					window_infos[index]->state.window_width = event.xconfigure.width;
					window_infos[index]->state.window_height = event.xconfigure.height;
					break;
				case ClientMessage:
					if ((Atom)event.xclient.data.l[0] == wm_delete_window) {
						window_infos[index]->active = false;
						XDestroyWindow(display, window_infos[index]->window);
					}

			}

			msg_check = false;

		}

		sleep_for_ms(10);
	}

	return;
}

void Entry_thread_function() {
	Entry();
	running = false;
}

int main(int argc, char* argv[]) {
	XInitThreads();

	display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "Unable to open X display\n");
		return 1;
	}

	screen = DefaultScreen(display);

	wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);

	window_infos = (struct window_info**)malloc(sizeof(void*) * 256);

	running = true;

	void* mainthread = create_thread(Entry_thread_function, NULL);

	WindowControl();

	join_thread(mainthread);

	XCloseDisplay(display);

	return 0;
}

#endif 

#endif // PLATFORM_H
