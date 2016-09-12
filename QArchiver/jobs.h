#ifndef JOBS_H
#define JOBS_H

#include "qarchive.h"
#include "queries.h"
#include "jobinterface.h"
#include <QObject>
#include <QList>
#include <QVariant>
#include <QString>


/*Jobs*/
class Job : public QJob
{
    Q_OBJECT

public:
    void start();

    bool isRunning() const;
    bool success() const;
    Archive *getArchive() const;

protected:
    explicit Job(Archive *arch, QObject *parent = 0);
    virtual ~Job();
    virtual bool doKill();
    virtual void emitResult();

    ArchiveInterface *archiveInterface();
    void connectToArchiveInterfaceSignals();

public slots:
    virtual void doWork() = 0;

protected slots:
    virtual void onError(const QString &message, const QString &details);
    virtual void onInfo(const QString &_info);
    virtual void onEntry(const ArchiveEntry &archiveEntry);
    virtual void onProgress(int progress);
    virtual void onEntryRemoved(const QString &path);
    virtual void onFinished(bool result);
    virtual void onUserQuery(Query *query);
    void onThreadQuit();

signals:
    void entryRemoved(const QString & entry);
    void error(const QString& errorMessage, const QString& details);
    void newEntry(const ArchiveEntry &);
    void userQuery(Query*);
    void currentArchive(const QString &fileName);
    void currentFile(const QString &fileName);
    void currentFileProgress(int progress);
    void info(const QString &plain);

private:
    Archive *m_archive;
    ArchiveInterface *m_archiveInterface;

    bool m_isRunning;
    bool m_OK;

    class JobThread;
    JobThread * const d;
}; // END class Job


class ListJob : public Job
{
    Q_OBJECT

public:
    explicit ListJob(Archive *arch, QObject *parent = 0);
    virtual ~ListJob();

    qlonglong extractedFilesSize() const;
    bool isPasswordProtected() const;
    bool isSingleFolderArchive() const;
    QString subfolderName() const;

public slots:
    virtual void doWork();

private:
    bool m_isSingleFolderArchive;
    bool m_isPasswordProtected;
    QString m_subfolderName;
    QString m_basePath;
    qlonglong m_extractedFilesSize;

private slots:
    void onNewEntry(const ArchiveEntry&);
}; // END class ListJob

class ExtractJob : public Job
{
    Q_OBJECT

public:
    explicit ExtractJob(const QVariantList& files, const QString& destinationDir, ExtractionOptions options, Archive *arch, QObject *parent = 0);
    virtual ~ExtractJob();

    QVariantList filesToExtract() const;
    QString destinationDirectory() const;
    ExtractionOptions extractionOptions() const;
    void setOption(const QString& optName, const QVariant& value);

public slots:
    virtual void doWork();

private:
    void setDefaultOptions();

    QVariantList m_files;
    QString m_destinationDir;
    ExtractionOptions m_options;
}; // END class ExtractJob

class AddJob : public Job
{
    Q_OBJECT

public:
    explicit AddJob(const QStringList& files, const CompressionOptions& options, Archive *arch, QObject *parent = 0);
    virtual ~AddJob();
    void setOption(const QString& optName, const QVariant& value);

public slots:
    virtual void doWork();

private:
    QStringList m_files;
    CompressionOptions m_options;
}; // END class AddJob

class DeleteJob : public Job
{
    Q_OBJECT

public:
    explicit DeleteJob(const QVariantList& files, Archive *arch, QObject *parent = 0);
    virtual ~DeleteJob();

public slots:
    virtual void doWork();

private:
    QVariantList m_files;
}; // END class DeleteJob


class TestJob : public Job
{
    Q_OBJECT
public:
    explicit TestJob(Archive *arch, QObject *parent = 0);
    virtual ~TestJob();

    QStringList testResult();

public slots:
    virtual void doWork();

protected slots:
    void onTestResult(const QString &line);

private:
    QStringList m_testResult;
}; // END class TestJob


class OpenJob : public Job
{
    Q_OBJECT
public:
    explicit OpenJob(Archive *arch, QObject *parent = 0);
    virtual ~OpenJob();

protected slots:
    virtual void onEntry(const ArchiveEntry &archiveEntry);

public slots:
    virtual void doWork();
}; // END class OpenJob


#endif // JOBS_H
