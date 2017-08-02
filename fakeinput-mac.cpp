#include <ApplicationServices/ApplicationServices.h>
#include "fakeinput.h"

//note: compile with: gcc -framework ApplicationServices

void initFakeInput() {
}

void freeFakeInput() {
}

void typeUniChar(wchar_t c) {
	CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

	CGEventRef down_evt = CGEventCreateKeyboardEvent(src, (CGKeyCode) 0, true);
	CGEventRef up_evt = CGEventCreateKeyboardEvent(src, (CGKeyCode) 0, false);

	UniChar str[] = {(UniChar)c, '\0'};
	CGEventKeyboardSetUnicodeString(down_evt, 1, str);
	CGEventKeyboardSetUnicodeString(up_evt, 1, str);

	CGEventPost (kCGHIDEventTap, down_evt);
	usleep(60);
	CGEventPost (kCGHIDEventTap, up_evt);

	CFRelease (down_evt); CFRelease (up_evt);
	CFRelease (src);
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
