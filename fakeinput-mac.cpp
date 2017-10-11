#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <Carbon/Carbon.h>
#include <mach/mach_init.h>
#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/vm_map.h>
#include <unistd.h>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QString>
#include <QMap>
#include <QProcess>

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
    {"BrightnessUp", NX_KEYTYPE_BRIGHTNESS_UP}, {"BrightnessDown", NX_KEYTYPE_BRIGHTNESS_DOWN},
    {"Eject", NX_KEYTYPE_EJECT}
};

QMap<QString, CGKeyCode> virtualKeyList {
    {"Return", kVK_Return}, {"BackSpace", kVK_Delete}, {"Ctrl", kVK_Control},
    {"Cmd", kVK_Command}, {"Tab", kVK_Tab}, {"Opt", kVK_Option}, {"Esc", kVK_Escape},
    {"Shift", kVK_Shift}, {"Home", kVK_Home}, {"End", kVK_End},

    {"=", kVK_ANSI_Equal}, {"-", kVK_ANSI_Minus},

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
    if(absPos.x < 0) {
        absPos.x = 0;
    }
    if(absPos.y < 0) {
        absPos.y = 0;
    }
    // Todo: support multiple monitors if this doesn't.
    // Will fix when i have a usable VM. fuck.
    CGRect mainMonitor = CGDisplayBounds(CGMainDisplayID());
    CGFloat monitorHeight = CGRectGetHeight(mainMonitor);
    CGFloat monitorWidth = CGRectGetWidth(mainMonitor);
    if(absPos.x > monitorWidth) {
        absPos.x = monitorWidth - 1;
    }
    if(absPos.y > monitorHeight) {
        absPos.y = monitorHeight - 1;
    }

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
    else if(virtualKeyList.contains(QString(QChar(c))))
        keyTap(QChar(c));
    else if(virtualKeyList.contains(QString(QChar(c).toLower()))) {
        keyDown("Shift");
        keyTap(QChar(c).toLower());
        keyUp("Shift");
    }
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
    if(button > 0 && button <= 3)
        buttonDown[button-1] = true;
    doMouseEvent(getMouseEventType(button, true), 0, 0, getCGButton(button));
    platformIndependentSleepMs(15); // sleep for good measure
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

bool zooming = false;

void stopZoom() {
    if(zooming) {
        keyUp("Cmd");
        zooming = false;
    }
}

void zoom(int amount) {
    if(!zooming) {
        zooming = true;
        keyDown("Cmd");
    }
    else
        mouseScroll(amount*-1);
}

QString runCommandForResult(QString command) {
    QProcess cmd;
    // trick to startDetached process and still read its output, use sh.
    // does not work with bash
    cmd.start("/bin/sh", QStringList()<< "-c" << command);
    cmd.waitForFinished(250);
    return cmd.readAllStandardOutput() + cmd.readAllStandardError();
}

QString getApplicationNames() {
    return runCommandForResult("find /Applications/. -maxdepth 3 -type d | grep '.app$' | rev | cut -d '/' -f1 | rev | cut -d '.' -f1");
}
void startApplicationByName(QString name) {
    runCommandForResult("open -a "+name);
}

//same as linux
QString getCommandSuggestions(QString command) {
    for(int i=0; i<command.length(); i++)
        if(command.at(i) == QString("\"") && (i == 0 || command.at(i - 1) != QString("\\")))
            command.replace(i, 1, "\\\""); // escape unescaped double quotes for grep

    QString history_results = runCommandForResult("grep \"^"+command+"\" ~/.bash_history");

    QProcess cmd; // compgen (list commands) only works in bash
    cmd.start("/bin/bash", QStringList()<< "-c" << ("compgen -c | grep \"^"+command+"\""));
    cmd.waitForFinished(250);
    QString compgen_results = cmd.readAllStandardOutput();

    QStringList list = compgen_results.split("\n") + history_results.split("\n");
    for(int i=0; i<list.count(); i++)
        list[i] = list[i].simplified(); // remove trailing whitespace
    list = list.toSet().toList(); // remove duplicates
    for(int i=0; i<list.count(); i++) { // sort by length
        for(int j=0; j<i; j++) {
            if(list[i].length() < list[j].length()) {
                list[i].swap(list[j]);
            }
        }
    }
    // Remove suggestions shorter than command
    QList<QString>::iterator i;
    for (i = list.begin(); i != list.end(); ++i) {
        if(command.length() >= (*i).length()) {
            list.erase(i);
        }
    }

    return list.join("\n");
}

unsigned long long _previousTotalTicks = 0;
unsigned long long _previousIdleTicks = 0;

float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks)
{
  unsigned long long totalTicksSinceLastTime = totalTicks-_previousTotalTicks;
  unsigned long long idleTicksSinceLastTime  = idleTicks-_previousIdleTicks;
  float ret = 1.0f-((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime)/totalTicksSinceLastTime : 0);
  _previousTotalTicks = totalTicks;
  _previousIdleTicks  = idleTicks;
  return ret;
}

// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
// You'll need to call this at regular intervals, since it measures the load between
// the previous call and the current one.
float GetCPULoad()
{
   host_cpu_load_info_data_t cpuinfo;
   mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
   if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) == KERN_SUCCESS)
   {
      unsigned long long totalTicks = 0;
      for(int i=0; i<CPU_STATE_MAX; i++) totalTicks += cpuinfo.cpu_ticks[i];
      return CalculateCPULoad(cpuinfo.cpu_ticks[CPU_STATE_IDLE], totalTicks);
   }
   else return -1.0f;
}

QString getCpuUsage() {
    float load = GetCPULoad();
    if(load >= 0.0f && load <= 1.0f)
        return QString::number( (int) (load*100) );
    else
        return "0";
}

static double ParseMemValue(const char * b)
{
   while((*b)&&(isdigit(*b) == false)) b++;
   return isdigit(*b) ? atof(b) : -1.0;
}

QString getRamUsage()
{
   FILE * fpIn = popen("/usr/bin/vm_stat", "r");
   if (fpIn)
   {
      double pagesUsed = 0.0, totalPages = 0.0;
      char buf[512];
      while(fgets(buf, sizeof(buf), fpIn) != NULL)
      {
         if (strncmp(buf, "Pages", 5) == 0)
         {
            double val = ParseMemValue(buf);
            if (val >= 0.0)
            {
               if ((strncmp(buf, "Pages wired", 11) == 0)||(strncmp(buf, "Pages active", 12) == 0)) pagesUsed += val;
               totalPages += val;
            }
         }
         else if (strncmp(buf, "Mach Virtual Memory Statistics", 30) != 0) break;  // Stop at "Translation Faults", we don't care about anything at or below that
      }
      pclose(fpIn);

      // get size in bytes
      pagesUsed *= getpagesize()/1024;
      totalPages *= getpagesize()/1024;
      // round to x.xx GB's
      pagesUsed /= 10000; pagesUsed = (int)pagesUsed; pagesUsed *= 10000;
      // round total to x.x GB's...
      totalPages /= 100000; totalPages = (int)totalPages; totalPages *= 100000;
      return QString::number((int) (pagesUsed) ) + " " + QString::number((int) (totalPages) );
   }
   return "";
}

QString getProcesses() {
    // pid / cpu / mem / name
    QStringList processes = runCommandForResult("ps ux | awk '{print $2,$3,$4,$11}' | tail -n +2 | grep -vE '/System|libexec|awk$|ps$|tail$|bash$|grep$|/bin/sh$|login$|Framework|Automator|distnoted$|cfprefsd$|usernoted$'").split("\n");
    QStringList duplicates;

    // split process name (last element) to last / to get simpler program name
    for(int i=0; i<processes.length(); i++) {
        QString process = processes[i];
        QStringList comp = process.split(" ");
        if(comp.length() < 4)
            continue;
        QString pid = comp.takeFirst();
        QString cpu = comp.takeFirst();
        QString mem = comp.takeFirst();
        QString name = comp.join(" ");
        name = name.remove(0, name.lastIndexOf("/")+1);
        if(duplicates.contains(name)) {
            processes.removeAt(i);
            i--;
            continue;
        }
        duplicates.append(name);
        processes[i] = pid + " " + cpu + " " + mem + " " + name;
    }

    return processes.join("\n");
}

void killProcess(QString pid) {
    runCommandForResult("kill -9 "+pid);
}

void shutdown() {
    runCommandForResult("osascript -e 'tell app \"System Events\" to shut down'");
}
void restart() {
    runCommandForResult("osascript -e 'tell app \"System Events\" to restart'");
}
void logout() {
    runCommandForResult("osascript -e 'tell app \"System Events\" to log out'");
}
void sleep() {
    runCommandForResult("osascript -e 'tell app \"System Events\" to sleep'");
}
void lock_screen() {
    runCommandForResult("/System/Library/CoreServices/Menu\ Extras/User.menu/Contents/Resources/CGSession -suspend");
}
void blank_screen() {
    runCommandForResult("pmset displaysleepnow");
}

}
