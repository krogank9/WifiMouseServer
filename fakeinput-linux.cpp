#include <QString>
#include <QDebug>
extern "C" {
	#include <xdo.h>
}
#include "fakeinput.h"

//note: compile with: gcc main.cpp linux.cpp -lxdo

namespace FakeInput {

xdo_t *xdoInstance;

void initFakeInput() {
	xdoInstance = xdo_new(0);
}

void freeFakeInput() {
    stopZoom();
	xdo_free(xdoInstance);
}

void typeChar(ushort c) {
    if(c == '\n') {
        keyTap("Return");
	}
    else if(c >= 0x00A0) { // 0x00A0 is the end of ASCII characters
        QString unicodeHex;
        unicodeHex.setNum(c, 16);
        unicodeHex = "U"+unicodeHex;

        qInfo() << "unicodeHex: " << unicodeHex << "\n";

        // money = d83d dcb0
		// Note: this starts glitching out if you set the pause too short.
		// Maybe because for unicode characters, the keys must be remapped.
        // 95ms seems to be sufficient.
        xdo_send_keysequence_window(xdoInstance, CURRENTWINDOW, unicodeHex.toLocal8Bit().data(), 95000);
	}
    else {
		char str[] = {c, '\0'};
		xdo_enter_text_window(xdoInstance, CURRENTWINDOW, str, 12000);
    }
}

void typeString(QString string) {
	int i = 0;
    while(i < string.length()) {
        typeChar( string.at(i++).unicode() );
	}
}

QString getSpecialKey(QString keyName)
{
    if( keyName == "Ctrl" )
        return "Control";
    else if( keyName == "Esc" )
        return "Escape";
    else if( keyName == "VolumeUp" )
        return "XF86AudioRaiseVolume";
    else if( keyName == "VolumeDown" )
        return "XF86AudioLowerVolume";
    else
        return keyName;
}

void keyTap(QString key) {
    xdo_send_keysequence_window(xdoInstance, CURRENTWINDOW, getSpecialKey(key).toLocal8Bit(), 12000);
}

void keyDown(QString key) {
    xdo_send_keysequence_window_down(xdoInstance, CURRENTWINDOW, getSpecialKey(key).toLocal8Bit(), 12000);
}

void keyUp(QString key) {
    xdo_send_keysequence_window_up(xdoInstance, CURRENTWINDOW, getSpecialKey(key).toLocal8Bit(), 12000);
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

bool zooming = false;

void stopZoom() {
    if(!zooming)
        return;
    keyUp("Alt");
    zooming = false;
}

void zoom(int amount) {
    if(!zooming) {
        zooming = true;
        keyDown("Alt");
    }
    mouseScroll(amount*-1);
}

}
