#ifndef OVERWRITEDIALOG_H
#define OVERWRITEDIALOG_H

#include <QDialog>

namespace Ui {
class OverwriteDialog;
}

class OverwriteDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit OverwriteDialog(QWidget *parent = 0);
    ~OverwriteDialog();
    
private:
    Ui::OverwriteDialog *ui;
};

#endif // OVERWRITEDIALOG_H
