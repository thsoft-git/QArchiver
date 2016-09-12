#include "singlefilecompression.h"

#include <archive.h>
#include <archive_entry.h>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QDebug>

#include "queries.h"

struct SingleFileCompression::ArchiveReadCustomDeleter
{
    static inline void cleanup(struct archive *a)
    {
        if (a) {
          #if ARCHIVE_VERSION_NUMBER < 4000000
            ::archive_read_finish(a); // libarchive < 4.0 API
          #else
            ::archive_read_free(a); // new libarchive >= 3.0 API
          #endif
        }
    }
};

struct SingleFileCompression::ArchiveWriteCustomDeleter
{
    static inline void cleanup(struct archive *a)
    {
        if (a) {
          #if ARCHIVE_VERSION_NUMBER < 4000000
            ::archive_write_finish(a); // libarchive < 4.0 API
          #else
            ::archive_write_free(a); // new libarchive >= 3.0 API
          #endif
        }
    }
};


SingleFileCompression::SingleFileCompression(QObject *parent) :
    ArchiveInterface(parent)
{
     m_archFilter = ARCHIVE_FILTER_NONE;
}

SingleFileCompression::SingleFileCompression(const QString &mimeType, QObject *parent) :
    ArchiveInterface(parent)
{
    if (mimeType.compare("application/x-compress") == 0)
    {
        /* ".Z" */
        m_archFilter = ARCHIVE_FILTER_COMPRESS;
    }
    else if (mimeType.compare("application/x-gzip") == 0) {
        /* ".gz" */
        m_archFilter = ARCHIVE_FILTER_GZIP;
    }
    else if (mimeType.compare("application/x-bzip") == 0) {
        /* ".bz, .bz2" */
        m_archFilter = ARCHIVE_FILTER_BZIP2;
    }
    else if (mimeType.compare("application/x-lzma") == 0) {
        /* ".lzma" */
        m_archFilter = ARCHIVE_FILTER_LZMA;
    }else if (mimeType.compare("application/x-xz") == 0) {
        /* ".xz" */
        m_archFilter = ARCHIVE_FILTER_XZ;
    }
    else {
        m_archFilter = ARCHIVE_FILTER_NONE;
    }
}

QStringList SingleFileCompression::supportedMimetypes(ArchiveOpenMode mode)
{
    QStringList mimeTypes;

    switch (mode) {
    case ReadOnly:
        mimeTypes << "application/x-compress"
                  << "application/x-gzip"
                  << "application/x-bzip"
                  << "application/x-lzma"
                  << "application/x-xz";
        break;
    case ReadWrite:
        mimeTypes = QStringList();
        break;
    }

    return mimeTypes;
}

bool SingleFileCompression::isReadOnly() const
{
    return true;
}

bool SingleFileCompression::open()
{
    ArchiveRead arch_reader(archive_read_new());
    if (!(arch_reader.data())) {
        return false;
    }

    if (archive_read_support_compression_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_support_format_raw(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(tr("<qt>Could not open the archive <i>%1</i>.</qt>", "@info").arg(filename()),
                   tr(archive_error_string( arch_reader.data() ), "@libArchiveError"));
        qDebug( archive_error_string(arch_reader.data()) );
        qDebug("%i: %s", archive_errno(arch_reader.data()), strerror(archive_errno(arch_reader.data())));
        return false;
    }

    m_extractedSize = 0;
    qint64 archiveSize = getArchive()->archiveFileSize();
    qint64 compressed = 0;
    qint64 uncompressed = 0;

    emit currentFileProgress(-2);
    emit currentFile(tr("Reading archive..."));

    char buff[10240];
    ssize_t readBytes;
    struct archive_entry *entry;

    while (archive_read_next_header(arch_reader.data(), &entry) == ARCHIVE_OK) {
        readBytes = archive_read_data(arch_reader.data(), buff, sizeof(buff));
        while (readBytes > 0) {
            m_extractedSize += readBytes;
            readBytes = archive_read_data(arch_reader.data(), buff, sizeof(buff));
            compressed = archive_filter_bytes(arch_reader.data(), -1);
            emit totalProgress(100 * compressed / archiveSize);
        }

        if (readBytes < 0) {
            qDebug() << "Error while reading archive data..." << readBytes;
            qDebug() << "Error" << archive_errno(arch_reader.data()) << ':' << archive_error_string(arch_reader.data());
            emit error(archive_error_string(arch_reader.data()));
        }
    }

    compressed = archive_filter_bytes(arch_reader.data(), -1);
    //qDebug() << compressed << getArchive()->archiveFileSize();
    uncompressed = archive_filter_bytes(arch_reader.data(), 0);
    //qDebug() << uncompressed << m_extractedSize;
    int entryCount = archive_file_count(arch_reader.data());
    Archive * arch_info = getArchive();
    arch_info->setExtractedSize(uncompressed);
    arch_info->setCompressedSize(compressed);
    arch_info->setEntryCount(entryCount);

    archive_read_close(arch_reader.data());
    arch_reader.reset(NULL);

    return true;
}

bool SingleFileCompression::list()
{
    ArchiveRead arch_reader(archive_read_new());
    if (!(arch_reader.data())) {
        return false;
    }

    if (archive_read_support_compression_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_support_format_raw(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(tr("<qt>Could not open the archive <i>%1</i>.</qt>", "@info").arg(filename()),
                   tr(archive_error_string( arch_reader.data() ), "@libArchiveError"));
        qDebug( archive_error_string(arch_reader.data()) );
        qDebug("%i: %s", archive_errno(arch_reader.data()), strerror(archive_errno(arch_reader.data())));
        return false;
    }

    struct archive_entry *entry;
    while (archive_read_next_header(arch_reader.data(), &entry) == ARCHIVE_OK) {
        archive_entry_set_pathname(entry, fileNameForData().toLocal8Bit());
        emit currentFile(tr("<qt>%1</qt>").arg(QString::fromLocal8Bit(archive_entry_pathname(entry))));
        archive_read_data_skip(arch_reader.data());

        emitEntry(entry);
    }

    archive_read_close(arch_reader.data());
    arch_reader.reset(NULL);

    return true;
}

bool SingleFileCompression::test()
{
    emit testResult(tr("Archive: %1\n\n").arg(filename()));

    ArchiveRead arch_reader(archive_read_new());
    if (!(arch_reader.data())) {
        return false;
    }

    if (archive_read_support_compression_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_support_format_raw(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(tr("Could not open the archive <i>%1</i>.", "@info").arg(filename()));
        qDebug("%i: %s", archive_errno( arch_reader.data() ), strerror(archive_errno( arch_reader.data() )));
        qDebug( archive_error_string( arch_reader.data() ) );
        return false;
    }

    emit totalProgress(0);
    qint64 procesedSize = 0;
    qint64 entry_size = m_extractedSize;

    struct archive_entry *entry;

    int error_count = 0;
    int result = 0;
    //while ((result = archive_read_next_header(arch_reader.data(), &entry)) == ARCHIVE_OK) {
    while ((result = archive_read_next_header(arch_reader.data(), &entry)) < ARCHIVE_EOF) {
        if (result == ARCHIVE_FATAL) {
            /* Krytické poškození archivu, další operace nejsou možné */
            break;
        }

        const QString pathname = fileNameForData();

        emit currentFile(pathname);
        emit currentFileProgress(0);
        emit testResult(QString("Testing     %1  ").arg(pathname, -30, QLatin1Char(' ')));

        // Zde načíst všechna data souboru
        int r;
        const void *buff;
        size_t size;
        off_t offset;

        size_t readed_size = 0;
        bool error = 0;

        for(;;) {
            r = archive_read_data_block(arch_reader.data(), &buff, &size, &offset);
            readed_size += size;
            procesedSize += size;
            emit currentFileProgress(100 * readed_size / entry_size);
            emit totalProgress(100 * procesedSize / m_extractedSize);
            qDebug() << "EntrySize:" << entry_size << "Size:" << size << "ReadedSize" << readed_size;

            if (r < ARCHIVE_OK) {
                error = 1;
                error_count++;
                emit testResult(QString("\n  Error: %1\n").arg(archive_error_string(arch_reader.data())));
            }

            if (r==ARCHIVE_EOF || r==ARCHIVE_FATAL) {
                break;
            }
        }

        emit testResult(error == 0 ? QString("OK\n") : QString("\n"));
    }

    if (result != ARCHIVE_EOF) {
        error_count++;
        emit testResult(QString("Error: %1").arg(archive_error_string(arch_reader.data())));
        emit error(tr("<qt>The archive reading failed with the following error: <i>%1</i></qt>", "@info")
                   .arg( archive_error_string(arch_reader.data()) ) );
    }

    bool ok = (archive_read_close(arch_reader.data()) == ARCHIVE_OK);

    emit testResult(error_count == 0 ?
                        QString("\nThere were no errors in the archive: %1\n").arg(filename()) :
                        QString("\nTotal errors: %1\n").arg(error_count));
    return ok;
}

bool SingleFileCompression::copyFiles(const QVariantList &files, const QString &destinationDirectory, ExtractionOptions options)
{
    Q_UNUSED(files);

    qDebug() << "Changing current directory to " << destinationDirectory;
    QDir::setCurrent(destinationDirectory);
    bool overwriteAll = options.value(QLatin1String( "AutoOverwrite" ), false).toBool();

    ArchiveRead arch_reader(archive_read_new());

    if (!(arch_reader.data())) {
        return false;
    }

    if (archive_read_support_compression_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_support_format_raw(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(tr("Could not open the archive <i>%1</i>", "@info").arg(filename()), archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    ArchiveWrite writer(archive_write_disk_new());
    if (!(writer.data())) {
        return false;
    }

    archive_write_disk_set_standard_lookup(writer.data());

//    struct stat st;
//    stat(QFile::encodeName(filename()).constData(), &st);

    emit totalProgress(0);

    struct archive_entry *entry;

    if (archive_read_next_header(arch_reader.data(), &entry) != ARCHIVE_OK) {
        // Error
        emit error(archive_error_string(writer.data()));
        return false;
    }

    QString entryName = fileNameForData();
    // entryFI is the fileinfo pointing to where the file will be written from the archive
    QFileInfo entryFI(entryName);
    // Now check if the file about to be written already exists
    if (entryFI.exists() && !overwriteAll) {

        OverwriteQuery query(entryName);
        emit userQuery(&query);
        query.waitForResponse();

        if (query.responseCancelled() || query.responseSkip() || query.responseAutoSkip()) {
            //archive_read_data_skip(arch_reader.data());
            archive_entry_clear(entry);
            return true;
        }
    }
    emit currentFile(entryName);

    archive_entry_copy_pathname(entry, entryName.toLocal8Bit());

    int header_response;
    if ((header_response = archive_write_header(writer.data(), entry)) < ARCHIVE_OK) {
        // Error
        emit error(archive_error_string(writer.data()));
        return false;
    }

    int data_response = copyData(arch_reader.data(), writer.data(), m_extractedSize);
    if (data_response != ARCHIVE_OK) {
        qDebug() << "Copy data failed.";
        return false;
    }
    emit totalProgress(100);

    bool reader_closed = (archive_read_close(arch_reader.data()) == ARCHIVE_OK);
    bool writer_closed = (archive_write_close(writer.data()) == ARCHIVE_OK);

    return reader_closed && writer_closed;
}

bool SingleFileCompression::addFiles(const QStringList &files, const CompressionOptions &options)
{
    Q_UNUSED(files);
    Q_UNUSED(options);

    // Libarchive can not write raw file
    emit error(tr("Unsupported operation."));
    return false;
}

bool SingleFileCompression::deleteFiles(const QList<QVariant> &files)
{
    Q_UNUSED(files);
    return false;
}

QString SingleFileCompression::fileNameForData() const
{
    Archive* arch = getArchive();
    QStringList suffixes = arch->mimeType().suffixes();

    for (int i = 0; i < suffixes.count(); ++i) {

        if (suffixes[i].at(0) != '.') {
            suffixes[i].prepend('.');
        }
    }

    QString uncompressedName(QFileInfo(arch->fileName()).fileName());

    foreach(const QString & suffix, suffixes) {
        qDebug() << suffix;

        if (uncompressedName.endsWith(suffix, Qt::CaseInsensitive)) {
            uncompressedName.chop(suffix.size());
            break;
        }
    }

    return uncompressedName;
}

void SingleFileCompression::emitEntry(archive_entry *aentry)
{
    ArchiveEntry e;

    // multibyte pathname in curent locale
    const char *pathname = archive_entry_pathname(aentry);

    e[FileName] = QString::fromLocal8Bit(pathname);

    e[InternalID] = e[FileName];

    e[Permissions] = QString::fromLocal8Bit(archive_entry_strmode(aentry));

    e[IsDirectory] = S_ISDIR(archive_entry_mode(aentry));
    //qDebug() << "Filename:" << e[FileName] << "IsDir:" << e[IsDirectory] << "FileType" << archive_entry_filetype(aentry) << S_IFREG << AE_IFREG;

    if (archive_entry_size_is_set(aentry)) {
        e[Size] = (qlonglong)archive_entry_size(aentry);
    }

    const QString owner = QString::fromLocal8Bit(archive_entry_uname(aentry));
    if (!owner.isEmpty()) {
        e[Owner] = owner;
    }

    const QString group = QString::fromLocal8Bit(archive_entry_gname(aentry));
    if (!group.isEmpty()) {
        e[Group] = group;
    }

    /* LibArchive supported time fields are
     *  atime (access time),
     *  birthtime (creation time),
     *  ctime (last time an inode property was changed) and
     *  mtime (modification time)
     */
    if (archive_entry_mtime_is_set(aentry)) {
        e[Timestamp] = QDateTime::fromTime_t(archive_entry_mtime(aentry));
    }


    emit entry(e);
}

int SingleFileCompression::copyDataBlock(struct archive *areader, struct archive *awriter)
{
  int r;
  const void *buff;
  size_t size;
  off_t offset;

  for (;;) {
    r = archive_read_data_block(areader, &buff, &size, &offset);
    if (r == ARCHIVE_EOF)
      return (ARCHIVE_OK);
    if (r < ARCHIVE_OK)
      return (r);
    r = archive_write_data_block(awriter, buff, size, offset);
    if (r < ARCHIVE_OK) {
      fprintf(stderr, "%s\n", archive_error_string(awriter));
      return (r);
    }
  }
}

int SingleFileCompression::copyData(struct archive *source, struct archive *dest, qint64 entry_size)
{
    char buff[10240];
    ssize_t readBytes;
    qint64 writen_size = 0;

    emit currentFileProgress(0);

    readBytes = archive_read_data(source, buff, sizeof(buff));
    while (readBytes > 0) {
        /* int writeBytes = */
        archive_write_data(dest, buff, readBytes);
        if (archive_errno(dest) != ARCHIVE_OK) {
            qDebug() << "Error while extracting..." << "Error" << archive_errno(dest) << ':' << archive_error_string(dest);
            emit error(archive_error_string(dest));
            return -1;
        }

        if (entry_size > 0) {
            writen_size += readBytes;
            emit currentFileProgress(100 * writen_size / entry_size);
            emit totalProgress(100 * writen_size / entry_size);
        }

        readBytes = archive_read_data(source, buff, sizeof(buff));
    }

    emit currentFileProgress(100);

    if (readBytes < 0) {
        qDebug() << "Error" << archive_errno(source) << ':' << archive_error_string(source);
        emit error(archive_error_string(source), "");
        return readBytes;
    }

    return ARCHIVE_OK;
}
