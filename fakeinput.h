namespace FakeInput {

void initFakeInput();
void freeFakeInput();
void typeString(wchar_t *string);
void keyDown(char *key);
void keyUp(char *key);
void keyTap(char *key);
void mouseMove(int addX, int addY);
void mouseDown(int button);
void mouseUp(int button);
void mouseScroll(int amount);

}
