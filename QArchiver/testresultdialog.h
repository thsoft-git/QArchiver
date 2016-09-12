#ifndef TESTRESULTDIALOG_H
#define TESTRESULTDIALOG_H

#include <QDialog>

namespace Ui {
class TestResultDialog;
}

class TestResultDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit TestResultDialog(QWidget *parent = 0);
    ~TestResultDialog();

    void setTestResultText(const QStringList &lines);
    void setTestResultText(const QString &text);
    
private slots:

    void on_savePushButton_clicked();

private:
    Ui::TestResultDialog *ui;
};

#endif // TESTRESULTDIALOG_H
