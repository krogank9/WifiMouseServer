// Because the SendInput function is only supported in
// Windows 2000 and later, WINVER needs to be set as
// follows so that SendInput gets defined when windows.h
// is included below.
#define WINVER 0x0500
#include <windows.h>

#include <QDateTime>
#include <QTimer>

#include "fakeinput.h"

namespace FakeInput {

void sendUnicodeEvent(DWORD flags, WORD scan)
{
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.time = 0;
    ip.ki.dwFlags = flags;
    ip.ki.wScan = scan;
    ip.ki.wVk = 0;

    ip.ki.dwExtraInfo = 0;
    SendInput(1, &ip, sizeof(INPUT));
}

void sendSpecialKeyEvent(DWORD flags, WORD key)
{
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.time = 0;
    ip.ki.dwFlags = flags;
    ip.ki.wScan = MapVirtualKey(key, MAPVK_VK_TO_VSC);
    ip.ki.wVk = key;

    ip.ki.dwExtraInfo = 0;
    SendInput(1, &ip, sizeof(INPUT));
}

void sendMouseEvent(DWORD flags, LONG dx, LONG dy, DWORD mouseData)
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.mouseData = mouseData;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = flags;

    SendInput(1, &input, sizeof(input));
}

// SendInput() function doesn't repeat keys, here's a timer to simulate
// the normal behavior in windows: keys repeating as you hold them down.
WORD lastKeyDown = -1;
qint64 lastKeyTime = 0;
bool lastKeyStillDown = false;

void keyRepeatCallback() {
    if(lastKeyStillDown && QDateTime::currentMSecsSinceEpoch() - lastKeyTime > 500)
       sendSpecialKeyEvent(0, lastKeyDown);
}

QTimer *keyRepeaterTimer;
void initFakeInput() {
    keyRepeaterTimer = new QTimer();
    keyRepeaterTimer->setSingleShot(false);
    keyRepeaterTimer->setInterval(35);
    keyRepeaterTimer->start();
    QObject::connect(keyRepeaterTimer, QTimer::timeout, keyRepeatCallback);
}

void freeFakeInput() {
    delete keyRepeaterTimer;
}

void typeUnicodeChar(wchar_t c)
{
    sendUnicodeEvent(KEYEVENTF_UNICODE, c);
    sendUnicodeEvent(KEYEVENTF_UNICODE | KEYEVENTF_KEYUP, c);
}

void typeChar(wchar_t c) {
    if(c == '\n') {
        keyTap("Return");
    }
    else {
        typeUnicodeChar(c);
    }
}

void typeString(wchar_t *string) {
    int i = 0;
    while(string[i] != '\0') {
        typeChar(string[i++]);
    }
}

#define EQ(K) ( strcmp(keyName, K) == 0 )
WORD getSpecialKey(char *keyName)
{
    if( EQ("Return") )
        return VK_RETURN;
    else if( EQ("BackSpace") )
        return VK_BACK;
    else if( EQ("Ctrl") )
        return VK_CONTROL;
    else if( EQ("Tab") )
        return VK_TAB;
    else if( EQ("Alt") )
        return VK_MENU;
    else if( EQ("Esc") )
        return VK_ESCAPE;
    else if( EQ("VolumeUp") )
        return VK_VOLUME_UP;
    else if( EQ("VolumeDown") )
        return VK_VOLUME_DOWN;
    else if( EQ("Left") )
        return VK_LEFT;
    else if( EQ("Right") )
        return VK_RIGHT;
    else if( EQ("Up") )
        return VK_UP;
    else if( EQ("Down") )
        return VK_DOWN;
    else
        return 0;
}

void keyTap(char *key) {
    keyDown(key);
    keyUp(key);
}

void keyDown(char *key) {
    lastKeyDown = getSpecialKey(key);
    lastKeyStillDown = true;
    lastKeyTime = QDateTime::currentMSecsSinceEpoch();
    sendSpecialKeyEvent(0, lastKeyDown);
}

void keyUp(char *key) {
    lastKeyStillDown = false;
    sendSpecialKeyEvent(KEYEVENTF_KEYUP, getSpecialKey(key));
}

void mouseMove(int addX, int addY) {
    sendMouseEvent(MOUSEEVENTF_MOVE, addX, addY, 0);
}

DWORD buttonToFlags(int button, bool down) {
    if(button == 1) {
        if(down)
            return MOUSEEVENTF_LEFTDOWN;
        else
            return MOUSEEVENTF_LEFTUP;
    }
    else if(button == 2) {
        if(down)
            return MOUSEEVENTF_MIDDLEDOWN;
        else
            return MOUSEEVENTF_MIDDLEUP;
    }
    else {//if(button == 3) {
        if(down)
            return MOUSEEVENTF_RIGHTDOWN;
        else
            return MOUSEEVENTF_RIGHTUP;
    }
}

void mouseDown(int button) {
    sendMouseEvent(buttonToFlags(button, true), 0, 0, 0);
}

void mouseUp(int button) {
    sendMouseEvent(buttonToFlags(button, false), 0, 0, 0);
}

void mouseScroll(int amount) {
    sendMouseEvent(MOUSEEVENTF_WHEEL, 0, 0, amount*-100);
}


}
