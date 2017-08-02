extern "C" {
	#include <xdo.h>
}
#include <stdio.h>
#include <cstring>
#include "fakeinput.h"

//note: compile with: gcc main.cpp linux.cpp -lxdo

namespace FakeInput {

xdo_t *xdoInstance;

void initFakeInput() {
	xdoInstance = xdo_new(0);
}

void freeFakeInput() {
	xdo_free(xdoInstance);
}

void typeChar(wchar_t c) {
    if(c == '\n') {
        keyTap("Return");
	}
	else if(c >= 0x00A0) { // 0x00A0 is the end of ASCII characters
		char unicodeCharNumber[5];
		sprintf(unicodeCharNumber, "U%04X", c);
		// Note: this starts glitching out if you set the pause too short.
		// Maybe because for unicode characters, the keys must be remapped.
		// 110ms seems to be sufficient.
		xdo_send_keysequence_window(xdoInstance, CURRENTWINDOW, unicodeCharNumber, 110000);
	}
	else {
		char str[] = {c, '\0'};
		xdo_enter_text_window(xdoInstance, CURRENTWINDOW, str, 12000);
	}
}

void typeString(wchar_t *string) {
	int i = 0;
	while(string[i] != '\0') {
		typeChar(string[i++]);
	}
}

#define EQ(K) ( strcmp(keyName, K) == 0 )
const char *getSpecialKey(char *keyName)
{
    if( EQ("Ctrl") )
        return "Control";
    else if( EQ("Esc") )
        return "Escape";
    else if( EQ("VolumeUp") )
        return "XF86AudioRaiseVolume";
    else if( EQ("VolumeDown") )
        return "XF86AudioLowerVolume";
    else
        return keyName;
}

void keyTap(char *key) {
    xdo_send_keysequence_window(xdoInstance, CURRENTWINDOW, getSpecialKey(key), 12000);
}

void keyDown(char *key) {
    xdo_send_keysequence_window_down(xdoInstance, CURRENTWINDOW, getSpecialKey(key), 12000);
}

void keyUp(char *key) {
    xdo_send_keysequence_window_up(xdoInstance, CURRENTWINDOW, getSpecialKey(key), 12000);
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

void mouseScroll(int amount) {
    int button = amount > 0 ? 5:4;
    int incr = amount > 0 ? -1:1;
    while(amount != 0) {
        mouseDown(button);
        mouseUp(button);
        amount += incr;
    }
}

}
