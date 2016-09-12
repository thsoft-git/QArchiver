#include <QtGui>
#include "qduoled.h"

struct QDuoLed::Private
{
    Private()
        : darkerFactor(300), color(&colors[1]), isOn(true)
    {
        colors[0] = Qt::gray;
        colors[1] = Qt::green;
        colors[2] = Qt::red;
    }

    int darkerFactor;
    QColor colors [3];
    QColor *color;
    bool isOn;
    friend class QDuoLed;
};


QDuoLed::QDuoLed(QWidget *parent)
    :QWidget(parent), m_d(new Private)
{
}

QDuoLed::~QDuoLed()
{
    delete m_d;
}

QColor QDuoLed::color(int index) const
{
    return m_d->colors[index];
}

void QDuoLed::setColor(int index, const QColor &color)
{
    if ((0 <= index) && (index <=2)) {
        if (m_d->colors[index] == color)
            return;
        m_d->colors[index] = color;
        update();
    }
}

void QDuoLed::setColor1(const QColor &color)
{
    if (m_d->colors[1] == color)
        return;
    m_d->colors[1] = color;
    update();
}

void QDuoLed::setColor2(const QColor &color)
{
    if (m_d->colors[2] == color)
        return;
    m_d->colors[2] = color;
    update();
}

QSize QDuoLed::sizeHint() const
{
    return QSize(20, 20);
}

QSize QDuoLed::minimumSizeHint() const
{
    return QSize(16, 16);
}


void QDuoLed::toggle()
{
    m_d->isOn = !m_d->isOn;
    update();
}

void QDuoLed::turnColor(int index)
{
    if ((1 <= index) && (index <=2)) {
        m_d->color = &m_d->colors[index];
        update();
    }
}

void QDuoLed::turnOn(bool on)
{
    m_d->isOn = on;
    update();
}

void QDuoLed::turnOff(bool off)
{
    turnOn(!off);
}

void QDuoLed::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen = painter.pen();

    //draw border
    QPen border_pen;
    border_pen.setWidth(2);

    const int angle = -720;
    const int width = ledWidth();
    QColor color = palette().color(QPalette::Light);
    painter.setBrush(Qt::NoBrush);

    for (int arc=120; arc<2880; arc+=240) {
        border_pen.setColor(color);
        painter.setPen(border_pen);
        int w = width - border_pen.width()/2;
        painter.drawArc(border_pen.width()/2, border_pen.width()/2, w, w, angle+arc, 240);
        painter.drawArc(border_pen.width()/2, border_pen.width()/2, w, w, angle-arc, 240);
        color = color.darker(110);
    }

    // draw LED light
    color = m_d->isOn ? *m_d->color : m_d->colors[0].darker(m_d->darkerFactor);

    QRadialGradient radialGrad(QPointF(width/2, width/2), width/2, QPointF(1.25*width/3, 1.25*width/3));
    radialGrad.setColorAt(0, color.lighter(230));
    radialGrad.setColorAt(0.8, color);
    radialGrad.setColorAt(1, color.darker());

    QBrush brush(radialGrad);
    painter.setBrush(brush);
    painter.setPen(pen);
    //painter.setPen(Qt::NoPen);

    painter.drawEllipse(1, 1, width-1, width-1);
}

void QDuoLed::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton ) {
        emit clicked();
    }
}

int QDuoLed::ledWidth() const
{
    int width = qMin(this->width(), this->height());
    width -= 2;
    return width > 0 ? width : 0;
}

