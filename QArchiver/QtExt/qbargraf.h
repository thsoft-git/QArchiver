#ifndef QBARGRAF_H
#define QBARGRAF_H

#include <QWidget>

class QBarGraf : public QWidget
{
    Q_OBJECT
public:
    explicit QBarGraf(QWidget *parent = 0);
    ~QBarGraf();
    virtual QSize sizeHint() const;
    QString text() const;
    void setText(const QString &txt);
    int minimum() const;
    int maximum() const;
    
signals:
    
public slots:
    void setMinimum(int min);
    void setMaximum(int max);
    void setValue(int val);
    
protected:
    void paintEvent(QPaintEvent *);

private:
    QString m_text;
    int m_min;
    int m_max;
    int m_value;
};

#endif // QBARGRAF_H
