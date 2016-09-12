#include "addtoarchive.h"

#include <QWeakPointer>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>

#include "adddialog.h"
#include "progressdialog.h"
#include <QMessageBox>
#include "archivetoolmanager.h"
#include "qarchive.h"
#include "jobs.h"

AddToArchive::AddToArchive(QObject *parent) :
    QJob(parent)
{
}

AddToArchive::~AddToArchive()
{
}

bool AddToArchive::showAddDialog(void)
{
    AddDialog* dialog = new AddDialog(m_inputs, m_firstPath, NULL);

    bool ret = dialog->exec();

    if (ret) {
        qDebug() << "Returned path:" << dialog->selectedFilePath();
        qDebug() << "Returned mime:" << dialog->currentMimeTypeName();
        setFilename(dialog->selectedFilePath());
        setMimeType(dialog->currentMimeTypeName());
    }

    delete dialog;

    return ret;
}

void AddToArchive::setStorePaths(bool value)
{
    m_storePaths = value;
}

void AddToArchive::connectDialog(ProgressDialog *dialog)
{
    m_dialog = dialog;
}

void AddToArchive::addInput(const QString &path)
{
    /*qDebug() << "AddToArchive::addInput():" << path;
    qDebug() << "Clean path: " << QDir::cleanPath(path);
    QFileInfo fi(path);
    qDebug() << fi.path() << endl
             << fi.absolutePath() << endl
             << fi.filePath() << endl
             << fi.absoluteFilePath() << endl
             << fi.dir().dirName()<< endl
             << fi.fileName()<< endl
             << fi.isDir();*/

    QString clean_path = QDir::cleanPath(path);
    m_inputs << clean_path;

    if (m_firstPath.isEmpty()) {
        QString firstEntry = clean_path;
        m_firstPath = QFileInfo(firstEntry).dir().absolutePath();
    }
}

void AddToArchive::addFiles(const QVariantList &paths)
{
    for (int i = 0; i < paths.size(); ++i) {
        addInput(QDir::fromNativeSeparators(paths.at(i).toString()));
    }
}

void AddToArchive::setAutoFilenameSuffix(const QString &suffix)
{
    m_autoFilenameSuffix = suffix;
}

void AddToArchive::setFilename(const QString &path)
{
    m_filename = path;
}

void AddToArchive::setMimeType(const QString &mimeType)
{
    m_mimeType = mimeType;
}

void AddToArchive::start()
{
    QTimer::singleShot(0, this, SLOT(slotStartJob()));
}

void AddToArchive::slotFinished(QJob *job)
{
    qDebug() << "AddToArchive::slotFinished";

    if (job->error()) {
        QMessageBox::critical(NULL, tr("Error"), job->errorText());
    }

    emitResult();
}


void AddToArchive::slotStartJob(void)
{
    qDebug();
    emit started(this);

    CompressionOptions options;

    if (!m_inputs.size()) {
        QMessageBox::warning(NULL, tr("Warning"), tr("No input files were given."));
        return;
    }

    Archive *archive;
    if (!m_filename.isEmpty()) {
        Q_ASSERT(!m_mimeType.isEmpty());
        archive = ArchiveToolManager::createArchive(m_mimeType, m_filename, this);
        qDebug() << "Set filename to " << m_filename;
    } else {
        if (m_autoFilenameSuffix.isEmpty()) {
            QMessageBox::critical(NULL, tr("Error"), tr("You need to either supply a filename for the archive or a suffix (such as zip, tar.gz) with the <i>--autofilename</i> argument."));
            return;
        }

        if (m_firstPath.isEmpty()) {
            qDebug() << "Weird, this should not happen. no firstpath defined. aborting";
            return;
        }

        QString base = QFileInfo(m_inputs.first()).absoluteFilePath();
        if (base.endsWith(QLatin1Char('/'))) {
            base.chop(1);
        }

        QString finalName = base + QLatin1Char( '.' ) + m_autoFilenameSuffix;

        //if file already exists, append a number to the base until it doesn't exist
        int appendNumber = 0;
        while (QFileInfo(finalName).exists()) {
            ++appendNumber;
            finalName = base + QLatin1Char( '_' ) + QString::number(appendNumber) + QLatin1Char( '.' ) + m_autoFilenameSuffix;
        }

        qDebug() << "Autoset filename to "<< finalName;

        Q_ASSERT(m_mimeType.isEmpty());
        archive = m_mimeType.isEmpty() ? ArchiveToolManager::create(finalName, this) : ArchiveToolManager::createArchive(m_mimeType, finalName, this);
    }

    if (archive == NULL) {
        QMessageBox::critical(NULL, tr("Error"), tr("Failed to create the new archive. Permissions might not be sufficient."));
        return;
    } else if (archive->isReadOnly()) {
        QMessageBox::critical(NULL, tr("Error"), tr("It is not possible to create archives of this type."));
        return;
    }

    if (m_firstPath.isEmpty()) {
        qDebug() << "Weird, this should not happen. no firstpath defined. aborting";
        return;
    }

    const QDir stripDir(m_firstPath);

    for (int i = 0; i < m_inputs.size(); ++i) {
        m_inputs[i] = stripDir.absoluteFilePath(m_inputs.at(i));
    }

    options[QLatin1String( "GlobalWorkDir" )] = stripDir.path();
    qDebug() << "Setting options[\"GlobalWorkDir\"] to " << stripDir.path();


    AddJob *job = archive->addFiles(m_inputs, options);

    m_dialog->registerJob(job);

    connect(job, SIGNAL(result(QJob*)), this, SLOT(slotFinished(QJob*)));

    job->start();
}

