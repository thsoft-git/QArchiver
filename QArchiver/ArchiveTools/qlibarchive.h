#ifndef QLIBARCHIVE_H
#define QLIBARCHIVE_H

#include "archiveinterface.h"

#include <QDir>
#include <QScopedPointer>

#define LIBARCHIVE_NO_RAR

class QMimeType;
class Archive;

class QLibArchive : public ArchiveInterface
{
    Q_OBJECT
public:
    explicit QLibArchive(const QString& mimeType, QObject *parent = 0);
    explicit QLibArchive(QObject *parent = 0);
    ~QLibArchive();

    enum LaOptions{
        AutoConvertFilenames = 0b00000001
    };

    static QString readOnlyMimeTypes();
    static QStringList supportedMimetypes(ArchiveOpenMode mode = ReadOnly);
    void setOptions(qint8 options);
    bool analyze(Archive *archive);
    bool setFormatByFilename(const QString &file_name);
    virtual bool isReadOnly() const;
    virtual bool open();
    virtual bool list();
    virtual bool copyFiles(const QVariantList& files, const QString& destinationDirectory, ExtractionOptions options);
    virtual bool addFiles(const QStringList& files, const CompressionOptions& options);
    virtual bool deleteFiles(const QVariantList& files);
    virtual bool test();
    int libArchiveFormatCode(const QMimeType &mimeType);

signals:
    
public slots:
    
private:
    struct ArchiveReadCustomDeleter;
    struct ArchiveWriteCustomDeleter;
    typedef QScopedPointer<struct archive, ArchiveReadCustomDeleter> ArchiveRead;
    typedef QScopedPointer<struct archive, ArchiveWriteCustomDeleter> ArchiveWrite;

    void emitEntryFromArchiveEntry(struct archive_entry *aentry);
    const QString relativeToLocalWD(const QString &fileName);
    QString absoluteFilePathLocalWD(const QString &relFilePath);
    int countFiles(const QStringList &files, QStringList *filesList);

    int extractionFlags() const;
    int copyData(struct archive *source, struct archive *dest, qint64 entry_size, bool partialprogress = true);
    bool fileToArchive(const QString& fileName, struct archive* arch_writer);
    QStringList archiveEntryList(struct archive *a_reader, const QString &fileName);

    QString responseName(int response); // debug

    int m_cachedArchiveEntryCount;
    int m_archFilesCount;
    bool m_emitNoEntries;
    qlonglong m_extractedFilesSize;
    qlonglong m_currentExtractedFilesSize;
    QDir m_workDir;
    QStringList m_writtenFiles;
    int m_archFormat;
    QString m_archFormatName;
    int m_archFilter;
    QString m_archFilterName;
    qint8 m_options;
};

#endif // QLIBARCHIVE_H
