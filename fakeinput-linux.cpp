#include <QString>
#include <QSet>
#include <QStringList>
#include <QProcess>
#include <QDateTime>
#include <QDebug>
#include <QMap>
extern "C" {
    #include <X11/Xlib.h>
    #include <X11/extensions/XTest.h>
    #include <X11/keysym.h>
    #include <X11/XF86keysym.h>
    #include <ctime>
}
#include "fakeinput.h"
#include "fakeinput-linux-keysyms-map.h"

namespace FakeInput {

QString desktopSession = "";

QString getOsName() { return "linux"; }

void platformIndependentSleepMs(qint64 ms) {
    const int MS_TO_NANO_MULTIPLIER = 1000000;
    struct timespec tim;
    tim.tv_sec = ms/1000;
    tim.tv_nsec = ((unsigned)ms)*MS_TO_NANO_MULTIPLIER;
    nanosleep(&tim, NULL);
}

QString runCommandForResult(QString command) {
    QProcess cmd;
    // trick to startDetached process and still read its output, use sh.
    // does not work with bash
    cmd.start("/bin/sh", QStringList()<< "-c" << command);
    cmd.waitForFinished(250);
    return cmd.readAllStandardOutput() + cmd.readAllStandardError();
}

Display *dpy;
QString backupApplicationsList;

void initFakeInput() {
    // Sometimes grepping app list can take too long. get apps at start & wait up to 5 seconds to ensure we always have apps to display
    QProcess cmd;
    cmd.start("/bin/sh", QStringList()<< "-c" << "grep -LEs 'NoDisplay|OnlyShowIn' /usr/share/applications/* | grep '.*.desktop' | xargs grep -hm 1 '^Name=' | sed 's/^.....//'");
    cmd.waitForFinished(5000);
    backupApplicationsList = cmd.readAllStandardOutput();

    desktopSession = runCommandForResult("echo $DESKTOP_SESSION").simplified();
    qInfo() << "desktopSession" << desktopSession;

    dpy = XOpenDisplay(NULL);
}

void freeFakeInput() {
}

void typeString(QString string) {
    // Note: this starts glitching out if you set the pause too short.
    // Maybe because for unicode characters, the keys must be remapped.
    // 95ms seems to be sufficient.
    //xdo_send_keysequence_window(xdoInstance, CURRENTWINDOW, unicodeHex.toLocal8Bit().data(), 95000);
    int i = 0;
    while(i < string.length()) {
        QChar c = string.at(i++);
        if(c == '\n')
            keyTap("Return");
        else
            keyTap(c);
    }
}

QMap<QString, unsigned int> xorgKeyCodes {
    {"Return", XK_Return}, {"BackSpace", XK_BackSpace}, {"Ctrl", XK_Control_L},
    {"Win", XK_Super_L}, {"Tab", XK_Tab}, {"Alt", XK_Alt_L}, {"Esc", XK_Escape},
    {"Shift", XK_Shift_L}, {"Menu", XK_Menu}, {"Insert", XK_Insert},
    {"Home", XK_Home}, {"End", XK_End}, {"Space", XK_space}, {" ", XK_space},

    {"VolumeUp", XF86XK_AudioRaiseVolume}, {"VolumeDown", XF86XK_AudioLowerVolume}, {"VolumeMute", XF86XK_AudioMute},
    {"PauseSong", XF86XK_AudioPause}, {"NextSong", XF86XK_AudioNext}, {"PrevSong", XF86XK_AudioPrev},

    {"BrightnessUp", XF86XK_KbdBrightnessUp}, {"BrightnessDown", XF86XK_KbdBrightnessDown},

    {"Left", XK_Left}, {"Right", XK_Right}, {"Up", XK_Up}, {"Down", XK_Down},
    {"PgU", XK_Page_Up}, {"PgD", XK_Page_Down},

    {"F1", XK_F1}, {"F2", XK_F2}, {"F3", XK_F3}, {"F4", XK_F4}, {"F5", XK_F5}, {"F6", XK_F6},
    {"F7", XK_F7}, {"F8", XK_F8}, {"F9", XK_F9}, {"F10", XK_F10}, {"F11", XK_F11}, {"F12", XK_F12},

    {"0", XK_0}, {"1", XK_1}, {"2", XK_2}, {"3", XK_3}, {"4", XK_4}, {"5", XK_5},
    {"6", XK_6}, {"7", XK_7}, {"8", XK_8}, {"9", XK_9},

    {"A", XK_A}, {"B", XK_B}, {"C", XK_C}, {"D", XK_D}, {"E", XK_E}, {"F", XK_F},
    {"G", XK_G}, {"H", XK_H}, {"I", XK_I}, {"J", XK_J}, {"K", XK_K}, {"L", XK_L},
    {"M", XK_M}, {"N", XK_N}, {"O", XK_O}, {"P", XK_P}, {"Q", XK_Q}, {"R", XK_R},
    {"S", XK_S}, {"T", XK_T}, {"U", XK_U}, {"V", XK_V}, {"W", XK_W}, {"X", XK_X},
    {"Y", XK_Y}, {"Z", XK_Z},

    {"a", XK_a}, {"b", XK_b}, {"c", XK_c}, {"d", XK_d}, {"e", XK_e}, {"f", XK_f},
    {"g", XK_g}, {"h", XK_h}, {"i", XK_i}, {"j", XK_j}, {"k", XK_k}, {"l", XK_l},
    {"m", XK_m}, {"n", XK_n}, {"o", XK_o}, {"p", XK_p}, {"q", XK_q}, {"r", XK_r},
    {"s", XK_s}, {"t", XK_t}, {"u", XK_u}, {"v", XK_v}, {"w", XK_w}, {"x", XK_x},
    {"y", XK_y}, {"z", XK_z},

    {"\\", XK_backslash}, {"'", XK_apostrophe}, {",", XK_comma}, {"-", XK_minus},
    {".", XK_period}, {"/", XK_slash}, {";", XK_semicolon}, {"=", XK_equal},
    {"[", XK_bracketleft}, {"]", XK_bracketright}, {"`", XK_grave},
    {"<", XK_less},
};

QMap<QString, unsigned int> xorgUpKeyCodes {
    {"!", XK_exclam}, {"\"", XK_quotedbl}, {"#", XK_numbersign}, {"$", XK_dollar},
    {"%", XK_percent}, {"&", XK_ampersand}, {"(", XK_parenleft},
    {")", XK_parenright}, {"*", XK_asterisk}, {"+", XK_plus},
    {":", XK_colon}, {">", XK_greater}, {"~", XK_asciitilde},
    {"?", XK_question}, {"@", XK_at}, {"^", XK_asciicircum}, {"_", XK_underscore},
    {"|", XK_bar}, {"{", XK_braceleft}, {"}", XK_braceright},
};

unsigned int getUnicodeKeycode(QString key) {
    if(xorgKeyCodes.contains(key))
        return XKeysymToKeycode(dpy, xorgKeyCodes.value(key));
    else if(xorgUpKeyCodes.contains(key))
        return XKeysymToKeycode(dpy, xorgUpKeyCodes.value(key));
    else {
        KeySym keysyms[] = { unicodeKeySyms.value(key, 0) };
        XChangeKeyboardMapping(dpy, 999, 1, keysyms, 1);
        return 999;
    }
}

void keyTap(QString key) {
    if( key.length() == 1 && (key.toLower() != key || xorgUpKeyCodes.contains(key)) )
        keyDown("Shift");

    unsigned int keycode = getUnicodeKeycode(key);
    XTestFakeKeyEvent(dpy, keycode, 1, 0);
    XTestFakeKeyEvent(dpy, keycode, 0, 0);
    XFlush(dpy);

    if( key.length() == 1 && (key.toLower() != key || xorgUpKeyCodes.contains(key)) )
        keyUp("Shift");
}

void keyDown(QString key) {
    XTestFakeKeyEvent(dpy, getUnicodeKeycode(key), 1, 0);
    XFlush(dpy);
}

void keyUp(QString key) {
    XTestFakeKeyEvent(dpy, getUnicodeKeycode(key), 0, 0);
    XFlush(dpy);
}

void mouseMove(int addX, int addY) {
    XTestFakeRelativeMotionEvent(dpy, addX, addY, 1);
    XFlush(dpy);
}

void mouseSetPos(int x, int y) {
    XTestFakeMotionEvent(dpy, 0, x, y, 0);
    XFlush(dpy);
}

void mouseDown(int button) {
    XTestFakeButtonEvent(dpy, button, 1, 0);
    XFlush(dpy);
    platformIndependentSleepMs(12);
}

void mouseUp(int button) {
    XTestFakeButtonEvent(dpy, button, 0, 0);
    XFlush(dpy);
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

void shutdown() {
    runCommandForResult("systemctl poweroff");
}

void restart() {
    runCommandForResult("systemctl reboot");
}

void logout() {
    if(desktopSession == "cinnamon")
        runCommandForResult("cinnamon-session-quit --logout --force");
    else if(desktopSession == "xubuntu")
        runCommandForResult("xfce4-session-logout --logout");
    else
        runCommandForResult("gnome-session-quit");
}

void sleep() {
    runCommandForResult("systemctl suspend");
}

void lock_screen() {
    qInfo() << desktopSession;
    if(desktopSession == "cinnamon")
        runCommandForResult("cinnamon-screensaver-command -l");
    else if(desktopSession == "xubuntu")
        runCommandForResult("xflock4");
    else
        runCommandForResult("gnome-screensaver-command -l");
}

void blank_screen() {
    runCommandForResult("xset dpms force off");
}

QString getApplicationNames() {
    QString result = runCommandForResult("grep -LEs 'NoDisplay|OnlyShowIn' /usr/share/applications/* | grep '.*.desktop' | xargs grep -hm 1 '^Name=' | sed 's/^.....//'");
    // if took too long to get app list return backup list gotten at start
    if(result.indexOf('\n') == result.lastIndexOf('\n'))
        return backupApplicationsList;
    else
        return result;
}

void startApplicationByName(QString name) {
    QString shellCommand = runCommandForResult("grep -rlhe '"+name+"' /usr/share/applications/* | xargs grep -he '^Exec=' | sed 's/^.....//' | grep -m 1 ''");
    if(shellCommand.indexOf(' ') >= 0)
        shellCommand = shellCommand.mid(0, shellCommand.indexOf(' ')); // "geany %F" -> "geany"
    runCommandForResult(shellCommand);
}

QString getCommandSuggestions(QString command)
{
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

QString getCpuUsage() {
    static QList< QList<int> > lastCpuUsage;
    QList< QList<int> > curCpuUsage;
    QString procStat = runCommandForResult("grep 'cpu[0-9]' /proc/stat | awk '{print $2,$4,$5}'");

    foreach( const QString coreInfo, procStat.split("\n") ) {
        QStringList usageTime = coreInfo.split(" ");
        if(usageTime.count() != 3)
            continue;

        // proc/stat command returned cpu info in following format:
        // time spent in user mode / time spent in system mode / idle time
        // appends ( total processing time, total idle time )
        curCpuUsage.append(QList<int>() << (usageTime[0].toInt() + usageTime[1].toInt()) << usageTime[2].toInt());
    }

    QStringList cpuPercentages;
    for(int i=0; i<curCpuUsage.count() && i<lastCpuUsage.count(); i++) {
        qint64 lastCheckTime = lastCpuUsage[i][0] + lastCpuUsage[i][1];
        qint64 curCheckTime = curCpuUsage[i][0] + curCpuUsage[i][1];
        qint64 timePassed = curCheckTime - lastCheckTime;

        if(timePassed == 0)
            return "";

        qint64 lastUsedTotal = lastCpuUsage[i][0];
        qint64 curUsedTotal = curCpuUsage[i][0];
        qint64 timeUsed = curUsedTotal - lastUsedTotal;

        qint64 percentOfTimeUsed = ((float)timeUsed/(float)timePassed) * 100.0f;

        cpuPercentages.append(QString::number(percentOfTimeUsed));
    }

    lastCpuUsage = curCpuUsage;
    return cpuPercentages.join(" ");
}

QString getRamUsage() {
    // used / total
    return runCommandForResult("free | grep '^Mem' | awk '{print $3,$2}'").simplified();
}

QString getProcesses() {
    // pid / cpu / mem / name
    return runCommandForResult("ps ux | awk '{print $2,$3,$4,$11}' | tail -n +2");
}

void killProcess(QString pid) {
    runCommandForResult("kill "+pid);
}

}
