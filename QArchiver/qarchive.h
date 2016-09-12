#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QVariant>
#include <QMutex>
#include <QtMimeTypes/QMimeDatabase>

class QJob;
class OpenJob;
class ListJob;
class ExtractJob;
class DeleteJob;
class AddJob;
class TestJob;
class Query;
class ArchiveInterface;

/* Meta data related to one entry in a compressed archive. */
enum EntryMetaDataType {
    FileName = 0,        /* The entry's file name */
    InternalID,          /* The entry's internal ID */
    Permissions,         /* The entry's permissions */
    Owner,               /* The user the entry belongs to */
    Group,               /* The user group the entry belongs to */
    Size,                /* The entry's uncompressed size */
    CompressedSize,      /* The compressed size for the entry */
    Link,                /* The entry is a symbolic link */
    Ratio,               /* The compression ratio for the entry */
    CRC,                 /* The entry's CRC */
    Method,              /* The compression method used on the entry */
    Version,             /* The archiver version needed to extract the entry */
    Timestamp,           /* The timestamp for the current entry */
    IsDirectory,         /* The entry is a directory */
    Comment,
    IsPasswordProtected, /* The entry is password-protected */
    Custom = 1048576
};

// Archive entry
typedef QHash<int, QVariant> ArchiveEntry;

// Compression / Extraction opt
typedef QHash<QString, QVariant> CompressionOptions;
typedef QHash<QString, QVariant> ExtractionOptions;


// AddOption
enum AddOptions {
    AddReplace = 0,
    AddNew = 1,
    AddExisting = 2
};


class Archive : public QObject
{
    Q_OBJECT

public:
    explicit Archive(ArchiveInterface *interface, QObject *parent = 0);
    Archive(const QMimeType &mimeType, const QString &fileName, ArchiveInterface *interface, QObject *parent = 0);

    // open, analyze
    OpenJob *open();
    // create
    QJob* create();
    // list
    ListJob* list();
    // delete
    DeleteJob* deleteFiles(const QList<QVariant> & files);
    // add
    AddJob* addFiles(const QStringList & files, CompressionOptions options = CompressionOptions());
    // extract
    ExtractJob* copyFiles(const QList<QVariant> & files, const QString & destinationDir, ExtractionOptions options = ExtractionOptions());
    // test archive
    TestJob* testArchive();

    ArchiveInterface *interface();
    bool exists() const;
    bool isOpen() const;
    bool isReadOnly() const;
    qint64 archiveFileSize() const;
    qint64 extractedSize() const;
    void setExtractedSize(qint64 size);
    qint64 compressedSize() const;
    void setCompressedSize(qint64 size);
    QString type() const;
    QDateTime lastModified() const;
    QMimeType mimeType() const;
    void setMimeType(const QMimeType &mime);
    QString fileName() const;
    void setFileName(const QString &fileName);
    bool isSingleFolderArchive() const;
    void setSingleFolderArchive(bool value);
    bool isPasswordProtected() const;
    QString password() const;
    void setPassword(const QString &password);
    QString subfolderName() const;
    QString codePage() const;
    void setCodePage(const QString &cp);
    int entryCount() const;
    void setEntryCount(int count);
    QString createSubfolderName();

public slots:
    //void setCodePage(const QString &cp, const QString &desc);

signals:
    void codePageChanged(const QString &cp);

private slots:
    void onListFinished(QJob*);
    void onAddFinished(QJob *);

private:
    Q_DISABLE_COPY(Archive)

    QMimeType m_mimeType;
    QString m_fileName;
    QString m_password;
    QString m_codepage;
    QString m_subfolderName;
    ArchiveInterface *m_iface;
    bool m_hasBeenListed;
    bool m_isPasswordProtected;
    bool m_isSingleFolderArchive;
    int m_entryCount;
    qlonglong m_extractedFilesSize;
    qlonglong m_compressedFilesSize;
    QMutex mutex;
};

#endif // ARCHIVE_H
