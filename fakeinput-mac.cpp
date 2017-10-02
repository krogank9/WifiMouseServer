#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <Carbon/Carbon.h>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QString>
#include <QMap>
#include <unistd.h>

#include "fakeinput.h"

io_connect_t auxKeyDriverRef;
bool driverLoaded = false;
void initAuxKeyDriver()
{
    mach_port_t masterPort, service, iter;

    if(IOMasterPort( bootstrap_port, &masterPort ) != KERN_SUCCESS)
        return;

    if(IOServiceGetMatchingServices(masterPort, IOServiceMatching( kIOHIDSystemClass), &iter ) != KERN_SUCCESS)
        return;

    service = IOIteratorNext( iter );
    if(service == 0) {
        IOObjectRelease(iter);
        return;
    }

    if(IOServiceOpen(service, mach_task_self(), kIOHIDParamConnectType, &auxKeyDriverRef ) == KERN_SUCCESS)
        driverLoaded = true;

    IOObjectRelease(service);
    IOObjectRelease(iter);
}

void tapAuxKey(const UInt8 auxKeyCode)
{
    if(!driverLoaded)
        return;

    NXEventData event;
    IOGPoint loc = { 0, 0 };

    // down
    UInt32 evtInfo = auxKeyCode << 16 | NX_KEYDOWN << 8;
    bzero(&event, sizeof(NXEventData));
    event.compound.subType = NX_SUBTYPE_AUX_CONTROL_BUTTONS;
    event.compound.misc.L[0] = evtInfo;
    if(IOHIDPostEvent( auxKeyDriverRef, NX_SYSDEFINED, loc, &event, kNXEventDataVersion, 0, FALSE ) != KERN_SUCCESS)
        return;

    // up
    evtInfo = auxKeyCode << 16 | NX_KEYUP << 8;
    bzero(&event, sizeof(NXEventData));
    event.compound.subType = NX_SUBTYPE_AUX_CONTROL_BUTTONS;
    event.compound.misc.L[0] = evtInfo;
    if(IOHIDPostEvent( auxKeyDriverRef, NX_SYSDEFINED, loc, &event, kNXEventDataVersion, 0, FALSE ) != KERN_SUCCESS)
        return;
}

// media & brightness keys
QMap<QString, UInt8> auxKeyList {
    {"VolumeUp", NX_KEYTYPE_SOUND_UP}, {"VolumeDown", NX_KEYTYPE_SOUND_DOWN}, {"VolumeMute", NX_KEYTYPE_MUTE},
    {"PauseSong", NX_KEYTYPE_PLAY}, {"NextSong", NX_KEYTYPE_NEXT}, {"PrevSong", NX_KEYTYPE_PREVIOUS},
    {"BrightnessUp", NX_KEYTYPE_BRIGHTNESS_UP}, {"BrightnessDown", NX_KEYTYPE_BRIGHTNESS_DOWN}
};

QMap<QString, CGKeyCode> virtualKeyList {
    {"Return", kVK_Return}, {"BackSpace", kVK_Delete}, {"Ctrl", kVK_Control},
    {"Cmd", kVK_Command}, {"Tab", kVK_Tab}, {"Opt", kVK_Option}, {"Esc", kVK_Escape},
    {"Shift", kVK_Shift}, {"Home", kVK_Home}, {"End", kVK_End},

    {"Left", kVK_LeftArrow}, {"Right", kVK_RightArrow}, {"Up", kVK_UpArrow}, {"Down", kVK_DownArrow},
    {"PgU", kVK_PageUp}, {"PgD", kVK_PageDown},

    {"F1", kVK_F1}, {"F2", kVK_F2}, {"F3", kVK_F3}, {"F4", kVK_F4}, {"F5", kVK_F5}, {"F6", kVK_F6},
    {"F7", kVK_F7}, {"F8", kVK_F8}, {"F9", kVK_F9}, {"F10", kVK_F10}, {"F11", kVK_F11}, {"F12", kVK_F12},

    {"0", kVK_ANSI_0}, {"1", kVK_ANSI_1}, {"2", kVK_ANSI_2}, {"3", kVK_ANSI_3},
    {"4", kVK_ANSI_4}, {"5", kVK_ANSI_5}, {"6", kVK_ANSI_6},
    {"7", kVK_ANSI_7}, {"8", kVK_ANSI_8}, {"9", kVK_ANSI_9},

    {"a", kVK_ANSI_A}, {"b", kVK_ANSI_B}, {"c", kVK_ANSI_C}, {"d", kVK_ANSI_D},
    {"e", kVK_ANSI_E}, {"f", kVK_ANSI_F}, {"g", kVK_ANSI_G}, {"h", kVK_ANSI_H},
    {"i", kVK_ANSI_I}, {"j", kVK_ANSI_J}, {"k", kVK_ANSI_K}, {"l", kVK_ANSI_L},
    {"m", kVK_ANSI_M}, {"n", kVK_ANSI_N}, {"o", kVK_ANSI_O}, {"p", kVK_ANSI_P},
    {"q", kVK_ANSI_Q}, {"r", kVK_ANSI_R}, {"s", kVK_ANSI_S}, {"t", kVK_ANSI_T},
    {"u", kVK_ANSI_U}, {"v", kVK_ANSI_V}, {"w", kVK_ANSI_W}, {"x", kVK_ANSI_X},
    {"y", kVK_ANSI_Y}, {"z", kVK_ANSI_Z}
};

namespace FakeInput {

QString getOsName() { return "mac"; }

void platformIndependentSleepMs(qint64 ms) {
    usleep((unsigned)ms);
}

void doMouseEvent(CGEventType type, int addX, int addY, CGMouseButton button, bool absolute = false) {
    CGEventRef getPos = CGEventCreate(NULL);
    CGPoint cursor = CGEventGetLocation(getPos);
    CFRelease(getPos);
    int curX = absolute? 0 : cursor.x;
    int curY = absolute? 0 : cursor.y;

    CGPoint absPos = CGPointMake(curX + addX, curY + addY);
    if(absPos.x < 0) absPos.x = 0;
    if(absPos.y < 0) absPos.y = 0;
    // Todo: support multiple monitors if this doesn't.
    // Will fix when i have a usable VM. fuck.
    CGRect mainMonitor = CGDisplayBounds(CGMainDisplayID());
    CGFloat monitorHeight = CGRectGetHeight(mainMonitor);
    CGFloat monitorWidth = CGRectGetWidth(mainMonitor);
    if(absPos.x > monitorWidth) absPos.x = monitorWidth;
    if(absPos.y > monitorHeight) absPos.y = monitorHeight;

    CGEventRef event = CGEventCreateMouseEvent(NULL, type, absPos, button);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
}

void doKeyboardEvent(CGKeyCode key, bool down) {
    CGEventRef event = CGEventCreateKeyboardEvent(NULL, key, down);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
}

void typeUniChar(ushort c, bool down = true, bool up = true) {
    CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    CGEventRef down_evt = CGEventCreateKeyboardEvent(src, (CGKeyCode) 0, true);
    CGEventRef up_evt = CGEventCreateKeyboardEvent(src, (CGKeyCode) 0, false);

    UniChar str[] = {(UniChar)c, '\0'};
    CGEventKeyboardSetUnicodeString(down_evt, 1, str);
    CGEventKeyboardSetUnicodeString(up_evt, 1, str);

    if(down)
        CGEventPost (kCGHIDEventTap, down_evt);
    if(up)
        CGEventPost (kCGHIDEventTap, up_evt);

    CFRelease (down_evt);
    CFRelease (up_evt);
    CFRelease (src);
}

// As on Windows, CGEventCreateKeyboardEvent() does not simulate
// the normal behavior in mac: keys repeat as you hold them down.
// Timer called every 35ms to simulate this behavior:
QString lastKeyDown;
qint64 lastKeyTime = 0;
bool lastKeyStillDown = false;
void keyRepeatCallback() {
    if(lastKeyStillDown && QDateTime::currentMSecsSinceEpoch() - lastKeyTime > 500)
       keyDown(lastKeyDown);
}

QTimer *keyRepeaterTimer;
void initFakeInput() {
    keyRepeaterTimer = new QTimer();
    keyRepeaterTimer->setSingleShot(false);
    keyRepeaterTimer->setInterval(35);
    keyRepeaterTimer->start();
    QObject::connect(keyRepeaterTimer, &QTimer::timeout, keyRepeatCallback);

    initAuxKeyDriver();
}

void freeFakeInput() {
    delete keyRepeaterTimer;
}

void typeChar(ushort c) {
    if(c == '\n')
        keyTap("Return");
    else
        typeUniChar(c);
}

void typeString(QString string) {
    for(int i=0; i<string.length(); i++) {
        typeChar(string.at(i).unicode());
    }
}

void keyTap(QString key) {
    keyDown(key);
    keyUp(key);
}

void keyDown(QString key) {
    lastKeyDown = key;
    // don't repeat modifiers
    if(key != "Opt" && key != "Cmd" && key != "Ctrl" && key != "Esc" && key != "Shift")
        lastKeyStillDown = true;
    lastKeyTime = QDateTime::currentMSecsSinceEpoch();

    if(virtualKeyList.contains(key))
        doKeyboardEvent(virtualKeyList.value(key), true);
    else if(key.length() == 1)
        typeUniChar(key.at(0).unicode(), true, false);
}

void keyUp(QString key) {
    lastKeyStillDown = false;

    // todo brightness & media keys in keyup
    if(virtualKeyList.contains(key))
        doKeyboardEvent(virtualKeyList.value(key), false);
    else if(auxKeyList.contains(key))
        tapAuxKey(auxKeyList.value(key));
    else if(key.length() == 1)
        typeUniChar(key.at(0).unicode(), false, true);
}

// need to keep track of buttons on mac for drag events.
bool buttonDown[] = {false, false, false};
void mouseMove(int addX, int addY) {
    CGEventType moveType = kCGEventMouseMoved;
    if(buttonDown[0]) moveType = kCGEventLeftMouseDragged;
    if(buttonDown[1]) moveType = kCGEventOtherMouseDragged;
    if(buttonDown[2]) moveType = kCGEventRightMouseDragged;
    doMouseEvent(moveType, addX, addY, (CGMouseButton)0, false);
}

void mouseSetPos(int x, int y) {
    CGEventType moveType = kCGEventMouseMoved;
    if(buttonDown[0]) moveType = kCGEventLeftMouseDragged;
    if(buttonDown[1]) moveType = kCGEventOtherMouseDragged;
    if(buttonDown[2]) moveType = kCGEventRightMouseDragged;
    doMouseEvent(moveType, x, y, (CGMouseButton)0, true);
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
    if(button == 1)
        return down? kCGEventLeftMouseDown:kCGEventLeftMouseUp;
    else if(button == 2)
        return down? kCGEventOtherMouseDown:kCGEventOtherMouseUp;
    else// button == 3
        return down? kCGEventRightMouseDown:kCGEventRightMouseUp;
}

void mouseDown(int button) {
    if(button >= 0 && button <= 3)
        buttonDown[button-1] = true;
    doMouseEvent(getMouseEventType(button, true), 0, 0, getCGButton(button));
}

void mouseUp(int button) {
    if(button > 0 && button <= 3)
        buttonDown[button-1] = false;
    doMouseEvent(getMouseEventType(button, false), 0, 0, getCGButton(button));
}

void mouseScroll(int amount) {
    CGEventRef scroll = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitLine, 1, -amount);
    CGEventPost(kCGHIDEventTap, scroll);
    CFRelease(scroll);
}

void zoom(int amount) {}
void stopZoom() {}

QString getApplicationNames() { return ""; }
void startApplicationByName(QString name) {}

QString getCommandSuggestions(QString command) { return ""; }
QString runCommandForResult(QString command) { return ""; }
QString getCpuUsage() { return ""; }
QString getRamUsage() { return ""; }
QString getProcesses() { return ""; }
void killProcess(QString pid) {}

void shutdown() {}
void restart() {}
void logout() {}
void sleep() {}
void lock_screen() {}
void blank_screen() {}

}
