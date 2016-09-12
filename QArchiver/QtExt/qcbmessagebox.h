#ifndef QCBMESSAGEBOX_H
#define QCBMESSAGEBOX_H

#include <QMessageBox>

class QCheckBox;

class QCbMessageBox : public QMessageBox
{
    Q_OBJECT
public:
    explicit QCbMessageBox(QWidget *parent = 0);
    QCheckBox *checkBox() const;
    void setCheckBox(QCheckBox *cb);
    
signals:
    
public slots:
    
private:
    QCheckBox *m_checkBox;
};

#endif // QCBMESSAGEBOX_H
