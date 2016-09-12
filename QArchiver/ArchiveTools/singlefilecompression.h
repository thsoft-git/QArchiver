#ifndef SINGLEFILECOMPRESSION_H
#define SINGLEFILECOMPRESSION_H

#include "archiveinterface.h"

class SingleFileCompression : public ArchiveInterface
{
    Q_OBJECT
public:
    explicit SingleFileCompression(QObject *parent = 0);
    explicit SingleFileCompression(const QString& mimeType, QObject *parent = 0);
    static QStringList supportedMimetypes(ArchiveOpenMode mode = ReadOnly);
    virtual bool isReadOnly() const;
    virtual bool open();
    virtual bool list();
    virtual bool test();
    virtual bool copyFiles(const QVariantList& files, const QString& destinationDirectory, ExtractionOptions options);
    virtual bool addFiles(const QStringList & files, const CompressionOptions& options);
    virtual bool deleteFiles(const QList<QVariant> & files);
    
signals:
    
public slots:
    
private:
    struct ArchiveReadCustomDeleter;
    struct ArchiveWriteCustomDeleter;
    typedef QScopedPointer<struct archive, ArchiveReadCustomDeleter> ArchiveRead;
    typedef QScopedPointer<struct archive, ArchiveWriteCustomDeleter> ArchiveWrite;

    QString fileNameForData() const;
    void emitEntry(struct archive_entry *aentry);
    int copyDataBlock(struct archive *areader, struct archive *awriter);
    int copyData(struct archive *source, struct archive *dest, qint64 entry_size);

    int m_archFilter;
    QString m_archFilterName;
    qlonglong m_compressedSize;
    qlonglong m_extractedSize;
    qlonglong m_currentExtractedFilesSize;
};

#endif // SINGLEFILECOMPRESSION_H
