#include "jobs.h"

#include <QThread>
#include <QFileInfo>
#include <QDebug>

#include "ArchiveTools/archiveinterface.h"

class Job::JobThread : public QThread
{
public:
    JobThread(Job *job, QObject *parent = 0)
        : QThread(parent), j(job)
    {
        connect(j, SIGNAL(result(QJob*)), SLOT(quit()));
        j->connect(this, SIGNAL(finished()),SLOT(onThreadQuit()));
    }

    virtual void run();

private:
    Job *j;
};

void Job::JobThread::run()
{
    j->doWork();

    if (j->isRunning()) {
        exec();
    }
}


Job::Job(Archive *arch, QObject *parent)
    : QJob(parent)
    , m_archive(arch)
    , m_archiveInterface(arch->interface())
    , m_isRunning(false)
    , d(new JobThread(this))
{
    static bool onlyOnce = false;
    if (!onlyOnce) {
        qRegisterMetaType<QPair<QString, QString> >("QPair<QString,QString>");
        onlyOnce = true;
    }

    setCapabilities(QJob::Killable);
}

Job::~Job()
{
    if (d->isRunning()) {
        d->quit();
        d->wait(550);
        d->terminate();
        d->wait();
    }

    delete d;
}

ArchiveInterface *Job::archiveInterface()
{
    return m_archiveInterface;
}

bool Job::isRunning() const
{
    return m_isRunning;
}

bool Job::success() const
{
    return m_OK;
}

Archive *Job::getArchive() const
{
    return m_archive;
}

void Job::start()
{
    m_isRunning = true;
    d->start();
    emit started(this);
}

void Job::emitResult()
{
    QJob::emitResult();
    m_isRunning = false;
}

void Job::connectToArchiveInterfaceSignals()
{
    //qDebug()<<"Job::connectToArchiveInterfaceSignals(): archiveInterface: " << archiveInterface();
    connect(archiveInterface(), SIGNAL(error(QString,QString)), SLOT(onError(QString,QString)));
    connect(archiveInterface(), SIGNAL(entry(ArchiveEntry)), SLOT(onEntry(ArchiveEntry)));
    connect(archiveInterface(), SIGNAL(entryRemoved(QString)), SLOT(onEntryRemoved(QString)));
    connect(archiveInterface(), SIGNAL(totalProgress(int)), SLOT(onProgress(int)));
    connect(archiveInterface(), SIGNAL(encodingInfo(QString)), SLOT(onInfo(QString)));
    connect(archiveInterface(), SIGNAL(finished(bool)), SLOT(onFinished(bool)));
    connect(archiveInterface(), SIGNAL(userQuery(Query*)), SLOT(onUserQuery(Query*)));
    connect(archiveInterface(), SIGNAL(currentFile(const QString &)), SIGNAL(currentFile(const QString &)));
    connect(archiveInterface(), SIGNAL(currentFileProgress(int)), SIGNAL(currentFileProgress(int)));

    //archiveInterface()->moveToThread(this->d);
}

void Job::onError(const QString & message, const QString & details)
{
    Q_UNUSED(details)
    //QString err_txt = (message + "<br/>" + details);
    QString errTxt = (message + "\n" + details);

    setError(1);
    setErrorText(errTxt);
}

void Job::onEntry(const ArchiveEntry & archiveEntry)
{
    //qDebug() << "Job:onEntry \n\n";
    emit newEntry(archiveEntry);
}

void Job::onProgress(int value)
{
    setPercent(static_cast<unsigned long>(value));
}

void Job::onInfo(const QString& _info)
{
    emit infoMessage(this, _info);
    emit info(_info);
}

void Job::onEntryRemoved(const QString & path)
{
    emit entryRemoved(path);
}

void Job::onFinished(bool result)
{
    //qDebug() << "Job::onFinished(bool): " << result;
    m_OK = result;

    //archiveInterface()->disconnect(this);
    emitResult();
}

void Job::onUserQuery(Query *query)
{
    emit userQuery(query);
}

void Job::onThreadQuit()
{
    qDebug()<< "Job thread quit";
}

bool Job::doKill()
{
    qDebug();
    bool ret = archiveInterface()->doKill();
    if (!ret) {
        qDebug() << "Killing does not seem to be supported here.";
    }
    return ret;
}

ListJob::ListJob(Archive *arch, QObject *parent)
    : Job(arch, parent)
    , m_isSingleFolderArchive(true)
    , m_isPasswordProtected(false)
    , m_extractedFilesSize(0)
{
    connect(this, SIGNAL(newEntry(ArchiveEntry)), this, SLOT(onNewEntry(ArchiveEntry)));
}

ListJob::~ListJob()
{
    qDebug("%s deleted...", metaObject()->className());
}

void ListJob::doWork()
{
    emit description(this, tr("Loading archive"));
    emit currentArchive(archiveInterface()->filename());
    qDebug() << "ListJob::doWork(): Loading archive...";
    connectToArchiveInterfaceSignals();
    bool ret = archiveInterface()->list();

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

qlonglong ListJob::extractedFilesSize() const
{
    return m_extractedFilesSize;
}

bool ListJob::isPasswordProtected() const
{
    return m_isPasswordProtected;
}

bool ListJob::isSingleFolderArchive() const
{
    return m_isSingleFolderArchive;
}

void ListJob::onNewEntry(const ArchiveEntry& entry)
{
    m_extractedFilesSize += entry[ Size ].toLongLong();
    m_isPasswordProtected |= entry [ IsPasswordProtected ].toBool();

    if (m_isSingleFolderArchive) {
        const QString fileName(entry[FileName].toString());
        const QString basePath(fileName.split(QLatin1Char( '/' )).at(0));

        if (m_basePath.isEmpty()) {
            m_basePath = basePath;
            if (basePath == fileName) {
                m_subfolderName = entry[IsDirectory].toBool() ? basePath : QString();
            }else {
                m_subfolderName = basePath;
            }
        } else {
            if (m_basePath != basePath) {
                m_isSingleFolderArchive = false;
                m_subfolderName.clear();
            }
        }
    }
}

QString ListJob::subfolderName() const
{
    return m_subfolderName;
}


ExtractJob::ExtractJob(const QVariantList& files, const QString& destinationDir, ExtractionOptions options, Archive *arch, QObject *parent)
    : Job(arch, parent)
    , m_files(files)
    , m_destinationDir(destinationDir)
    , m_options(options)
{
    setDefaultOptions();
}

ExtractJob::~ExtractJob()
{
    qDebug("%s deleted...", metaObject()->className());
}

QVariantList ExtractJob::filesToExtract() const
{
    return m_files;
}

void ExtractJob::doWork()
{
    QString desc = tr("Extracting files");
    QString info_msg = tr("Please wait, QArchiver extracting %1.");
    QString info;
    if (m_files.count() == 0) {
        info = tr("all files");
    } else {
        info = m_files.count() == 1 ? tr("one file") : tr("%1 files").arg(m_files.count());
    }
    emit description(this, desc);
    emit infoMessage(this, info_msg.arg(info));
    emit currentArchive(archiveInterface()->filename());

    connectToArchiveInterfaceSignals();

    qDebug() << "Starting extraction with selected files:"
             << m_files
             << "Destination dir:" << m_destinationDir
             << "Options:" << m_options;

    bool ret = archiveInterface()->copyFiles(m_files, m_destinationDir, m_options);

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

void ExtractJob::setDefaultOptions()
{
    ExtractionOptions defaultOptions;

    defaultOptions[QLatin1String("PreservePaths")] = false;

    ExtractionOptions::const_iterator it = defaultOptions.constBegin();
    for (; it != defaultOptions.constEnd(); ++it) {
        if (!m_options.contains(it.key())) {
            m_options[it.key()] = it.value();
        }
    }
}

QString ExtractJob::destinationDirectory() const
{
    return m_destinationDir;
}

ExtractionOptions ExtractJob::extractionOptions() const
{
    return m_options;
}

void ExtractJob::setOption(const QString &optName, const QVariant &value)
{
    m_options.insert(optName, value);
}

AddJob::AddJob(const QStringList& files, const CompressionOptions& options , Archive *arch, QObject *parent)
    : Job(arch, parent)
    , m_files(files)
    , m_options(options)
{
}

AddJob::~AddJob()
{
    qDebug("%s deleted...", metaObject()->className());
}

void AddJob::setOption(const QString &optName, const QVariant &value)
{
    m_options.insert(optName, value);
}

void AddJob::doWork()
{
    emit description(this, tr("Adding files"));
    emit currentArchive(archiveInterface()->filename());

    ArchiveInterface *writeInterface = archiveInterface();
    Q_ASSERT(writeInterface);

    connectToArchiveInterfaceSignals();
    bool ret = writeInterface->addFiles(m_files, m_options);

    if (!writeInterface->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

DeleteJob::DeleteJob(const QVariantList& files, Archive *arch, QObject *parent)
    : Job(arch, parent)
    , m_files(files)
{
}

DeleteJob::~DeleteJob()
{
    qDebug("%s deleted...", metaObject()->className());
}


void DeleteJob::doWork()
{
    QString desc = tr("Deleting files");
    QString info_msg = tr("Please wait, QArchiver deleting %1.");
    QString info = m_files.count() == 1 ? tr("a file from the archive") : tr("%1 files").arg(m_files.count());

    emit description(this, desc);
    emit infoMessage(this, info_msg.arg(info));
    emit currentArchive(archiveInterface()->filename());

    ArchiveInterface *writeInterface = archiveInterface();
    Q_ASSERT(writeInterface);

    connectToArchiveInterfaceSignals();
    int ret = writeInterface->deleteFiles(m_files);

    if (!writeInterface->waitForFinishedSignal()) {
        onFinished(ret);
    }
}


TestJob::TestJob(Archive *arch, QObject *parent)
    :Job(arch, parent)
{
}

TestJob::~TestJob()
{
    qDebug("%s deleted...", metaObject()->className());
}

QStringList TestJob::testResult()
{
    return m_testResult;
}

void TestJob::doWork()
{
    emit description(this, tr("Testing archive"));
    emit currentArchive(archiveInterface()->filename());

    connectToArchiveInterfaceSignals();
    connect(archiveInterface(), SIGNAL(testResult(const QString &)), SLOT(onTestResult(const QString &)));
    bool ret = archiveInterface()->test();

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

void TestJob::onTestResult(const QString &line)
{
    m_testResult.append(line);
}


OpenJob::OpenJob(Archive *arch, QObject *parent)
    :Job(arch, parent)
{
}

OpenJob::~OpenJob()
{
    qDebug("%s deleted...", metaObject()->className());
}


void OpenJob::onEntry(const ArchiveEntry &archiveEntry)
{
    Q_UNUSED(archiveEntry)
    //    m_extractedFilesSize += entry[ Size ].toLongLong();
    //    m_isPasswordProtected |= entry [ IsPasswordProtected ].toBool();
}

void OpenJob::doWork()
{
    emit description(this, tr("Opening archive"));
    //emit infoMessage(this, tr("Please wait, QArchiver opening archive."));
    emit currentArchive(archiveInterface()->filename());
    //qDebug() << "OpenJob::doWork(): Opening archive...";
    connectToArchiveInterfaceSignals();

    ArchiveInterface *ifc = archiveInterface();
    bool ret = ifc->open();

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}
