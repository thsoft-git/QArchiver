#include "qarchive.h"

#include "jobs.h"
#include "queries.h"
#include "ArchiveTools/archiveinterface.h"

#include <QEventLoop>
#include <QDir>
#include <QFileInfo>
#include <QDebug>


Archive::Archive(ArchiveInterface *interface, QObject *parent) :
    QObject(parent),
    m_iface(interface),
    m_hasBeenListed(false),
    m_isPasswordProtected(false),
    m_isSingleFolderArchive(false)
{
    Q_ASSERT(interface);
    interface->setParent(this);
    interface->setArchive(this);

    m_entryCount = 0;
    m_extractedFilesSize = 0;
    m_compressedFilesSize = 0;

    if (QMetaType::type("ArchiveEntry") == 0) {
        qRegisterMetaType<ArchiveEntry>("ArchiveEntry");
    }
}


Archive::Archive(const QMimeType &mimeType, const QString &fileName, ArchiveInterface *interface, QObject *parent) :
    QObject(parent),
    m_mimeType(mimeType),
    m_fileName(fileName),
    m_iface(interface),
    m_hasBeenListed(false),
    m_isPasswordProtected(false),
    m_isSingleFolderArchive(false)
{
    Q_ASSERT(interface);
    interface->setParent(this);
    interface->setArchive(this);
    m_entryCount = 0;
    m_extractedFilesSize = 0;
    m_compressedFilesSize = 0;

    if (QMetaType::type("ArchiveEntry") == 0) {
        qRegisterMetaType<ArchiveEntry>("ArchiveEntry");
    }
}

OpenJob *Archive::open()
{
    OpenJob* job = new OpenJob(this, this);
    return job;
}

QJob *Archive::create()
{
    // zatím nevyužito
    return 0;
}

ListJob* Archive::list()
{
    ListJob *job = new ListJob(this, this);
    //job->setAutoDelete(false); //NOTE: Pokud autodelete false, nutno job po použití smazat!!!

    // Collect/Update some information about the archive
    connect(job, SIGNAL(result(QJob*)), this, SLOT(onListFinished(QJob*)));

    return job;
}

DeleteJob* Archive::deleteFiles(const QList<QVariant> & files)
{
    if (m_iface->isReadOnly()) {
        return NULL;
    }

    DeleteJob *newJob = new DeleteJob(files, this, this);
    return newJob;
}

AddJob* Archive::addFiles(const QStringList & files, CompressionOptions options)
{
    //Q_ASSERT(!m_iface->isReadOnly());

    if (m_iface->isReadOnly()) {
        return NULL;
    }

    AddJob *newJob = new AddJob(files, options, this, this);
    connect(newJob, SIGNAL(result(QJob*)), this, SLOT(onAddFinished(QJob*)));

    return newJob;
}

ExtractJob* Archive::copyFiles(const QList<QVariant> & files, const QString & destinationDir, ExtractionOptions options)
{
    ExtractionOptions newOptions = options;

    ExtractJob *newJob = new ExtractJob(files, destinationDir, newOptions, this, this);
    //newJob->setAutoDelete(false); //NOTE: Pokud autodelete false, nutno job po použití smazat!!!
    return newJob;
}

TestJob *Archive::testArchive()
{
    TestJob *newJob = new TestJob(this, this);
    return newJob;
}

ArchiveInterface *Archive::interface()
{
    return m_iface;
}

bool Archive::exists() const
{
    return QFileInfo(fileName()).exists();
}

bool Archive::isOpen() const
{
    return m_hasBeenListed;
}

bool Archive::isReadOnly() const
{
    bool fileIsReadOnly = true;
    bool ifaceIsReadOnly = m_iface->isReadOnly();


    // Check if file is writable
    QFileInfo fileInfo(fileName());
    if (fileInfo.exists()) {
        fileIsReadOnly = !fileInfo.isWritable();
    } else {
        // Should also check if we can create a file in that directory
        QDir parentDir = fileInfo.dir();
        fileInfo.setFile(parentDir.absolutePath());

        if (fileInfo.exists() && fileInfo.isDir()) {
            fileIsReadOnly = !fileInfo.isWritable();
        }else {
            fileIsReadOnly = true;
        }
    }

    return (fileIsReadOnly || ifaceIsReadOnly);
}


qint64 Archive::archiveFileSize() const
{
    return QFileInfo(fileName()).size();
}

QMimeType Archive::mimeType() const
{
    return m_mimeType;
}

void Archive::setMimeType(const QMimeType &mime)
{
    QMutexLocker locker(&mutex);
    m_mimeType = mime;
}

void Archive::onAddFinished(QJob* job)
{
    // if the archive was previously a single folder archive and an add job
    // has successfully finished, then it is no longer a single folder archive
    if (m_isSingleFolderArchive && !job->error()) {
        m_isSingleFolderArchive = false;
        createSubfolderName();
    }
}

QString Archive::createSubfolderName()
{
    QFileInfo fi(fileName());
    QString base = fi.completeBaseName();

    // Special cases for:
    if (base.right(4).toUpper() == QLatin1String(".TAR")) {
        // tar.gz/bzip2 files
        base.chop(4);
    }
    else if (base.right(5).toUpper() == QLatin1String(".CPIO")) {
        // cpio.gz files
        base.chop(5);
    }

    m_subfolderName = base;
    return base;
}

void Archive::onListFinished(QJob *job)
{
    qDebug()<< "Archive::onListFinished()";

    ListJob *ljob = qobject_cast<ListJob*>(job);
    //m_extractedFilesSize = ljob->extractedFilesSize();
    m_isSingleFolderArchive = ljob->isSingleFolderArchive();
    m_isPasswordProtected = ljob->isPasswordProtected();
    m_subfolderName = ljob->subfolderName();
    if (m_subfolderName.isEmpty()) {
        createSubfolderName();
    }
    m_hasBeenListed = true;
}

QString Archive::fileName() const
{
    return m_fileName;
}

void Archive::setFileName(const QString &fileName)
{
    m_fileName = fileName;
}


bool Archive::isSingleFolderArchive() const
{
    return m_isSingleFolderArchive;
}

void Archive::setSingleFolderArchive(bool value)
{
    m_isSingleFolderArchive = value;
}


bool Archive::isPasswordProtected() const
{
    return m_isPasswordProtected;
}


QString Archive::password() const
{
    return m_password;
}


void Archive::setPassword(const QString &password)
{
    if (!password.isEmpty()) {
        m_isPasswordProtected = true;
    }

    m_password = password;
}


QString Archive::subfolderName() const
{
    return m_subfolderName;
}


QString Archive::codePage() const
{
    return m_codepage;
}


void Archive::setCodePage(const QString &cp)
{
    if (m_codepage != cp) {
        m_codepage = cp;
        emit codePageChanged(m_codepage);
    }
}


int Archive::entryCount() const
{

    return m_entryCount;
}


void Archive::setEntryCount(int count)
{
    m_entryCount = count;
}


qint64 Archive::extractedSize() const
{
    return m_extractedFilesSize;
}


void Archive::setExtractedSize(qint64 size)
{
    m_extractedFilesSize = size;
}


qint64 Archive::compressedSize() const
{
    return m_compressedFilesSize;
}


void Archive::setCompressedSize(qint64 size)
{
    m_compressedFilesSize  = size;
}

