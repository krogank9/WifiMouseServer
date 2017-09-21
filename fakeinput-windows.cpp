// Because the SendInput function is only supported in
// Windows 2000 and later, WINVER needs to be set as
// follows so that SendInput gets defined when windows.h
// is included below.
#define WINVER 0x0500
#include <windows.h>
#ifdef _MSC_VER
 #pragma comment(lib, "user32.lib")
#endif

// Note: must compile with MSVC++ 2015

#include <QDateTime>
#include <QString>
#include <QTimer>
#include <QDebug>
#include <QProcess>

#include "fakeinput.h"

namespace FakeInput {

QString getOsName() { return "windows"; }

void platformIndependentSleepMs(qint64 ms) {
    Sleep(ms);
}

QString runCommandForResult(QString command) {
    QProcess cmd;
    // trick to startDetached process and still read its output, use sh.
    // does not work with bash
    cmd.start("start /B " + command);
    cmd.waitForFinished(250);
    return cmd.readAllStandardOutput() + cmd.readAllStandardError();
}

void sendUnicodeEvent(DWORD flags, WORD scan)
{
    INPUT ip;
    ZeroMemory(&ip, sizeof(ip));
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
    ZeroMemory(&ip, sizeof(ip));
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
    ZeroMemory(&input, sizeof(input));
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
    QObject::connect(keyRepeaterTimer, &QTimer::timeout, keyRepeatCallback);
}

void freeFakeInput() {
    delete keyRepeaterTimer;
}

void typeUnicodeChar(ushort c)
{
    sendUnicodeEvent(KEYEVENTF_UNICODE, c);
    sendUnicodeEvent(KEYEVENTF_UNICODE | KEYEVENTF_KEYUP, c);
}

void typeChar(QChar c) {
    if(c == '\n') {
        keyTap("Return");
    }
    else {
        QString hex;
        hex.setNum(c.unicode(), 16);
        qInfo() << "char: " << hex << "\n";
        typeUnicodeChar(c.unicode());
    }
}

void typeString(QString string) {
    int i = 0;
    while(i < string.length()) {
        typeChar(string[i++]);
    }
}

WORD getSpecialKey(QString keyName)
{
    if( keyName == "Return" )
        return VK_RETURN;
    else if( keyName == "BackSpace" )
        return VK_BACK;
    else if( keyName == "Ctrl" )
        return VK_CONTROL;
    else if( keyName == "Tab" )
        return VK_TAB;
    else if( keyName == "Alt" )
        return VK_MENU;
    else if( keyName == "Esc" )
        return VK_ESCAPE;
    else if( keyName == "VolumeUp" )
        return VK_VOLUME_UP;
    else if( keyName == "VolumeDown" )
        return VK_VOLUME_DOWN;
    else if( keyName == "Left" )
        return VK_LEFT;
    else if( keyName == "Right" )
        return VK_RIGHT;
    else if( keyName == "Up" )
        return VK_UP;
    else if( keyName == "Down" )
        return VK_DOWN;
    else
        return 0;
}

void keyTap(QString key) {
    keyDown(key);
    keyUp(key);
}

void keyDown(QString key) {
    lastKeyDown = getSpecialKey(key);
    lastKeyStillDown = true;
    lastKeyTime = QDateTime::currentMSecsSinceEpoch();
    sendSpecialKeyEvent(0, lastKeyDown);
}

void keyUp(QString key) {
    lastKeyStillDown = false;
    sendSpecialKeyEvent(KEYEVENTF_KEYUP, getSpecialKey(key));
}

void mouseSetPos(int x, int y) {
    // todo
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
    else {//if button == 3
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

bool zooming = false;
int totalZoom = 0;

void stopZoom() {
    if(!zooming)
        return;
    keyUp("Ctrl");
    zooming = false;
}

void zoom(int amount) {
    if(!zooming) {
        zooming = true;
        keyDown("Ctrl");
    }
    mouseScroll(-amount);
}

void shutdown() {
    runCommandForResult("shutdown -t 0");
}

void restart() {
    runCommandForResult("shutdown -t 0 -r -f");
}

void logout() {
    runCommandForResult("shutdown -t 0 -l");
}

void sleep() {
    runCommandForResult("psshutdown -t 0 -d");
}

void lock_screen() {
    runCommandForResult("rundll32.exe user32.dll,LockWorkStation");
}

void blank_screen() {
    SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
}

QString getCommandSuggestions(QString command)
{
    return "assoc\nat\nattrib\nbootcfg\ncd\nchkdsk\ncls\ncopy\ndel\ndir\ndiskpart\ndriverquery\necho\nexit\nfc\nfind\nfindstr\nfor\nfsutil\nftype\ngetmac\ngoto\nif\nipconfig\nmd\nmore\nmove\nnet\nnetsh\nnetstat\npath\npathping\npause\nping\npopd\npowercfg\nreg\nrmdir\nren\nsc\nschtasks\nset\nsfc\nshutdown\nsort\nstart\nsubst\nsysteminfo\ntaskkill\ntasklist\ntree\ntype\nvssadmin\nxcopy";
}

QString getApplicationNames() { return ""; }
void startApplicationByName(QString name) {}

QString getCpuUsage() {return "";}
QString getRamUsage() {return "";}
QString getProcesses() {return "";}

void killProcess(QString pid) {}

}
