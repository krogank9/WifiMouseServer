#ifndef FIXEDSVGWIDGET_H
#define FIXEDSVGWIDGET_H

#include <QWidget>
#include <QSvgRenderer>
#include <QPainter>

// Renders an SVG centered and fit while maintaining aspect ratio

class FixedSvgWidget : public QWidget
{
public:
    explicit FixedSvgWidget(QWidget *parent = 0);

    void paintEvent(QPaintEvent *event);
    void load(const QString &filename);
    void setRenderer(QSvgRenderer *renderer);
    QSvgRenderer *getRenderer();
private:
    QSvgRenderer *renderer;
};

#endif // FIXEDSVGWIDGET_H
