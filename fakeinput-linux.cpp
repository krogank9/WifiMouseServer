#include <QString>
#include <QSet>
#include <QStringList>
#include <QProcess>
extern "C" {
    #include <xdo.h>
    #include <ctime>
}
#include "fakeinput.h"

//note: compile with: gcc main.cpp linux.cpp -lxdo

namespace FakeInput {

QString desktopSession = "";

QString getOsName()
{
    return "linux";
}

void platformIndependentSleepMs(qint64 ms) {
    const int MS_TO_NANO_MULTIPLIER = 1000000;
    struct timespec tim;
    tim.tv_sec = ms/1000;
    tim.tv_nsec = ((unsigned)ms)*MS_TO_NANO_MULTIPLIER;
    nanosleep(&tim, NULL);

    desktopSession = runCommandForResult("echo $DESKTOP_SESSION").simplified();
}

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

void mouseSetPos(int x, int y) {
    xdo_move_mouse(xdoInstance, x, y, 0);
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

void shutdown() {
    QProcess cmd;
    cmd.start("systemctl poweroff");
    cmd.waitForFinished(1000);
}

void restart() {
    QProcess cmd;
    cmd.start("systemctl reboot");
    cmd.waitForFinished(1000);
}

void logout() {
    QProcess cmd;
    if(desktopSession == "cinnamon")
        cmd.start("cinnamon-session-quit --logout --force");
    else if(desktopSession == "xubuntu")
        cmd.start("xfce4-session-logout --logout");
    else
        cmd.start("gnome-session-quit");

    cmd.waitForFinished(1000);
}

void sleep() {
    QProcess cmd;
    cmd.start("systemctl suspend");
    cmd.waitForFinished(1000);
}

// To get list of application names separated by \n:
// grep -LEs "NoDisplay|OnlyShowIn" /usr/share/applications/* | grep ".*.desktop" | xargs grep -hm 1 "^Name=" | sed 's/^.....//'
// To get the shell command for program name:
// grep -rlhe "LibreOffice" /usr/share/applications/* | xargs grep -he "^Exec=" | sed 's/^.....//' | grep -m 1 ""
QString runCommandForResult(QString command)
{
    QProcess cmd;
    // trick to startDetached process and still read its output, use sh
    cmd.start("/bin/sh", QStringList()<< "-c" << command);
    cmd.waitForFinished(250);
    return cmd.readAllStandardOutput() + cmd.readAllStandardError();
}

QString getCommandSuggestions(QString command)
{
    QProcess cmd1;
    command.replace("\"","\\\"");
    cmd1.start("/bin/sh", QStringList()<< "-c" << "grep \"^"+command+"\" ~/.bash_history");
    cmd1.waitForFinished(250);
    QString history_results = cmd1.readAllStandardOutput();

    QProcess cmd2;
    cmd2.start("/bin/bash", QStringList()<< "-c" << "compgen -c | grep \"^"+command+"\"");
    cmd2.waitForFinished(250);
    QString compgen_results = cmd2.readAllStandardOutput();

    QStringList list = compgen_results.split("\n") + history_results.split("\n");
    for(int i=0; i<list.count(); i++) // remove trailing whitespace
        list[i] = list[i].simplified();
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

    //qInfo() << list.join("\n");

    return list.join("\n");
}

}
