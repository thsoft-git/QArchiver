#ifndef PROPERTIESDIALOG_H
#define PROPERTIESDIALOG_H

#include <QDialog>

class Archive;

namespace Ui {
class PropertiesDialog;
}

class PropertiesDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit PropertiesDialog(QWidget *parent = 0);
    ~PropertiesDialog();
    void setArchive(Archive* archive);
    
private:
    Ui::PropertiesDialog *ui;
    Archive *m_archive;
};

#endif // PROPERTIESDIALOG_H
