#include "adddialog.h"
#include "ui_adddialog.h"

#include <QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QStandardItemModel>
#include <QtGui/QRadioButton>
#include <QtMimeTypes/QMimeDatabase>


#include "iconprovider.h"
#include "archivetoolmanager.h"

#define PATH_DATA_ROLE Qt::UserRole + 1

AddDialog::AddDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Compress to Archive"));
}

AddDialog::AddDialog(const QStringList &itemsToAdd, const QString &startDir, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Compress to Archive"));
    setSelectedFilePath(startDir);
    setupArchiveFormatGroup();
    setupIconList(itemsToAdd);

    if (itemsToAdd.size() == 1) {
        // Set up a default name if there's only one file to compress
        const QFileInfo fileInfo(itemsToAdd.first());
        //const QString fileName = fileInfo.isDir() ? fileInfo.dir().dirName() : fileInfo.baseName();
        const QString archiveName = fileInfo.baseName();
        setSelectedFilePath(startDir + "/" + archiveName + "." + currentMimeTypeSuffix());
    }else {
        // Set up a defult archive name to parent directory of first entry
        qDebug() << "startDir" << startDir;
        const QFileInfo fileInfo(startDir);
        const QString archiveName = fileInfo.baseName();
        setSelectedFilePath(startDir + "/" + archiveName + "." + currentMimeTypeSuffix());
    }
}

AddDialog::~AddDialog()
{
    delete ui;
}

void AddDialog::setupArchiveFormatGroup()
{
    QMimeDatabase db;
    layout_format = new QGridLayout(ui->groupBoxArchiveFormat);
    layout_format->setObjectName("layout_format");

    rb_format_zip = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_zip->setObjectName("rb_format_zip");
    rb_format_zip->setText("zip");
    mimeMap[rb_format_zip] = db.mimeTypeForName("application/zip");
    rb_format_zip->setToolTip(mimeMap[rb_format_zip].comment());
    connect(rb_format_zip, SIGNAL(clicked()), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_zip, 0, 0);

    rb_format_7z = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_7z->setObjectName("rb_format_7z");
    rb_format_7z->setText("7z");
    mimeMap[rb_format_7z] = db.mimeTypeForName("application/x-7z-compressed");
    rb_format_7z->setToolTip(mimeMap[rb_format_7z].comment());
    connect(rb_format_7z, SIGNAL(clicked(bool)), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_7z, 1, 0);

    rb_format_ar = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_ar->setObjectName("rb_format_ar");
    rb_format_ar->setText("a");
    mimeMap[rb_format_ar] = db.mimeTypeForName("application/x-archive");
    rb_format_ar->setToolTip(mimeMap[rb_format_ar] .comment());
    connect(rb_format_ar, SIGNAL(clicked(bool)), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_ar, 2, 0);

    rb_format_cpio = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_cpio->setObjectName("rb_format_cpio");
    rb_format_cpio->setText("cpio");
    mimeMap[rb_format_cpio] = db.mimeTypeForName("application/x-cpio");
    rb_format_cpio->setToolTip(mimeMap[rb_format_cpio].comment());
    connect(rb_format_cpio, SIGNAL(clicked(bool)), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_cpio, 0, 1);

    rb_format_cpio_gz = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_cpio_gz->setObjectName("rb_format_cpio_gz");
    rb_format_cpio_gz->setText("cpio.gz");
    mimeMap[rb_format_cpio_gz] = db.mimeTypeForName("application/x-cpio-compressed");
    rb_format_cpio_gz->setToolTip(mimeMap[rb_format_cpio_gz].comment());
    connect(rb_format_cpio_gz, SIGNAL(clicked(bool)), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_cpio_gz, 1, 1);

    rb_format_tar = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_tar->setObjectName("rb_format_tar");
    rb_format_tar->setText("tar");
    mimeMap[rb_format_tar] = db.mimeTypeForName("application/x-tar");
    rb_format_tar->setToolTip(mimeMap[rb_format_tar].comment());
    connect(rb_format_tar, SIGNAL(clicked(bool)), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_tar, 2, 1);

    rb_format_tar_Z = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_tar_Z->setObjectName("rb_format_tar_Z");
    rb_format_tar_Z->setText("tar.Z");
    mimeMap[rb_format_tar_Z] = db.mimeTypeForName("application/x-tarz");
    rb_format_tar_Z->setToolTip(mimeMap[rb_format_tar_Z].comment());
    connect(rb_format_tar_Z, SIGNAL(clicked(bool)), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_tar_Z, 0, 2);

    rb_format_tar_gz = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_tar_gz->setObjectName("rb_format_tar_gz");
    rb_format_tar_gz->setText("tar.gz");
    mimeMap[rb_format_tar_gz] = db.mimeTypeForName("application/x-compressed-tar");
    rb_format_tar_gz->setToolTip(mimeMap[rb_format_tar_gz].comment());
    connect(rb_format_tar_gz, SIGNAL(clicked(bool)), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_tar_gz, 1, 2);

    rb_format_tar_bz2 = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_tar_bz2->setObjectName("rb_format_tar_bz2");
    rb_format_tar_bz2->setText("tar.bz2");
    mimeMap[rb_format_tar_bz2] = db.mimeTypeForName("application/x-bzip-compressed-tar");
    rb_format_tar_bz2->setToolTip(mimeMap[rb_format_tar_bz2].comment());
    connect(rb_format_tar_bz2, SIGNAL(clicked(bool)), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_tar_bz2, 2, 2);

    rb_format_tar_lzma = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_tar_lzma->setObjectName("rb_format_tar_lzma");
    rb_format_tar_lzma->setText("tar.lzma");
    mimeMap[rb_format_tar_lzma] = db.mimeTypeForName("application/x-lzma-compressed-tar");
    rb_format_tar_lzma->setToolTip(mimeMap[rb_format_tar_lzma].comment());
    connect(rb_format_tar_lzma, SIGNAL(clicked(bool)), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_tar_lzma, 0, 3);

    rb_format_tar_xz = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_tar_xz->setObjectName("rb_format_tar_xz");
    rb_format_tar_xz->setText("tar.xz");
    mimeMap[rb_format_tar_xz] = db.mimeTypeForName("application/x-xz-compressed-tar");
    rb_format_tar_xz->setToolTip(mimeMap[rb_format_tar_xz].comment());
    connect(rb_format_tar_xz, SIGNAL(clicked(bool)), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_tar_xz, 1, 3);

    rb_format_rar = new QRadioButton(ui->groupBoxArchiveFormat);
    rb_format_rar->setObjectName("rb_format_rar");
    rb_format_rar->setText("rar");
    mimeMap[rb_format_rar] = db.mimeTypeForName("application/x-rar");
    rb_format_rar->setToolTip(mimeMap[rb_format_rar].comment());
    connect(rb_format_rar, SIGNAL(clicked(bool)), this, SLOT(onArchiveFormatChanged()) );
    layout_format->addWidget(rb_format_rar, 2, 3);

    // Set default format
    rb_format_tar_bz2->click();
}

void AddDialog::onArchiveFormatChanged()
{
    QRadioButton *format_button = qobject_cast<QRadioButton *>(sender());
    if (format_button == NULL) {
        qDebug("Warning: SLOT(AddDialog::archiveFormatChanged()): sender() není QRadioButton!");
        return;
    }

    QString filePath = ui->lineEditSelectedPath->text().trimmed();
    qDebug() << "Trimmed: "<< filePath;

    QStringList suffList = m_currentMimeType.suffixes();
    foreach (const QString &suff, suffList) {
        if (filePath.endsWith(suff)) {
            // size of filePath without suffix
            int path_size = filePath.size() - suff.size();
            filePath.truncate(path_size);
            break;
        }
    }
    qDebug() << "Truncate: "<< filePath;

    m_currentMimeType = mimeMap.value(format_button);

    if (filePath.endsWith(QChar('.'))) {
        filePath.append(format_button->text());
    }else {
        filePath.append('.' + format_button->text());
    }
    qDebug() << "Append new suffix: "<< filePath;

    ui->lineEditSelectedPath->setText(filePath);
}


QString AddDialog::selectedFilePath() const
{
    return ui->lineEditSelectedPath->text();
}

void AddDialog::setSelectedFilePath(const QString &filePath)
{
    ui->lineEditSelectedPath->setText(filePath);
}

QMimeType AddDialog::currentMimeType()
{
    return m_currentMimeType;
}

QString AddDialog::currentMimeTypeSuffix()
{
    QString suffix = m_currentMimeType.preferredSuffix();
    if (suffix == "tar.bz") {
        return suffix.append("2");
    }
    return suffix;
}

QString AddDialog::currentMimeTypeName()
{
    return currentMimeType().name();
}

void AddDialog::done(int r)
{
    if(QDialog::Accepted == r)  // ok was pressed
    {
        // Validate the filePath
        QFileInfo fileInfo(selectedFilePath());
        if(fileInfo.exists())
        {
//            QMessageBox::warning(this, tr("Overwrite File?"),
//                                 tr("The file \"%1\" already exists. Do you wish to overwrite it?").arg(fileInfo.fileName()),
//                                 tr("Overwrite"), tr("Cancel"));
//            //statusBar->setText("Invalid data in text edit...try again...");

            QMessageBox warnMsg;
            warnMsg.setIcon(QMessageBox::Warning);
            warnMsg.setText(tr("The file \"%1\" already exists.").arg(fileInfo.fileName()));
            //warnMsg.setInformativeText("Do you want to save your changes?");
            warnMsg.setInformativeText(tr("Do you wish to overwrite it?"));
            QPushButton* overWriteBtn = warnMsg.addButton(tr("Overwrite"), QMessageBox::AcceptRole);
            /*QPushButton* cancelBtn = */warnMsg.addButton(QMessageBox::Cancel);
            warnMsg.setDefaultButton(overWriteBtn);
            int ret = warnMsg.exec();
            if (ret == 0) {
                // Overwrite
                if ( QFile::remove(fileInfo.absoluteFilePath()) ) {
                    // File successfuly removed
                    QDialog::done(r);
                    return;
                } else {
                    // Can not remove the file
                    QMessageBox::critical(this, tr("Error"), tr("The file \"%1\" can not be overwriten!").arg(fileInfo.fileName()));
                }
            }
            return;
        }
        else
        {
            QDialog::done(r);
            return;
        }
    }
    else    // cancel, close or exc was pressed
    {
        QDialog::done(r);
        return;
    }
}

void AddDialog::setupIconList(const QStringList &itemsToAdd)
{
    QStandardItemModel* listModel = new QStandardItemModel(this);
    QStringList sortedList(itemsToAdd);

    sortedList.sort();

    Q_FOREACH(const QString& path, sortedList) {

        QStandardItem* item = new QStandardItem;
        item->setText(QFileInfo(path).fileName());

        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFile(path);
        QString iconName = mime.iconName();
        item->setIcon(QIcon::fromTheme(iconName, QIcon(IconProvider::fileIcon(path))));
        // PathDataRole = UserRole + 1
        item->setData(QVariant(path), PATH_DATA_ROLE);
        item->setToolTip(path);

        listModel->appendRow(item);
    }

    ui->listViewCompressList->setModel(listModel);
}

QString AddDialog::suffixFromFilterStr(const QString &filterStr)
{
    /*
     * RegExp for capturing suffix from filter string
     * in format "*.7z" or "Archiv 7-zip (*.7z)"
     */
    QRegExp filter_regex("(?:^\\*\\.(?!.*\\()|\\(\\*\\.)(\\w+\\.*\\w*)");

    QString suffix;

    if (filter_regex.indexIn(filterStr) != -1) {
        suffix = filter_regex.cap(1);
        qDebug() << suffix;
    }
    return suffix;
}

void AddDialog::on_pushButtonBrowse_clicked()
{
    /* Actual path */
    QFileInfo info(selectedFilePath());
    QString path = info.absoluteFilePath();
    qDebug() << path;
    QString filterString = m_currentMimeType.filterString();
    QString selectedFilter;
    //QString fileName = QFileDialog::getSaveFileName(this, tr("Find archive"), path, filterString, &selectedFilter, QFileDialog::DontConfirmOverwrite);
    QString fileName = getFileName(this, tr("Find archive"), path, filterString, &selectedFilter);

    if (fileName.isEmpty()) {
        // Canceled
        return;
    }

    QString suffix = suffixFromFilterStr(selectedFilter);
    if (!suffix.isEmpty())
    {
        if (!fileName.endsWith(suffix)) {
             fileName += fileName.endsWith(QChar('.')) ? suffix : (QChar('.') + suffix);
        }
    }
    setSelectedFilePath(fileName);
}

QString AddDialog::getFileName(QWidget *parent, const QString &title, const QString &path, const QString &filter, QString *selectedFilter)
{

    QFileInfo fileInfo(path);
    QString dirPath = fileInfo.isDir() ? fileInfo.absoluteFilePath() : fileInfo.absolutePath();
    qDebug() << "Dir:" << dirPath;
    QString fileName = fileInfo.isDir() ? fileInfo.fileName()+'.'+currentMimeTypeSuffix() : fileInfo.fileName();
    qDebug() << "fileName:" << fileName;

    QFileDialog dialog(parent, title, path);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(filter);
    dialog.selectFile(fileName);

    if (dialog.exec() == QDialog::Accepted)
    {
        fileName = dialog.selectedFiles().first();
        if (selectedFilter) {
            *selectedFilter = dialog.selectedNameFilter();
        }
    }
    else {
        fileName = QString();
    }

    return fileName;
}

QString AddDialog::getFileName2(QWidget *parent, const QString &title, const QString &path, const QStringList &filters, QString *selectedFilter)
{
    //The file "archive.cpio" already exists. Do you wish to overwrite it?
    /*
     * RegEx for capturing suffix from filter string
     * in format "*.7z" or "Archiv 7-zip (*.7z)"
     */
    QRegExp filter_regex(QLatin1String("(?:^\\*\\.(?!.*\\()|\\(\\*\\.)(\\w+\\.*\\w*)"));
    QFileInfo fileInfo(path);
    QString fileSuffix = fileInfo.completeSuffix().toLower();
    QStringList filterList(filters);

    // Find current filter, move to front
    QString currentFileFilter;
    for (int i = 0; i < filterList.count(); ++i) {
        if (filterList.at(i).contains(QRegExp("\\*\\."+fileSuffix+"[ )]{1}"))){
            currentFileFilter = filterList.at(i);
            qDebug() <<"Current filter"<<  currentFileFilter;
            filterList.move(i, 0);
            break;
        }
    }

    QFileDialog dialog(parent, title, path);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilters(filterList);

    qDebug() << "FileDialogCurrentdir:" << dialog.directory().absolutePath();
    qDebug() << "FileDialogSelected:" << dialog.selectedFilter();
    // Selectne ale nezobrazuje se to!!! => Řešení: přesunout aktualni na 1. pozici
    dialog.selectNameFilter(currentFileFilter);
    dialog.selectFile(fileInfo.fileName());
    qDebug() << "FileDialogSelected:" << dialog.selectedFilter();

    if (dialog.exec() == QDialog::Accepted)
    {
        QString file_name = dialog.selectedFiles().first();
        if (selectedFilter) {
            *selectedFilter = dialog.selectedFilter();
        }
        QFileInfo info(file_name);
        if (info.suffix().isEmpty() && !dialog.selectedNameFilter().isEmpty()) {
            if (filter_regex.indexIn(dialog.selectedNameFilter()) != -1) {
                QString extension = filter_regex.cap(1);
                qDebug() << extension;
                file_name += QLatin1String(".") + extension;
            }
        }
        return file_name;
    }
    else {
        return QString();
    }
}
