#include "propertiesdialog.h"
#include "ui_propertiesdialog.h"

#include "qarchive.h"
#include "QSizeFormater.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>

PropertiesDialog::PropertiesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PropertiesDialog)
{
    ui->setupUi(this);
    ui->compressionBarGraf->setText(tr("Compression"));
    ui->compressionBarGraf->setValue(100);
    setWindowTitle(tr("Archive Properties"));
}

PropertiesDialog::~PropertiesDialog()
{
    delete ui;
}

void PropertiesDialog::setArchive(Archive *archive)
{
    m_archive = archive;
    QFileInfo fileInfo(m_archive->fileName());

    ui->labelFileNane->setText(fileInfo.fileName());
    ui->labelLocation->setText(fileInfo.path());
    ui->labelArchiveType->setText(m_archive->mimeType().comment());
    ui->labelFileSize->setText(QSizeFormater::convertSize(m_archive->archiveFileSize()));
    ui->labelFileSize->setToolTip(QString("%1 B").arg(locale().toString(m_archive->archiveFileSize())));
    ui->labelUncompressedSize->setText(QSizeFormater::convertSize(m_archive->extractedSize()));
    ui->labelUncompressedSize->setToolTip(QString("%1 B").arg(locale().toString(m_archive->extractedSize())));
    ui->labelEntryCount->setText(QString().setNum(m_archive->entryCount()));

    ui->compressionBarGraf->setMinimum(0);
    ui->compressionBarGraf->setMaximum(m_archive->extractedSize());
    ui->compressionBarGraf->setValue(m_archive->archiveFileSize());

    ui->labelCreationDate->setText(locale().toString(fileInfo.created(), "d. MMMM yyyy hh:mm:ss"));
    ui->labelModified->setText(locale().toString(fileInfo.lastModified(), "d. MMMM yyyy hh:mm:ss"));
}
