#include <QString>

namespace FakeInput {

void initFakeInput();
void freeFakeInput();
void typeString(QString string);
void keyDown(QString key);
void keyUp(QString key);
void keyTap(QString key);
void mouseMove(int addX, int addY);
void mouseDown(int button);
void mouseUp(int button);
void mouseScroll(int amount);
void zoom(int amount);
void stopZoom();

void shutdown();
void restart();
void logout();
void sleep();

}
