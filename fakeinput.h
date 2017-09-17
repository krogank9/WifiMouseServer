#include <QString>

namespace FakeInput {

void platformIndependentSleepMs(qint64 ms);
QString getOsName();

QString getApplicationNames();
void startApplicationByName(QString name);

QString getCommandSuggestions(QString command);
QString runCommandForResult(QString command);
QString getCpuUsage();
QString getRamUsage();
QString getProcesses();
void killProcess(QString pid);

void initFakeInput();
void freeFakeInput();
void typeString(QString string);
void keyDown(QString key);
void keyUp(QString key);
void keyTap(QString key);
void mouseMove(int addX, int addY);
void mouseSetPos(int x, int y);
void mouseDown(int button);
void mouseUp(int button);
void mouseScroll(int amount);
void zoom(int amount);
void stopZoom();

void shutdown();
void restart();
void logout();
void sleep();
void lock_screen();
void blank_screen();

}
