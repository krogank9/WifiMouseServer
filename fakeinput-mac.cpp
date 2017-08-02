#include <ApplicationServices/ApplicationServices.h>
#include "fakeinput.h"

//note: compile with: gcc -framework ApplicationServices

void initFakeInput() {
}

void freeFakeInput() {
}

void doMouseEvent(CGEventType type, int addX, int addY, CGMouseButton button) {
    CGEventRef getPos = CGEventCreate(NULL);
    CGPoint cursor = CGEventGetLocation(getPos);
    CFRelease(getPos);
    int curX = cursor.x;
    int curY = cursor.y;

    CGPoint absPos = CGPointMake(curX + addX, curY + addY);

    CGEventRef event = CGEventCreateMouseEvent(NULL, type, absPos, button);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
}

void doKeyboardEvent(CGKeyCode key, bool down) {
    CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

    CGEventRef event= CGEventCreateKeyboardEvent(src, key, down);
    CGEventPost (kCGHIDEventTap, event);

    CFRelease (event);
    CFRelease (src);
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

    CFRelease (down_evt);
    CFRelease (up_evt);
    CFRelease (src);
}

void typeChar(wchar_t c) {
    if(c == '\n')
        keyTap("Return");
    else
        typeUniChar(c);
}

void typeString(wchar_t *string) {
	int i = 0;
	while(string[i] != '\0') {
		typeChar(string[i++]);
	}
}

#define EQ(K) ( strcmp(keyName, K) == 0 )
CGKeyCode getSpecialKey(char *keyName)
{
    if( EQ("Return") )
        return kVK_Return;
    else if( EQ("Ctrl") )
        return kVK_Control;
    else if( EQ("Esc") )
        return kVK_Escape;
    else if( EQ("VolumeUp") )
        return kVK_VolumeUp;
    else if( EQ("VolumeDown") )
        return kVK_VolumeDown;
    else
        return (CGKeyCode)0;
}

void keyTap(char *key) {
    keyDown(key);
    usleep(60);//i forget if this is needed on mac
    keyUp(key);
}

void keyDown(char *key) {
    doKeyboardEvent(getSpecialKey(key), true);
}

void keyUp(char *key) {
    doKeyboardEvent(getSpecialKey(key), true);
}

void mouseMove(int addX, int addY) {
    doMouseEvent(kCGEventMouseMoved, addX, addY, 0);
}

CGMouseButton getCGButton(int button) {
    if(button == 1)
        return kCGMouseButtonLeft;
    else if(button == 2)
        return kCGMouseButtonCenter;
    else//if(button == 3)
        return kCGMouseButtonRight;
}

CGEventType getMouseEventType(int button, bool down) {
    CGEventType cgType;
    if(button == 1)
        return down? kCGEventLeftMouseDown:kCGEventLeftMouseUp;
    else if(button == 2)
        return down? kCGEventOtherMouseDown:kCGEventOtherMouseUp;
    else//if(button == 3)
        return down? kCGEventRightMouseDown:kCGEventRightMouseUp;
}

void mouseDown(int button) {
    doMouseEvent(getMouseEventType(button, true), addX, addY, getCGButton(button));
}

void mouseUp(int button) {
    doMouseEvent(getMouseEventType(button, false), addX, addY, getCGButton(button));
}

void mouseScroll(int amount) {
    CGEventRef scroll = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitLine, 1, amount);
    CGEventPost(kCGHIDEventTap, scroll);
    CFRelease(scroll);
}
