#include "fixedsvgwidget.h"

FixedSvgWidget::FixedSvgWidget(QWidget *parent) : QWidget(parent)
{
    this->renderer = new QSvgRenderer(this);
}

void FixedSvgWidget::setRenderer(QSvgRenderer *renderer)
{
    this->renderer = renderer;
}

QSvgRenderer *FixedSvgWidget::getRenderer()
{
    return this->renderer;
}

void FixedSvgWidget::load(const QString &filename)
{
    renderer->load(filename);
}

void FixedSvgWidget::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    //p.drawRect(0,0,this->width()-1,this->height()-1);

    QRectF containerBounds(0,0, this->width(), this->height());
    QRectF svgBounds(containerBounds);

    double svgAspectRatio = renderer->defaultSize().width()/renderer->defaultSize().height();
    double widgetAspectRatio = containerBounds.width()/containerBounds.height();

    if(svgAspectRatio == 0)
    {
        return;
    }
    // SVG wider than its container, decrease SVG's height to fit AR.
    else if(svgAspectRatio > widgetAspectRatio)
    {
        svgBounds.setHeight(containerBounds.width() / svgAspectRatio);
    }
    // SVG taller than its container, decrease SVG's width to fit AR
    else if(svgAspectRatio < widgetAspectRatio)
    {
        svgBounds.setWidth(containerBounds.height() * svgAspectRatio);
    }

    // Center SVG inside widget
    svgBounds.moveTop((containerBounds.height() - svgBounds.height())/2);
    svgBounds.moveLeft((containerBounds.width() - svgBounds.width())/2);

    renderer->render(&p, svgBounds);
}
