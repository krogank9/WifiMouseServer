#ifndef ROTATINGSQUARE_H
#define ROTATINGSQUARE_H

#include <QDateTime>
#include <QRectF>
#include <QObject>

class RotatingSquare
{
public:
    RotatingSquare(float x = 0, float y = 0, float rotation = 0, float size = 1, float xVel = 0, float yVel = 0, float rotationVel = 0);
    ~RotatingSquare();

    void update(float containerWidth, float containerHeight);

    float x, y, rotation, size;
    float xVel, yVel, rotationVel;

    float rotatedSizeIncrease;

    QRectF *aabb;
private:
    void updateAABB();

    qint64 lastTime;
};

#endif // ROTATINGSQUARE_H
