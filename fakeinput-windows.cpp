// Because the SendInput function is only supported in
// Windows 2000 and later, WINVER needs to be set as
// follows so that SendInput gets defined when windows.h
// is included below.
#define WINVER 0x0500
#include <Windows.h>
#ifdef _MSC_VER
 #pragma comment(lib, "user32.lib")
 #pragma comment(lib, "pdh.lib")
#endif
#include <Pdh.h>
#include "tchar.h"

// Note: must compile with MSVC++ 2015

#include <QDateTime>
#include <QString>
#include <QTimer>
#include <QDebug>
#include <QProcess>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QStandardPaths>
#include <QDir>
#include <QList>

#include "fakeinput.h"
#include "stdlib.h"

QMap<QString, WORD> virtualKeyList {
    {"Return", VK_RETURN}, {"BackSpace", VK_BACK}, {"Ctrl", VK_CONTROL},
    {"Win", VK_LWIN}, {"Tab", VK_TAB}, {"Alt", VK_MENU}, {"Esc", VK_ESCAPE},
    {"Shift", VK_SHIFT}, {"Menu", VK_RMENU}, {"Insert", VK_INSERT},
    {"Home", VK_HOME}, {"End", VK_END},

    {"VolumeUp", VK_VOLUME_UP}, {"VolumeDown", VK_VOLUME_DOWN}, {"VolumeMute", VK_VOLUME_MUTE},
    {"PauseSong", VK_MEDIA_PLAY_PAUSE}, {"NextSong", VK_MEDIA_NEXT_TRACK}, {"PrevSong", VK_MEDIA_PREV_TRACK},

    {"Left", VK_LEFT}, {"Right", VK_RIGHT}, {"Up", VK_UP}, {"Down", VK_DOWN},
    {"PgU", VK_PRIOR}, {"PgD", VK_NEXT},

    {"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3}, {"F4", VK_F4}, {"F5", VK_F5}, {"F6", VK_F6},
    {"F7", VK_F7}, {"F8", VK_F8}, {"F9", VK_F9}, {"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},

    {"0", 0x30}, {"1", 0x31}, {"2", 0x32}, {"3", 0x33}, {"4", 0x34}, {"5", 0x35},
    {"6", 0x36}, {"7", 0x37}, {"8", 0x38}, {"9", 0x39},

    {"a", 0x41}, {"b", 0x42}, {"c", 0x43}, {"d", 0x44}, {"e", 0x45}, {"f", 0x46},
    {"g", 0x47}, {"h", 0x48}, {"i", 0x49}, {"j", 0x4a}, {"k", 0x4b}, {"l", 0x4c},
    {"m", 0x4d}, {"n", 0x4e}, {"o", 0x4f}, {"p", 0x50}, {"q", 0x51}, {"r", 0x52},
    {"s", 0x53}, {"t", 0x54}, {"u", 0x55}, {"v", 0x56}, {"w", 0x57}, {"x", 0x58},
    {"y", 0x59}, {"z", 0x5a}
};

QList<QFileInfo> recurseAndGetShortcutList(QDir dir, int recursions = 0) {
    QList<QFileInfo> lnkList;
    QFileInfoList dirInfoList = dir.entryInfoList();
    for(int i=0; i<dirInfoList.length(); i++) {
        QFileInfo info = dirInfoList.at(i);
        if(info.isDir()) {
            QDir recurDir(info.absoluteFilePath());
            if(recurDir.absolutePath() != dir.absolutePath()
            && recurDir.absolutePath().startsWith(dir.absolutePath())
            // don't search nested start menu folders
            && !recurDir.absolutePath().contains(QRegExp("/Roaming/Microsoft/Windows/Start Menu/Programs/.*/.*/"))) {
                lnkList.append(recurseAndGetShortcutList(recurDir, recursions+1));
            }
        }
        else if(info.isFile()) {
            if(info.suffix() == "lnk"
            && !info.absoluteFilePath().contains("/Roaming/Microsoft/Windows/Recent/")
            && !info.absoluteFilePath().contains("/Roaming/Microsoft/Windows/SendTo/"))
                lnkList.append(info);
        }
    }
    return lnkList;
}

QString programsList = "";
QString getProgramsList() {
    QDir dir(QDir::home().absolutePath() + "/AppData/Roaming");
    QList<QFileInfo> exes = recurseAndGetShortcutList(dir);
    QString programsList;
    for(int i=0; i<exes.size(); i++) {
        if(i != 0)
            programsList += "\n";
        programsList += exes.at(i).absoluteFilePath();
    }
    return programsList;
}

namespace FakeInput {

QString getOsName() { return "windows"; }

void platformIndependentSleepMs(qint64 ms) {
    Sleep(ms);
}

QList<QString> nonBlockingCommands = {
    "echo", "netstat", "ipconfig", "dir", "chdir",
    "at", "fsutil", "ping", "reg", "set", "sort",
    "systeminfo", "taskkill"
};

QString runCommandForResult(QString command) {
    QProcess cmd;
    // trick to start process detached & read output not possible on windows
    // so only allow any we know that will not block for more than few hundred ms to return
    for(int i=0; i<nonBlockingCommands.size(); i++) {
        if(command.startsWith(nonBlockingCommands.at(i))) {
            cmd.start(command);
            cmd.waitForFinished(350);
            QString result = cmd.readAllStandardOutput() + cmd.readAllStandardError();
            qInfo() << result;
            return result;
        }
    }
    cmd.startDetached(command);
    return "";
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

void sendMouseEventAbsolute(DWORD flags, LONG x, LONG y, DWORD mouseData)
{
    INPUT input;
    ZeroMemory(&input, sizeof(input));
    input.type = INPUT_MOUSE;
    input.mi.mouseData = mouseData;
    input.mi.dx = x * (65536 / GetSystemMetrics(SM_CXSCREEN));
    input.mi.dy = y * (65536 / GetSystemMetrics(SM_CYSCREEN));
    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | flags;

    SendInput(1, &input, sizeof(input));
}

// SendInput() function doesn't repeat keys, here's a timer to simulate
// the normal behavior in windows: keys repeating as you hold them down.
QString lastKeyDown;
qint64 lastKeyTime = 0;
bool lastKeyStillDown = false;

void keyRepeatCallback() {
    if(lastKeyStillDown && QDateTime::currentMSecsSinceEpoch() - lastKeyTime > 500)
        keyDown(lastKeyDown);
}

static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;

QTimer *keyRepeaterTimer;
void initFakeInput() {
    keyRepeaterTimer = new QTimer();
    keyRepeaterTimer->setSingleShot(false);
    keyRepeaterTimer->setInterval(35);
    keyRepeaterTimer->start();
    QObject::connect(keyRepeaterTimer, &QTimer::timeout, keyRepeatCallback);

    PdhOpenQuery(NULL, NULL, &cpuQuery);
    // You can also use L"\\Processor(*)\\% Processor Time" and get individual CPU values with PdhGetFormattedCounterArray()
    PdhAddEnglishCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
    PdhCollectQueryData(cpuQuery);

    programsList = getProgramsList();
}

void freeFakeInput() {
    keyRepeaterTimer->stop();
    delete keyRepeaterTimer;
}

void typeUnicodeChar(ushort c)
{
    if(virtualKeyList.contains(QString(c))) {
        keyDown(QString(c)); // This allows for key combos. ctrl+a ctrl+l
        keyUp(QString(c)); // else it doesn't get registered when you use unicode scan key. need VK_ key
    }
    else if(virtualKeyList.contains(QString(c).toLower())) {
        keyDown("Shift");
        keyDown(QString(c).toLower()); // allows to use shiftkey on mobile keyboard for combos
        keyUp(QString(c).toLower()); // ex. ctrl + N(capital) will open incognito in chrome
        keyUp("Shift");
    }
    else {
        sendUnicodeEvent(KEYEVENTF_UNICODE, c);
        sendUnicodeEvent(KEYEVENTF_UNICODE | KEYEVENTF_KEYUP, c);
    }
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

void keyTap(QString key) {
    keyDown(key);
    keyUp(key);
}

void keyDown(QString key) {
    lastKeyDown = key;
    // don't repeat modifiers
    if(key != "Win" && key != "Alt" && key != "Ctrl" && key != "Esc" && key != "Shift")
        lastKeyStillDown = true;
    lastKeyTime = QDateTime::currentMSecsSinceEpoch();

    if(virtualKeyList.contains(key))
        sendSpecialKeyEvent(0, virtualKeyList.value(key));
    else if(key.length() == 1)
        sendUnicodeEvent(KEYEVENTF_UNICODE, key.at(0).unicode());
}

void changeBrightness(int change) {
    // this command takes a second to run. just save brightness after getting it once:
    static int savedBrightness = -1;
    if(savedBrightness == -1) {
        QString getCmd = "powershell.exe \"(Get-WmiObject -Namespace root\\wmi -Class WmiMonitorBrightness).CurrentBrightness\"";
        QProcess pShellForResult;
        pShellForResult.start(getCmd);
        pShellForResult.waitForFinished(5000);
        QString result = pShellForResult.readAllStandardOutput();
        savedBrightness = result.simplified().toInt();
    }
    savedBrightness += change;
    savedBrightness = savedBrightness > 100 ? 100 : (savedBrightness < 0 ? 0 : savedBrightness);
    QString setCmd = "powershell.exe \"(Get-WmiObject -Namespace root\\wmi -Class WmiMonitorBrightnessMethods).wmisetbrightness(0, "+QString::number(savedBrightness)+")\"";
    QProcess pShell;
    pShell.startDetached(setCmd);
}

void keyUp(QString key) {
    lastKeyStillDown = false;
    if(virtualKeyList.contains(key))
        sendSpecialKeyEvent(KEYEVENTF_KEYUP, virtualKeyList.value(key));
    else if(key == "BrightnessUp")
        changeBrightness(10);
    else if(key == "BrightnessDown")
        changeBrightness(-10);
    else if(key.length() > 0)
        sendUnicodeEvent(KEYEVENTF_UNICODE | KEYEVENTF_KEYUP, key.at(0).unicode());
}

void mouseSetPos(int x, int y) {
    sendMouseEventAbsolute(MOUSEEVENTF_MOVE, x, y, 0);
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
    LockWorkStation();
}

void blank_screen() {
    SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
}

QString getCommandSuggestions(QString command)
{
    return "assoc\nat\nattrib\nbootcfg\ncd\nchkdsk\ncls\ncopy\ndel\ndir\ndiskpart\ndriverquery\necho\nexit\nfc\nfind\nfindstr\nfor\nfsutil\nftype\ngetmac\ngoto\nif\nipconfig\nmd\nmore\nmove\nnet\nnetsh\nnetstat\npath\npathping\npause\nping\npopd\npowercfg\nreg\nrmdir\nren\nsc\nschtasks\nset\nsfc\nshutdown\nsort\nstart\nsubst\nsysteminfo\ntaskkill\ntasklist\ntree\ntype\nvssadmin\nxcopy";
}

QString getApplicationNames() {
    QStringList sorted = programsList.split("\n");
    // Remove file extensions & prefix
    for(int i=0; i<sorted.size(); i++) {
        QString cur = sorted.at(i);
        cur = cur.mid(cur.lastIndexOf("/")+1);
        cur = cur.mid(0, cur.indexOf("."));
        sorted[i] = cur;
    }
    // Remove uninstall program shortcuts
    for(int i=0; i<sorted.size(); i++)
        if(sorted.at(i).startsWith("Uninstall "))
            sorted.removeAt(i--);
    // Sort alphabetically
    for(int i=0; i<sorted.size()-1; i++)
        for(int j=i+1; j<sorted.size(); j++)
            if(sorted.at(i) > sorted.at(j))
                sorted.swap(i, j);
    return sorted.join("\n");
}
void startApplicationByName(QString name) {
    QStringList apps = programsList.split("\n");
    for(int i=0; i<apps.size(); i++) {
        QString appName = apps.at(i);
        if(appName.endsWith(name+".lnk")) {
            std::wstring wstr = appName.toStdWString();
            ShellExecute(NULL, NULL, wstr.data(), NULL, NULL, SW_NORMAL);
            break;
        }
    }
}

QString getCpuUsage() {
    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData(cpuQuery);
    PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
    return QString::number((int)counterVal.doubleValue);
}

unsigned long lastTotalRamKbs = 8*1000*1000;
QString getRamUsage() {
    MEMORYSTATUSEX statex;
    ZeroMemory(&statex, sizeof(statex));
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    unsigned long total = statex.ullTotalPhys;
    unsigned long available = statex.ullAvailPhys;
    unsigned long used = total - available;
    lastTotalRamKbs = total > 0 ? total/1000 : 8000000;
    return QString::number(used/1000) + " " + QString::number(total/1000);
}

QString getProcessesFromPowershell() {
    QProcess pShell;
    // pid / cpu / mem / name
    pShell.start("powershell.exe \"$perflist = (get-wmiobject Win32_PerfFormattedData_PerfProc_Process); foreach ($p in $perflist) {'' + $p.IDProcess + ' ' + $p.PercentProcessorTime + ' ' + $p.WorkingSet + ' ' + $p.name}\"");
    pShell.waitForFinished();
    return pShell.readAllStandardOutput();
}

QFuture<QString> pFuture;
QString lastFutureResult;
qint64 lastFutureResultTime = 0;

QString getProcesses() {
    if(pFuture.isFinished() && QDateTime::currentMSecsSinceEpoch() - lastFutureResultTime > 3000) {
        lastFutureResultTime = QDateTime::currentMSecsSinceEpoch();
        if(pFuture.resultCount() > 0)
            lastFutureResult = pFuture.result();
        pFuture = QtConcurrent::run(getProcessesFromPowershell);
    }

    // pid / cpu / mem / name
    QString perfList = lastFutureResult;

    QStringList toList = perfList.split("\n");
    for(int i=0; i<toList.size(); i++) {
        QStringList info = toList[i].simplified().split(" ");
        if(info.size() < 2)
            continue;
        long pMemoryKbs = info[2].toULong()/1000;
        float pctMem = ((float)pMemoryKbs)/((float)lastTotalRamKbs);
        int pctMemInt = (pctMem*100.0f);
        info[2] = QString::number(pctMemInt);
        toList[i] = info.join(" ");
    }
    // filter junk processes
    for(int i=0; i<toList.size(); i++)
        if(toList[i].endsWith("_Total") || toList[i].endsWith("Idle")
          || toList[i].contains("WifiMouseServer")
          || toList[i].contains("svchost") || toList[i].contains("powershell")
          || toList[i].contains("conhost") || toList[i].endsWith("Memory Compression"))
            toList.removeAt(i--);
    return toList.join("\n");
}
void killProcess(QString pid) {
    QProcess killer;
    killer.startDetached("Taskkill /PID "+pid.simplified()+" /F");
}

}
