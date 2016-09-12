#include "qbargraf.h"

#include <QPainter>

QBarGraf::QBarGraf(QWidget *parent) :
    QWidget(parent)
{
    m_text ="";
    m_min = 0; //Properties
    m_max = 100;
    m_value = 50;
}

QBarGraf::~QBarGraf()
{
}

QSize QBarGraf::sizeHint() const
{
    int preferedWidth = qMax(45, fontMetrics().width(m_text));
    return QSize(preferedWidth, 255);
}

QString QBarGraf::text() const
{
    return m_text;
}

void QBarGraf::setText(const QString &txt)
{
    m_text = txt;
    update();
}

int QBarGraf::minimum() const
{
    return m_min;
}

int QBarGraf::maximum() const
{
    return m_max;
}

void QBarGraf::setMinimum(int min)
{
    m_min = min;
}

void QBarGraf::setMaximum(int max)
{
    m_max = max;
}

void QBarGraf::setValue(int val)
{
    m_value = val;
    update();
}

void QBarGraf::paintEvent(QPaintEvent *)
{

    static const QPointF points[4] = {
        QPointF(0.0, 8.0),
        QPointF(15.0, 0.0),
        QPointF(30.0, 8.0),
        QPointF(15.0, 16.0)
    };

//    static const QPointF points2[4] = {
//        QPointF(0.0, 208.0),
//        QPointF(15.0, 200.0),
//        QPointF(30.0, 208.0),
//        QPointF(15.0, 216.0)
//    };

    static const QPointF levyBok[4] = {
        QPointF(0.0, 8.0),
        QPointF(15.0, 16.0),
        QPointF(15.0, 216.0),
        QPointF(0.0, 208.0)
    };
    static const QPointF pravyBok[4] = {
        QPointF(15.0, 16.0),
        QPointF(30.0, 8.0),
        QPointF(30.0, 208.0),
        QPointF(15.0, 216.0)
    };

    static const double bottom = 200;
    static const double top = 0;
    double percent =((double)(m_value - m_min) / (double)(m_max - m_min));
    int y =  bottom - qRound(percent * (bottom - top));

    QPointF points1[4] = {
        QPointF(0.0, y+8.0),
        QPointF(15.0, y+0.0),
        QPointF(30.0, y+8.0),
        QPointF(15.0, y+16.0)
    };

    QPointF levyBok1[4] = {
        QPointF(0.0, y+8.0),
        QPointF(15.0, y+16.0),
        QPointF(15.0, 216.0),
        QPointF(0.0, 208.0)
    };
    QPointF pravyBok1[4] = {
        QPointF(15.0, y+16.0),
        QPointF(30.0, y+8.0),
        QPointF(30.0, 208.0),
        QPointF(15.0, 216.0)
    };

    QPainter painter(this);
    painter.setFont(font());
    QRect labelBound = painter.boundingRect(rect(), Qt::AlignLeft|Qt::AlignTop, m_text);
    QRect labelRect = QRect(0, 0, rect().width(), labelBound.height());
    painter.drawText(labelRect, Qt::AlignHCenter, m_text);
    //painter.drawText(rect(), Qt::AlignCenter, "Qt");
    painter.translate(rect().width()/2 - 15, labelRect.height() + 10);

    painter.setPen(QPen(QColor(64,255,64,160).lighter(120)));
    painter.setBrush(QBrush(QColor(64,255,64,200)));
    painter.drawPolygon(points1, 4);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(0,0,255,64)));
    painter.drawPolygon(points, 4);
    //painter.drawPolygon(points2, 4);
    painter.drawPolygon(pravyBok, 4);
    //painter.setBrush(QBrush(QColor(0,0,255,128).darker(200)));
    painter.drawPolygon(levyBok, 4);
    painter.drawPolygon(levyBok, 4);

    painter.setPen(QPen(QColor(0,0,255,64)));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolygon(points, 4);
    //painter.drawPolygon(points2, 4);
    painter.drawPolygon(pravyBok, 4);
    painter.drawPolygon(levyBok, 4);

    painter.setPen(QPen(QColor(64,255,64,160).lighter(120)));
    painter.setBrush(QBrush(QColor(64,255,64,160)));
    painter.drawPolygon(pravyBok1, 4);
    painter.setBrush(QBrush(QColor(64,255,64,160).darker(120)));
    painter.drawPolygon(levyBok1, 4);

    painter.translate(-(rect().width()/2 - 15), 220);
    double ratio = ((double)(m_value - m_min) / (double)(m_max - m_min))*100;
    painter.setPen(Qt::black);
    painter.drawText(labelRect, Qt::AlignHCenter, QString().setNum(ratio, 'f', 2).append(" %") );
}
