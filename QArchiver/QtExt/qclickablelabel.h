#ifndef QCLICKABLELABEL_H
#define QCLICKABLELABEL_H

#include <QLabel>

class QClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit QClickableLabel(QWidget *parent = 0);
    explicit QClickableLabel(const QString &text, QWidget *parent = 0, Qt::WindowFlags f = 0);
    ~QClickableLabel();
    
signals:
    void clicked();
    
public slots:
    
protected:
    void mousePressEvent(QMouseEvent* event);
};

#endif // QCLICKABLELABEL_H
