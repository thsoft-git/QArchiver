#include "extractdialog.h"
#include "ui_extractdialog.h"

#include <QDebug>
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>

ExtractDialog::ExtractDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExtractDialog)
{
    ui->setupUi(this);
    ui->newDirPushButton->hide();

    // Do not emit signals vhile addItem
    // or connect currentIndexChanged signal manualy later
    //ui->pathComboBox->blockSignals(true);
    ui->pathComboBox->addItem("");
    ui->pathComboBox->addItem(QDir::homePath());
    //ui->pathComboBox->blockSignals(false);

    //fileSystemModel = new QFileSystemModel(this);
    //fileSystemModel->setRootPath("");
    fileSystemModel = new QDirModel(this);
    // Setup sort criteria
    // directory first, ignore case,
    // and sort by name
    fileSystemModel->setSorting(QDir::DirsFirst |
                      QDir::IgnoreCase |
                      QDir::Name);

    // Tie TreeView with fileSystemModel
    ui->treeView->setModel(fileSystemModel);

    // Set initial selection
    QModelIndex index = fileSystemModel->index(QDir::homePath());
    // Highlight the selected
    ui->treeView->setCurrentIndex(index);
    ui->treeView->setRootIsDecorated(true);
    // Set initial view of directory
    // for the selected drive as expanded
    ui->treeView->expand(index);
    // Resizing the column - first column
    ui->treeView->resizeColumnToContents(0);

    // Make it scroll to the selected
    ui->treeView->scrollTo(index,QAbstractItemView::PositionAtTop);
    //ui->treeView->hideColumn(1);// for hide Size Column
    //ui->treeView->hideColumn(2);// for hide Type Column
    //ui->treeView->hideColumn(3);// for hide Date Modified Column
    //connect(fileSystemModel, SIGNAL(directoryLoaded(QString)), this, SLOT(onDirectoryLoaded(QString)));
    connect(ui->pathComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(onPathCurrentIndexChanged(QString)));
    //connect(this, SIGNAL(accepted()), this, SLOT(onDialogAccepted()));

    setSingleFolderArchive(false);
    connect(this, SIGNAL(accepted()), this, SLOT(writeSettings()));
}

ExtractDialog::~ExtractDialog()
{
    delete ui;
}

void ExtractDialog::setExtractSelectedFiles(bool value)
{
    if (value) {
        ui->filesToExtractGroupBox->show();
        ui->selectedFilesRadioButton->setChecked(true);
        ui->selectedFilesRadioButton->setEnabled(true);
        //ui->extractAllLabel->hide();
    } else  {
        //ui->filesToExtractGroupBox->hide();
        ui->selectedFilesRadioButton->setChecked(false);
        ui->selectedFilesRadioButton->setEnabled(false);
        //ui->extractAllLabel->show();
    }
}

void ExtractDialog::setSingleFolderArchive(bool value)
{
    ui->singleFolderGroupBox->setChecked(!value);
}

void ExtractDialog::setCurrentDirectory(const QString &path)
{
    qDebug() << "ExtractDialog::setCurrentDirectory(..)";
    ui->pathComboBox->setItemText(0, path);
    ui->pathComboBox->setCurrentIndex(0);

    // Set initial selection
    QModelIndex index = fileSystemModel->index(path);

    // Set initial view of directory
    // Highlight the selected
    ui->treeView->setCurrentIndex(index);
    // for the selected drive as expanded
    ui->treeView->expand(index);
    // Resizing the column - first column
    ui->treeView->resizeColumnToContents(0);
    // Make it scroll to the selected
    ui->treeView->scrollTo(index, QAbstractItemView::PositionAtTop);
}

bool ExtractDialog::extractAllFiles() const
{
    return ui->allFilesRadioButton->isChecked();
}

bool ExtractDialog::extractToSubfolder() const
{
    return ui->singleFolderGroupBox->isChecked();
}

bool ExtractDialog::preservePaths() const
{
    return ui->preservePathsCheckBox->isChecked();
}

void ExtractDialog::setPreservePaths(bool value)
{
    ui->preservePathsCheckBox->setChecked(value);
}

bool ExtractDialog::openDestDirAfterExtraction() const
{
    return ui->openFolderCheckBox->isChecked();
}

void ExtractDialog::setOpenDestFolderAfterExtraction(bool value)
{
    ui->openFolderCheckBox->setChecked(value);
}

bool ExtractDialog::closeAfterExtraction() const
{
    return ui->closeAfterCheckBox->isChecked();
}

bool ExtractDialog::overwriteExisting() const
{
    return ui->overwriteExistingCheckBox->isChecked();
}

QString ExtractDialog::destinationDirectory() const
{
    if (extractToSubfolder()) {
        return QString(ui->pathComboBox->currentText() + QLatin1Char( '/' ) + subfolder());

    } else {
        return ui->pathComboBox->currentText();
    }
}

QString ExtractDialog::subfolder() const
{
    return ui->subfolderLineEdit->text();
}

void ExtractDialog::setSubfolder(const QString &subfolder)
{
    ui->subfolderLineEdit->setText(subfolder);
}

void ExtractDialog::accept()
{
    QString destDirPath = destinationDirectory();
    qDebug() << "Dest dir path:" << destDirPath;
    while (1) {
        QFileInfo fileInfo(destDirPath);
        if (fileInfo.exists()) {
            //qDebug() << "Dest dir/file exist: ";
            if (fileInfo.isDir()) {
                if (extractToSubfolder()) {
                    // "Extract here", "Retry", "Cancel";
                    int overwrite = QMessageBox::question(this, tr("Folder exists"),
                                                          tr("The folder <i>%1</i> already exists.<br/><br/>Are you sure you want to extract here?").arg(destDirPath),
                                                          QMessageBox::Yes | QMessageBox::Retry | QMessageBox::Cancel);
                    if (overwrite == QMessageBox::Retry) {
                        // The user clicked Retry.
                        continue;
                    } else if (overwrite == QMessageBox::Cancel) {
                        return;
                    }
                }
            } else {
                QMessageBox::critical(this, tr("Error"),
                                      tr("Name <i>%1</i> already exists as a file.<br/><br/>The folder could not be created.").arg(subfolder()) );
                return;
            }
        } else {
            QDir destDir(destDirPath);
            bool mkpath_ok = destDir.mkpath(destDirPath);
            if (!mkpath_ok) {
                QMessageBox::critical(this, tr("Error"),
                                      tr("The folder <i>%1</i> could not be created.<br/><br/>Please check your permissions to create it.").arg(subfolder()));
                return;
            }
        }
        break;
    }

    QDialog::accept();
}

void ExtractDialog::scrollPath()
{
    qDebug() << "scrollPath()";
    QModelIndex index = fileSystemModel->index(ui->pathComboBox->currentText());
    // Make it scroll to the selected
    ui->treeView->scrollTo(index, QAbstractItemView::PositionAtTop);
    // Resizing the column - first column
    ui->treeView->resizeColumnToContents(0);
}

void ExtractDialog::scrollPath(const QString &path)
{
    qDebug() << "scrollPath:" << path;

    QModelIndex index = fileSystemModel->index(path);
    // collapse all expanded
    ui->treeView->collapseAll();
    // Highlight the selected
    ui->treeView->setCurrentIndex(index);
    // Make it scroll to the selected
    ui->treeView->scrollTo(index, QAbstractItemView::PositionAtTop);
    // Resizing the column - first column
    ui->treeView->resizeColumnToContents(0);
}

void ExtractDialog::scrollPathExpand()
{
    scrollPathExpand(ui->pathComboBox->currentText());
}

void ExtractDialog::scrollPathExpand(const QString path)
{
    qDebug() << "scrollPathExpand:" << path;

    QModelIndex index = fileSystemModel->index(path);
    // collapse all expanded
    ui->treeView->collapseAll();
    // Highlight the selected
    ui->treeView->setCurrentIndex(index);
    // For the selected path as expanded
    ui->treeView->expand(index);
    // Make it scroll to the selected
    ui->treeView->scrollTo(index, QAbstractItemView::PositionAtTop);
    // Resizing the column - first column
    ui->treeView->resizeColumnToContents(0);
}

void ExtractDialog::writeSettings()
{
    ;
}

void ExtractDialog::onDirectoryLoaded(const QString &path)
{
    qDebug() << "onDirectoryLoaded" << path << ui->pathComboBox->currentText();
    //if( path==ui->pathComboBox->itemText(0)){
    if( path==ui->pathComboBox->currentText()){
         QTimer::singleShot(100, this, SLOT(scrollPath()));
    }
}

void ExtractDialog::onPathCurrentIndexChanged(const QString &path)
{
    scrollPath(path);
}

void ExtractDialog::on_applyPathPushButton_clicked()
{
    QString path = ui->pathComboBox->currentText();
    QFileInfo fileInfo(path);

    if (!fileInfo.exists()) {
        QMessageBox::warning(this, qApp->applicationName(),
                             tr("Can not find folder %1.").arg(path),
                             QMessageBox::Ok, QMessageBox::Ok);
        // Adresář neexistuje -> není nutné rozbalovat cestu v treeView
        return;
    }

    scrollPathExpand(path);
}

void ExtractDialog::on_treeView_clicked(const QModelIndex &index)
{
    // Při kliknutí zobrazí zvolenou cestu v pathComboBox lineEditu
    QString path = fileSystemModel->filePath(index);
    ui->pathComboBox->setEditText(path);
}

void ExtractDialog::on_newDirPushButton_clicked()
{
    // Vytvoření nového adresáře
    // Make new directory
    QModelIndex index = ui->treeView->currentIndex();
    if(!index.isValid()) return;

    if(fileSystemModel->isReadOnly()) {
        QMessageBox::warning(this, "Create folder", "Creating folder is not enabled here.\n\nFolder is created automaticaly.");
        return;
    }

    QString name  = QInputDialog::getText(this, "Create folder", "Enter a name:");
    if(name.isEmpty()) return;

    fileSystemModel->mkdir(index, name);
}
