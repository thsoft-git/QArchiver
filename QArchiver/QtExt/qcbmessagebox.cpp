#include "qcbmessagebox.h"

#include <QCheckBox>
#include <QLayout>
#include <QDebug>

QCbMessageBox::QCbMessageBox(QWidget *parent) :
    QMessageBox(parent)
{
}

QCheckBox *QCbMessageBox::checkBox() const
{
    return m_checkBox;
}

void QCbMessageBox::setCheckBox(QCheckBox *cb)
{
    if (cb == NULL) {
        if (m_checkBox != NULL) {
            delete m_checkBox;
            m_checkBox = NULL;
        }
    }
    else {
        m_checkBox = cb;
        m_checkBox->setParent(this);
        m_checkBox->blockSignals(true);
        addButton(m_checkBox, QMessageBox::ResetRole);
    }
}
