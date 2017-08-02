#include "rotatingsquare.h"
#include <math.h>
#include <QDebug>

RotatingSquare::RotatingSquare(float x, float y, float rotation, float size, float xVel, float yVel, float rotationVel)
{
    this->x = x;
    this->y = y;
    this->rotation = rotation;
    this->size = size;

    this->xVel = xVel;
    this->yVel = yVel;
    this->rotationVel = rotationVel;

    this->aabb = new QRectF(0,0,0,0);
    this->lastTime = QDateTime::currentMSecsSinceEpoch();

    // When rotated on a 45 degree angle the size becomes the dist between two diagonals of the square
    this->rotatedSizeIncrease = sqrt(size*size + size*size) - size;
}

RotatingSquare::~RotatingSquare()
{
    delete aabb;
}

void RotatingSquare::updateAABB()
{
    float amountRotated = abs(abs(fmod(rotation, 90.0f)) - 45.0f) / 45.0f;
    amountRotated = 1.0f - amountRotated;

    float calcSize = size + rotatedSizeIncrease*amountRotated;
    this->aabb->setRect(x-calcSize/2,y-calcSize/2,calcSize,calcSize);
}

void RotatingSquare::update(float containerWidth, float containerHeight)
{
    updateAABB();

    // calculate seconds elapsed since last frame
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    float elapsedSecs = ((float)(now - lastTime))/1000.0f;
    lastTime = now;
    if(elapsedSecs > 1.0f)
        elapsedSecs = 1.0f;

    // adjust position & rotation based on velocity & elapsed time
    x += xVel * elapsedSecs;
    y += yVel * elapsedSecs;
    rotation += rotationVel * elapsedSecs;

    // bounce off walls
    if(aabb->x() < 0 && xVel < 0)
        xVel *= -1;
    if(aabb->right() > containerWidth && xVel > 0)
        xVel *= -1;
    if(aabb->y() < 0 && yVel < 0)
        yVel *= -1;
    if(aabb->bottom() > containerHeight && yVel > 0)
        yVel *= -1;
}
