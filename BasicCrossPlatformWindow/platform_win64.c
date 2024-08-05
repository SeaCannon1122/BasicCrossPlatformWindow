#ifdef _WIN64

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>

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



#endif // _WIN64