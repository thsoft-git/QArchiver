#ifndef QDUOLED_H
#define QDUOLED_H

#include <QWidget>

class QColor;

class QDuoLed : public QWidget
{
    Q_OBJECT
public:
    explicit QDuoLed(QWidget *parent = 0);
    ~QDuoLed();

    QColor color(int index) const;
    void setColor(int index, const QColor &color);
    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

public slots:
    /**
     * @brief setColor1 set color1 to "color"
     * @param color
     */
    void setColor1(const QColor &color);

    /**
     * @brief setColor2 set color2 to "color"
     * @param color
     */
    void setColor2(const QColor &color);

    void toggle();

    /**
     * @brief turnColor set current color by index
     * index == 1: Current color is color1
     * index == 2: Current color is color2
     * @param index Index of color to set as current;
     */
    void turnColor(int index);
    void turnOn(bool on=true);
    void turnOff(bool off=true);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *);
    void mouseReleaseEvent(QMouseEvent *event);
    int ledWidth() const;

private:
    struct Private;
    Private * const m_d;
};

#endif // QDUOLED_H
