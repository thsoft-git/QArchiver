#include "testresultdialog.h"
#include "ui_testresultdialog.h"


#include <QFile>
#include <QFileDialog>
#include <QTextStream>


TestResultDialog::TestResultDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TestResultDialog)
{
    ui->setupUi(this);
}

TestResultDialog::~TestResultDialog()
{
    delete ui;
}

void TestResultDialog::setTestResultText(const QStringList &lines)
{
    //ui->textEdit->setText(lines.join("\n"));
    ui->textEdit->setText(lines.join(""));
    ui->savePushButton->setEnabled(!ui->textEdit->toPlainText().isEmpty());
}

void TestResultDialog::setTestResultText(const QString &text)
{
    ui->textEdit->setText(text);
    ui->savePushButton->setEnabled(!ui->textEdit->toPlainText().isEmpty());
}

void TestResultDialog::on_savePushButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save to file"), QDir::currentPath());
    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << ui->textEdit->toPlainText() << endl;
    }
}
