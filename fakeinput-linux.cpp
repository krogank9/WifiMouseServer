#include <QString>
#include <QSet>
#include <QStringList>
#include <QProcess>
#include <QDateTime>
#include <QDebug>
#include <QMap>
extern "C" {
    #include <xdo.h>
    #include <ctime>
}
#include "fakeinput.h"

//note: compile with: gcc main.cpp linux.cpp -lxdo

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

xdo_t *xdoInstance;
QString backupApplicationsList;

void initFakeInput() {
    // Sometimes grepping app list can take too long. get apps at start & wait up to 5 seconds to ensure we always have apps to display
    QProcess cmd;
    cmd.start("/bin/sh", QStringList()<< "-c" << "grep -LEs 'NoDisplay|OnlyShowIn' /usr/share/applications/* | grep '.*.desktop' | xargs grep -hm 1 '^Name=' | sed 's/^.....//'");
    cmd.waitForFinished(5000);
    backupApplicationsList = cmd.readAllStandardOutput();

    desktopSession = runCommandForResult("echo $DESKTOP_SESSION").simplified();
    qInfo() << "desktopSession" << desktopSession;
    xdoInstance = xdo_new(0);
}

void freeFakeInput() {
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

        // Note: this starts glitching out if you set the pause too short.
        // Maybe because for unicode characters, the keys must be remapped.
        // 95ms seems to be sufficient.
        xdo_send_keysequence_window(xdoInstance, CURRENTWINDOW, unicodeHex.toLocal8Bit().data(), 95000);
    }
    else {
        char str[] = {(char)c, '\0'};
        xdo_enter_text_window(xdoInstance, CURRENTWINDOW, str, 12000);
    }
}

void typeString(QString string) {
    int i = 0;
    while(i < string.length()) {
        typeChar( string.at(i++).unicode() );
    }
}

QMap<QString, QString> xorgKeyNames {
    {"Ctrl", "Control"}, {"Esc", "Escape"}, {"Win", "Super_L"},
    {"PgU", "Page_Up"}, {"PgD", "Page_Down"}, {"Space", "space"},

    {"VolumeUp", "XF86AudioRaiseVolume"}, {"VolumeDown", "XF86AudioLowerVolume"}, {"VolumeMute", "XF86AudioMute"},
    {"PauseSong", "XF86AudioPlay"}, {"NextSong","XF86AudioNext"}, {"PrevSong", "XF86AudioPrev"},

    {"BrightnessUp", "XF86MonBrightnessUp"}, {"BrightnessDown", "XF86MonBrightnessDown"}
};

void keyTap(QString key) {
    xdo_send_keysequence_window(xdoInstance, CURRENTWINDOW, xorgKeyNames.value(key, key).toLocal8Bit(), 12000);
}

void keyDown(QString key) {
    xdo_send_keysequence_window_down(xdoInstance, CURRENTWINDOW, xorgKeyNames.value(key, key).toLocal8Bit(), 12000);
}

void keyUp(QString key) {
    xdo_send_keysequence_window_up(xdoInstance, CURRENTWINDOW, xorgKeyNames.value(key, key).toLocal8Bit(), 12000);
}

void mouseMove(int addX, int addY) {
    xdo_move_mouse_relative(xdoInstance, addX, addY);
}

void mouseSetPos(int x, int y) {
    xdo_move_mouse(xdoInstance, x, y, 0);
}

void mouseDown(int button) {
    xdo_mouse_down(xdoInstance, CURRENTWINDOW, button);
    platformIndependentSleepMs(15); // have to sleep or instantaneous mouse down then up wont register the up
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
