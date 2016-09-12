#ifndef ARCHIVEINTERFACE_H
#define ARCHIVEINTERFACE_H

#include "qarchive.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

class Query;

enum ArchiveOpenMode{
    ReadOnly,
    ReadWrite,
    Create = ReadWrite
};

class ArchiveInterface : public QObject
{
    Q_OBJECT
public:
    explicit ArchiveInterface(QObject *parent = 0);
    virtual ~ArchiveInterface();

    static QStringList supportedMimetypes(ArchiveOpenMode mode = ReadOnly);

    QString filename() const;
    QString password() const;
    void setPassword(const QString &password);
    Archive *getArchive() const;
    void setArchive(Archive *arch);
    virtual bool isReadOnly() const;

    /* Open archive */
    virtual bool open() = 0;

    /* List archive contents.*/
    virtual bool list() = 0;

    /* Test archive */
    virtual bool test() = 0;

    /* Extract files from archive. */
    virtual bool copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options) = 0;

    // See qarchive.h for a list of what the compressionoptions might contain
    virtual bool addFiles(const QStringList & files, const CompressionOptions& options) = 0;
    virtual bool deleteFiles(const QList<QVariant> & files) = 0;

    bool waitForFinishedSignal() const;
    virtual bool doKill();
    
signals:
    void error(const QString &message, const QString &details = QString());
    void entry(const ArchiveEntry &archiveEntry);
    void entryRemoved(const QString &path);
    void totalProgress(int progress);
    void encodingInfo(const QString &info);
    void finished(bool result);
    void userQuery(Query *query);
    void currentFile(const QString &fileName);
    void currentFileProgress(int progress);
    void testResult(const QString &line);
    void charset(const QString &name, const QString &description);
    
public slots:
    
protected:
    /*  */
    void setWaitForFinishedSignal(bool value);
    Archive *m_archive;

private:
    bool m_waitForFinishedSignal;
};

#endif // ARCHIVEINTERFACE_H
