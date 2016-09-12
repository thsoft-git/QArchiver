#include "overwritedialog.h"
#include "ui_overwritedialog.h"

OverwriteDialog::OverwriteDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OverwriteDialog)
{
    ui->setupUi(this);
}

OverwriteDialog::~OverwriteDialog()
{
    delete ui;
}
