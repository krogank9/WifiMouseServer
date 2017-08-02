// Because the SendInput function is only supported in
// Windows 2000 and later, WINVER needs to be set as
// follows so that SendInput gets defined when windows.h
// is included below.
#define WINVER 0x0500
#include <windows.h>
#include <stdio.h>

#include "fakeinput.h"

void initFakeInput() {
}

void freeFakeInput() {
}

void typeChar(wchar_t c) {
	// Create a keyboard event structure
	INPUT ip;
	ip.type = INPUT_KEYBOARD;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;

	// Press a unicode "key"
	ip.ki.dwFlags = KEYEVENTF_UNICODE;
	ip.ki.wVk = 0;
	ip.ki.wScan = c;
	SendInput(1, &ip, sizeof(INPUT));

	// Release key
	ip.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
}

void typeString(wchar_t *string) {
	int i = 0;
	while(string[i] != '\0') {
		typeChar(string[i++]);
	}
}

void keyTap(char *key) {
	xdo_send_keysequence_window(xdoInstance, CURRENTWINDOW, string, 12000);
}

void keyDown(char *key) {
	xdo_send_keysequence_window_down(xdoInstance, CURRENTWINDOW, key, 12000);
}

void keyUp(char *key) {
	xdo_send_keysequence_window_up(xdoInstance, CURRENTWINDOW, key, 12000);
}

void mouseMove(int addX, int addY) {
	xdo_move_mouse_relative(xdoInstance, addX, addY);
}

void mouseDown(int button) {
	xdo_mouse_down(xdoInstance, CURRENTWINDOW, button);
}

void mouseUp(int button) {
	xdo_mouse_up(xdoInstance, CURRENTWINDOW, button);
}
