#include "qlibarchive.h"

#include <sys/types.h>
#include <sys/stat.h> //lstat
#include <unistd.h>
#include <fcntl.h>

#include <archive.h>
#include <archive_entry.h>
#include <errno.h>

#include "qarchive.h"
#include "queries.h"
#include "Codecs/textencoder.h"
#include "Codecs/qenca.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QtMimeTypes/QMimeDatabase>


#ifdef _MSC_BUILD
  #define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR) // MSVC stat.h nema definovane
#endif

struct QLibArchive::ArchiveReadCustomDeleter
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

struct QLibArchive::ArchiveWriteCustomDeleter
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

//const QString QLibArchive::c_mimeType[] =  {"application/zip", "application/x-java-archive",
//                                            "application/x-tar", "application/x-compressed-tar",
//                                            "application/x-bzip-compressed-tar", "application/x-xz-compressed-tar"};
//const int QLibArchive::c_archiveFormat[] = {ARCHIVE_FORMAT_ZIP, ARCHIVE_FORMAT_ZIP,
//                                            ARCHIVE_FORMAT_TAR, ARCHIVE_FORMAT_TAR,
//                                            ARCHIVE_FORMAT_TAR, ARCHIVE_FORMAT_TAR};

/*
 * --ignore-zeros
 *   An bsdtar alias of --options read_concatenated_archives for compatibility with GNU tar.
 */



QLibArchive::QLibArchive(const QString &mimeType, QObject *parent) :
    ArchiveInterface(parent),
    m_cachedArchiveEntryCount(0),
    m_emitNoEntries(false),
    m_extractedFilesSize(0),
    m_workDir(QDir::current()),
    m_archFormat(0),
    m_archFilter(0)
{
    m_archFilterName = QString();
    m_archFormatName = QString();

    if (mimeType.compare("application/x-rar") == 0)
    {
        /* .rar */
        m_archFormat = ARCHIVE_FORMAT_RAR;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (mimeType.compare("application/x-7z-compressed") == 0)
    {
        /* .7z */
        m_archFormat = ARCHIVE_FORMAT_7ZIP;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (mimeType.compare("application/zip") == 0)
    {
        /* .zip */
        m_archFormat = ARCHIVE_FORMAT_ZIP;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (mimeType.compare("application/x-java-archive") == 0)
    {
        /* .jav */
        m_archFormat = ARCHIVE_FORMAT_ZIP;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (mimeType.compare("application/x-cpio") == 0)
    {
        /* .cpio */
        qDebug() << "Detected cpio archive";
        m_archFormat = ARCHIVE_FORMAT_CPIO_POSIX;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (mimeType.compare("application/x-cpio-compressed") == 0)
    {
        /* .cpio.gz */
        qDebug() << "Detected cpio.gz archive";
        m_archFormat = ARCHIVE_FORMAT_CPIO_POSIX;
        m_archFilter = ARCHIVE_COMPRESSION_GZIP;
    }
    else if (mimeType.compare("application/x-sv4cpio") == 0 )
    {
        /* .sv4cpio */
        qDebug() << "Detected sv4cpio";
        m_archFormat = ARCHIVE_FORMAT_CPIO_SVR4_NOCRC;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (mimeType.compare("application/x-bcpio") == 0)
    {
        /* bcpio */
        qDebug() << "Detected bcpio";
        m_archFormat = ARCHIVE_FORMAT_CPIO;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (mimeType.compare("application/x-sv4crc") == 0) {
        /* sv4crc */
        qDebug() << "Detected sv4crc";
        m_archFormat = ARCHIVE_FORMAT_CPIO;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (mimeType.compare("application/x-archive") == 0
             || mimeType.compare("application/x-ar") == 0)
    {
        /*(Ar) .a */
        qDebug() << "Detected AR archive";
        m_archFormat = ARCHIVE_FORMAT_AR_BSD;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (mimeType.compare("application/x-tar") == 0)
    {
        /* .tar */
        qDebug() << "Detected uncompressed TAR format.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (mimeType.compare("application/x-tarz") == 0) {
        /* .tar.Z */
        qDebug() << "Detected COMPRESS compressed TAR archive.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_COMPRESS;
    }
    else if (mimeType.compare("application/x-compressed-tar") == 0) {
        /* .tar.gz */
        qDebug() << "Detected gzip compressed TAR archive.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_GZIP;
    }
    else if (mimeType.compare("application/x-bzip-compressed-tar")==0) {
        /* .tar.bz .tar.bz2 */
        qDebug() << "Detected bzip2 compressed TAR archive.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_BZIP2;
    }
    else if (mimeType.compare("application/x-xz-compressed-tar")==0) {
        /* .tar.xz */
        qDebug() << "Detected xz compressed TAR archive.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_XZ;
    }
    else if (mimeType.compare("application/x-lzma-compressed-tar")==0) {
        /* .tar.lzma */
        qDebug() << "Detected lzma compressed TAR archive.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_LZMA;
    }
    else if (mimeType.compare("application/x-rpm")==0) {
        /* .rpm */
        m_archFormat = ARCHIVE_FORMAT_CPIO;
        m_archFilter = ARCHIVE_COMPRESSION_RPM;
    }
    else if (mimeType.compare("application/x-deb")==0) {
        /* .deb */
        m_archFormat = ARCHIVE_FORMAT_AR;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (mimeType.compare("application/x-lha")==0) {
        /* .lha */
        m_archFormat = ARCHIVE_FORMAT_LHA;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
}


QLibArchive::QLibArchive(QObject *parent) :
    ArchiveInterface(parent),
    m_cachedArchiveEntryCount(0),
    m_emitNoEntries(false),
    m_extractedFilesSize(0),
    m_workDir(QDir::current()),
    m_archFormat(0),
    m_archFilter(0)
{
    ;
}

QLibArchive::~QLibArchive()
{
}


QString QLibArchive::readOnlyMimeTypes()
{
    /* ================ ReadOnly MimeTypes ================ */
    QString readonly_mimetypes;

    /* .jar */
    readonly_mimetypes += "application/x-java-archive;";

    /* .rpm */
    readonly_mimetypes += "application/x-rpm;application/x-source-rpm;";

    /* .deb */
    readonly_mimetypes += "application/x-deb;";

    /* bcpio, sv4crc */
    readonly_mimetypes += "application/x-bcpio;application/x-sv4crc;";

    /* .xar (read only, starting in libarchive 2.8) ARCHIVE_FORMAT_XAR */
    /*      QMimeDb hlásí "application/octet-stream"                   */
    //readonly_mimetypes += "application/x-xar;";

    /* .lha/.lzh (read only, starting in libarchive 3.0) */
    readonly_mimetypes += "application/x-lha;";

    /* .cab Microsoft CAB format (read only, starting in libarchive 3.0) */
    //readonly_mimetypes += "application/vnd.ms-cab-compressed;";

    /* .rar (read only, starting in libarchive 3.0) */
    readonly_mimetypes += "application/x-rar;";

    /* .7z 7-Zip (read only, starting in libarchive 3.0) */
    readonly_mimetypes += "application/x-7z-compressed;";

    return readonly_mimetypes;
}

QStringList QLibArchive::supportedMimetypes(ArchiveOpenMode mode)
{
    /* ================ ReadWrite MimeTypes ================ */
    /* .tar; .tar.XX */
    QString readwrite_mimetypes = "application/x-tar;application/x-tarz;application/x-compressed-tar;application/x-bzip-compressed-tar;application/x-xz-compressed-tar;application/x-lzma-compressed-tar;";

    /* .cpio  (odc), .cpio.gz (odc),  .sv4cpio (newc) */
    readwrite_mimetypes += "application/x-cpio;application/x-cpio-compressed;application/x-sv4cpio;";

    /* ar (.a) */
    readwrite_mimetypes += "application/x-archive;application/x-ar;";

    /* .zip */
    readwrite_mimetypes += "application/zip;";


    QString readonly_mimetypes = readOnlyMimeTypes();

    QStringList supported;
    supported << readwrite_mimetypes.split(';', QString::SkipEmptyParts);

    if (mode == ReadOnly) {
        supported << readonly_mimetypes.split(';', QString::SkipEmptyParts);
    }

    return supported;
}

void QLibArchive::setOptions(qint8 options)
{
    m_options = options;
}

bool QLibArchive::analyze(Archive *archive)
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

    if (archive_read_support_format_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    /* XAR Debug => jinak není potřeba volat, když archive_read_support_format_all */
    /* Mageia 2 - libarchive 3.0.3
     * vrací error string: XAR is not supported on this platform
    if (archive_read_support_format_xar(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }
    */

    //if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(archive->fileName()), 10240) != ARCHIVE_OK) {
        //emit error(tr("<qt>Could not open the archive <i>%1</i>.</qt>", "@info").arg(filename()));
        emit error(tr("<qt>Could not open the archive <i>%1</i>.</qt>", "@info").arg( archive->fileName() ),
                   tr(archive_error_string( arch_reader.data() ), "@libArchiveError") );
        qDebug("%i: %s", archive_errno( arch_reader.data() ), strerror(archive_errno( arch_reader.data() )));
        qDebug( archive_error_string( arch_reader.data() ) );
        return false;
    }

    emit currentFileProgress(-2);

    m_cachedArchiveEntryCount = 0;
    m_extractedFilesSize = 0;
    qint64 compressedRead = 0;
    qint64 archiveSize = getArchive()->archiveFileSize();

    int archFormatPrev = 0;
    struct archive_entry *entry;
    bool analyze_encoding = false;
    bool pax_restricted = false;
    int result;
    QByteArray ba;

    //while ((result = archive_read_next_header(arch_reader.data(), &entry)) == ARCHIVE_OK) {
    while ((result = archive_read_next_header(arch_reader.data(), &entry)) < ARCHIVE_EOF) {
        if (result == ARCHIVE_FATAL) {
            /* Krytické poškození archivu, další operace nejsou možné */
            break;
        }
        qDebug();
        const char* pathname = archive_entry_pathname(entry);
        const wchar_t* pathname_w = archive_entry_pathname_w(entry);
        if (pathname_w == NULL && pathname != NULL) {
            analyze_encoding = true;
            qDebug() << "QLibArchive:" << "Filenames in unknown encoding. Enca analyzer enabled.";
        }
        ba.append(pathname);
        emit currentFile(tr("<qt>Analysing entry:<br/>%1</qt>").arg(QFile::decodeName(archive_entry_pathname(entry))));

        m_extractedFilesSize += (qlonglong)archive_entry_size(entry);

        qDebug() << "Pathname" << pathname;
        qDebug() << "Size" << archive_entry_size(entry);
        qDebug() << "Filetype" << archive_entry_filetype(entry);
        qDebug() << "Mode" << archive_entry_mode(entry) << "IsDir" << S_ISDIR(archive_entry_mode(entry)) << "IsFile:" << S_ISREG(archive_entry_mode(entry));

        if (QString::fromLocal8Bit(pathname).endsWith('/') && (archive_entry_size(entry) == 0)) {
            archive_entry_set_filetype(entry, AE_IFDIR);
            qDebug() << "QLibArchive:" << "Entry seems like directory, correcting filetype flag...";
            qDebug() << "Mode" << archive_entry_mode(entry) << "IsDir" << S_ISDIR(archive_entry_mode(entry)) << "IsFile:" << S_ISREG(archive_entry_mode(entry));
        }


        /* archive_format_name()
         *  -- vrací "ZIP 2.0 (deflation)", "ZIP 1.0 (uncompressed)"
         *     verze a komprese závisí na aktualně čtené položce archivu
         *  -- ve vývojové verzi na githubu už jen "ZIP"
         *  =>> vytažení verze a komprese asi nemá cenu řešit (v příští verzi libArchive bude jinak)
         */
        m_archFormat = archive_format(arch_reader.data());
        m_archFormatName = QString::fromLocal8Bit(archive_format_name(arch_reader.data()));
        qDebug() << "ArchiveFormat:" << m_archFormat  << "Name:" << m_archFormatName;
        if (archFormatPrev == ARCHIVE_FORMAT_TAR_USTAR && m_archFormat == ARCHIVE_FORMAT_TAR_PAX_INTERCHANGE ) {
            pax_restricted = true;
            qDebug("QLibArchive: Detected archive format \"TAR_PAX_RESTRICTED\".");
        }

        m_archFilter = archive_compression(arch_reader.data());
        m_archFilterName = QString::fromLocal8Bit(archive_compression_name(arch_reader.data()));
        qDebug() << "Komprese:" << m_archFilter << "Name:" << m_archFilterName;

        m_archFilesCount = archive_file_count(arch_reader.data());
        m_cachedArchiveEntryCount++;
        //qDebug() << "ArchiveFileCount:" << m_archFilesCount << "m_cachedArchiveEntryCount:" << m_cachedArchiveEntryCount;
        archive_read_data_skip(arch_reader.data());
        compressedRead = archive_filter_bytes(arch_reader.data(), -1);
        emit totalProgress(100 * compressedRead / archiveSize);
        archFormatPrev = m_archFormat;
    }

    if (result != ARCHIVE_EOF) {
        emit error(tr("The archive reading failed with the following error: <message>%1</message>", "@info").arg(
                       archive_error_string(arch_reader.data())));
        return false;
    }
    m_archFilesCount = archive_file_count(arch_reader.data());

    archive->setEntryCount(m_archFilesCount);
    archive->setExtractedSize(m_extractedFilesSize);

    m_archFormat = archive_format(arch_reader.data());

    if (m_archFormat == ARCHIVE_FORMAT_TAR_PAX_INTERCHANGE && pax_restricted) {
        // LibArchive automaticky nerozpozná TAR_PAX_RESTRICTED
        // Pokud nekterá položka TAR_USTAR a nasledujici TAR_PAX_INTERCHANGE
        // => archiv je TAR_PAX_RESTRICTED
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFormatName = "Restricted POSIX pax interchange";
    }

    bool ok = (archive_read_close(arch_reader.data()) == ARCHIVE_OK);

    analyze_encoding = true;
    if (analyze_encoding) {
        emit currentFile(tr("Detecting charset..."));
        QEnca enca;
        if (enca.analyse(ba)) {
            qDebug() << "Charset HUMAN:" << enca.charsetName(ENCA_NAME_STYLE_HUMAN);
            qDebug() << "Charset ENCA:" << enca.charsetName(ENCA_NAME_STYLE_ENCA);
            qDebug() << "Charset ICONV:" << enca.charsetName(ENCA_NAME_STYLE_ICONV);
            qDebug() << "Charset MIME:" << enca.charsetName(ENCA_NAME_STYLE_MIME);
            qDebug() << "Charset RFC1345:" << enca.charsetName(ENCA_NAME_STYLE_RFC1345);
            qDebug() << "Charset CSTOCS:" << enca.charsetName(ENCA_NAME_STYLE_CSTOCS);

            QByteArray archive_charset(enca.charsetName(ENCA_NAME_STYLE_ICONV));
            QByteArray system_charset(enca.systemCharset());
            if ( (archive_charset != "ASCII") && (archive_charset != system_charset) )
            {
                emit charset(enca.charsetName(ENCA_NAME_STYLE_ICONV), enca.charsetName(ENCA_NAME_STYLE_HUMAN));
                emit encodingInfo(tr("Filename encoding is not in current locale codepage! QArchiver detected: %1. "
                                     "If it is not valid encoding of archive, please select correct encoding from menu.")
                                  .arg(QString::fromLocal8Bit(enca.charsetName(ENCA_NAME_STYLE_HUMAN))));

                archive->setCodePage(QString::fromLocal8Bit(enca.charsetName(ENCA_NAME_STYLE_ICONV)));
            } else {
                archive->setCodePage("");
            }
        }else {
            emit encodingInfo(tr("Filename encoding is not in current locale codepage!"
                                 " QArchiver can't detect the encoding: %1.").arg(enca.strError()));
        }
    }

    //qDebug() << "ArchiveFormatBase" << (m_archFormat & ARCHIVE_FORMAT_BASE_MASK) << ARCHIVE_FORMAT_TAR;
    return ok;
}

bool QLibArchive::setFormatByFilename(const QString &file_name)
{
    m_archFilterName = QString();
    m_archFormatName = QString();

    if (file_name.right(3).toUpper() == "ZIP") {
        qDebug() << "Detected ZIP format for new file";
        m_archFormat = ARCHIVE_FORMAT_ZIP;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    if (file_name.right(3).toUpper() == "LHA") {
        qDebug() << "Detected LHA format for new file";
        m_archFormat = ARCHIVE_FORMAT_LHA;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
        emit error("Libarchive can not write archive in LHA format.");
        return false;
    }
    else if (file_name.right(2).toLower() == ".a") {
        qDebug() << "Detected AR archive";
        m_archFormat = ARCHIVE_FORMAT_AR_BSD;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (file_name.right(5).toLower() == ".cpio") {
        qDebug() << "Detected cpio archive";
        m_archFormat = ARCHIVE_FORMAT_CPIO_POSIX;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (file_name.right(5+3).toLower() == ".cpio.gz") {
        qDebug() << "Detected cpio.gz archive";
        m_archFormat = ARCHIVE_FORMAT_CPIO_POSIX;
        m_archFilter = ARCHIVE_COMPRESSION_GZIP;
    }
    else if (file_name.right(8).toLower() == ".sv4cpio") {
        qDebug() << "Detected sv4cpio";
        m_archFormat = ARCHIVE_FORMAT_CPIO_SVR4_NOCRC;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (file_name.right(3).toUpper() == "TAR") {
        qDebug() << "Detected uncompressed TAR format for new file.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_NONE;
    }
    else if (file_name.right(3+2).toUpper() == "TAR.Z") {
        /* Pozor ARK vytváří tar.Z chybně s kompresí gzip!!! */
        qDebug() << "Detected COMPRESS compressed TAR archive.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_COMPRESS;
    }
    else if (file_name.right(3+3).toUpper() == "TAR.GZ") {
        qDebug() << "Detected gzip compressed TAR archive.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_GZIP;
    }
    else if (file_name.right(3+4).toUpper() == "TAR.BZ2") {
        qDebug() << "Detected bzip2 compressed TAR archive.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_BZIP2;
    }
    else if (file_name.right(3+3).toUpper() == "TAR.XZ") {
        qDebug() << "Detected xz compressed TAR archive.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_XZ;
    }
    else if (file_name.right(3+5).toUpper() == "TAR.LZMA") {
        qDebug() << "Detected lzma compressed TAR archive.";
        m_archFormat = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
        m_archFilter = ARCHIVE_COMPRESSION_LZMA;
    }
    else {
        emit error("Unable to determine the type of archive.");
        return false;
    }
    return true;
}

bool QLibArchive::open()
{
    if (m_archive->exists()) {
        return analyze(m_archive);
    } else {
        return false;
    }
}

bool QLibArchive::list()
{
    qDebug() <<  "QLibArchive::list()";
    bool encoding_warn_emited = false;
    QString codepage = getArchive()->codePage();

    ArchiveRead arch_reader(archive_read_new());
    if (!(arch_reader.data())) {
        return false;
    }

    if (archive_read_support_compression_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_support_format_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (!codepage.isEmpty()) {
        QByteArray options("hdrcharset=");
        options += codepage.toLocal8Bit();

        if (archive_read_set_options(arch_reader.data(), options /*"hdrcharset=CP852"*/) != ARCHIVE_OK) {
            emit error(archive_error_string(arch_reader.data()));
            qDebug("Error %i: %s", archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
            qDebug("This system cannot convert character-set for %s.", options.constData());
            return false;
        }
    }

    bool ignore_zeros = false;
    if (ignore_zeros) {
        archive_read_set_option(arch_reader.data(),NULL, "read_concatenated_archives", "1");
    }

    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(tr("<qt>Could not open the archive <i>%1</i>.</qt>", "@info").arg(filename()),
                   tr(archive_error_string( arch_reader.data() ), "@libArchiveError"));
        qDebug( archive_error_string(arch_reader.data()) );
        qDebug("%i: %s", archive_errno(arch_reader.data()), strerror(archive_errno(arch_reader.data())));
        return false;
    }

    m_cachedArchiveEntryCount = 0;
    m_extractedFilesSize = 0;
    int totalCount = getArchive()->entryCount();
    emit totalProgress(0);

    struct archive_entry *entry;
    int result;

    // ZIP: formát není znám dokud není proveden alespoň jeden archive_read_next_header()
    // tar.gz  ==||==

    qDebug() << "Formát:" << archive_format(arch_reader.data()) << "-" << archive_format_name(arch_reader.data());
    qDebug() << "Komprese:" << archive_compression(arch_reader.data()) << "-" << archive_compression_name(arch_reader.data());

    // V případě že do Zipu s názvy CP852 vložíme soubor s názvem v UTF-8 libarchive
    // archive_read_next_header() vrací ARCHIVE_WARN u souborů s cz názvy
    // tyto položky pak nejsou čitelné (pathname_w i pathname jsou NULL)
    // ErorString: Pathname nelze převést z UTF-8 do curent locale

    // Zdá se že "hdrcharset=CP852" "hdrcharset=codepage" to vyřeší!!!!!!!!

    //while ((result = archive_read_next_header(arch_reader.data(), &entry)) == ARCHIVE_OK) {
    while ((result = archive_read_next_header(arch_reader.data(), &entry)) < ARCHIVE_EOF) {
        if (result < ARCHIVE_WARN) {
            break;
        }

        const char* pathname = archive_entry_pathname(entry);
        const wchar_t* pathname_w = archive_entry_pathname_w(entry);
        if (pathname_w == NULL && pathname != NULL) {
            qDebug() << "QLibArchive::list" << "Filenames in unknown encoding";
            if (!encoding_warn_emited && codepage.isEmpty()) {
                // Encodong info
                encoding_warn_emited = true;
                //emit info(tr("Filename encoding is not in current locale codepage! Please select valid encoding from menu."));
            }
        }
        emit currentFile(tr("<qt>%1</qt>").arg(QFile::decodeName(archive_entry_pathname(entry))));

        if (QString::fromLocal8Bit(pathname).endsWith('/') && (archive_entry_size(entry) == 0)) {
            archive_entry_set_filetype(entry, AE_IFDIR);
            qDebug() << "Entry seems like directory, correcting filetype flag...";
            qDebug() << "Mode" << archive_entry_mode(entry) << S_ISDIR(archive_entry_mode(entry));
        }

        if (!m_emitNoEntries) {
            emitEntryFromArchiveEntry(entry);
        }

        m_extractedFilesSize += (qlonglong)archive_entry_size(entry);

        m_cachedArchiveEntryCount++;
        archive_read_data_skip(arch_reader.data());
        emit totalProgress(100 * m_cachedArchiveEntryCount / totalCount);
    }

    if (result != ARCHIVE_EOF) {
        emit error(tr("<qt>The archive reading failed with the following error: <message>%1</message></qt>", "@info").arg(
                       archive_error_string(arch_reader.data())));
        return false;
    }

//    qint64 compressed1 = archive_position_compressed(arch_reader.data());
//    const char* fn1 = archive_filter_name(arch_reader.data(), -1);
//    const char* fn0 = archive_filter_name(arch_reader.data(), 0);
    qint64 compressed = archive_filter_bytes(arch_reader.data(), -1);
    qint64 uncompressed = archive_filter_bytes(arch_reader.data(), 0);
    Archive * arch_info = getArchive();
    arch_info->setExtractedSize(qMax(uncompressed, m_extractedFilesSize));
    arch_info->setCompressedSize(compressed);
    arch_info->setEntryCount(m_cachedArchiveEntryCount);

    return archive_read_close(arch_reader.data()) == ARCHIVE_OK;
}

QString QLibArchive::responseName(int response)
{
    QString rstr;
    switch (response) {
    case ARCHIVE_EOF:
        rstr = "ARCHIVE_EOF";
        break;
    case ARCHIVE_OK:
        rstr = "ARCHIVE_OK";
        break;
    case ARCHIVE_RETRY:
        rstr = "ARCHIVE_RETRY";
        break;
    case ARCHIVE_WARN:
        rstr = "ARCHIVE_WARN";
        break;
    case ARCHIVE_FAILED:
        rstr = "ARCHIVE_FAILED";
        break;
    case ARCHIVE_FATAL:
        rstr = "ARCHIVE_FATAL";
        break;
    default:
        rstr = "??";
        break;
    }

    return rstr;
}

int QLibArchive::libArchiveFormatCode(const QMimeType &mimeType)
{
    if ( mimeType.name() == QLatin1String("application/x-tar")                    /* .tar */
         || mimeType.name() == QLatin1String("application/x-compressed-tar")      /* .tar.gz */
         || mimeType.name() == QLatin1String("application/x-bzip-compressed-tar") /* .tar.bz .tar.bz2 */
         || mimeType.name() == QLatin1String("application/x-xz-compressed-tar")   /* .tar.xz */
         || mimeType.name() == QLatin1String("application/x-lzma-compressed-tar") /* .tar.lzma */
         || mimeType.name() == QLatin1String("application/x-tarz")                /* .tar.Z */
         )
    {
        return ARCHIVE_FORMAT_TAR;
    }
    else if ( mimeType.name() == QLatin1String("application/zip")
              || mimeType.name() == "application/x-java-archive") {
        return ARCHIVE_FORMAT_ZIP;
    }
    else if (mimeType.name() == "application/x-cpio"
             || mimeType.name() ==  "application/x-cpio-compressed")
    {
        return ARCHIVE_FORMAT_CPIO_POSIX;
    }
    else if (mimeType.name() ==  "application/x-sv4cpio")
    {
        return ARCHIVE_FORMAT_CPIO_SVR4_NOCRC;
    }
    else if (mimeType.name() ==  "application/x-sv4crc") {
        return ARCHIVE_FORMAT_CPIO_SVR4_CRC;
    }
    else if (mimeType.name() =="application/x-ar"
             || mimeType.name() == "application/x-archive") {
        return ARCHIVE_FORMAT_AR;
    }
    else if (mimeType.name() =="application/x-lha") {
        return ARCHIVE_FORMAT_LHA;
    }
    else
    {
        return -1;
    }
}

bool QLibArchive::copyFiles(const QVariantList &files, const QString &destinationDirectory, ExtractionOptions options)
{
    qDebug() << "Changing current directory to " << destinationDirectory;
    QDir::setCurrent(destinationDirectory);

    const QString codepage = getArchive()->codePage();
    qDebug() << "Extracting filenames with codepage " << codepage;

    const bool extractAll = files.isEmpty();
    const bool preservePaths = options.value(QLatin1String( "PreservePaths" )).toBool();

    QString rootNode = options.value(QLatin1String("RootNode"), QVariant()).toString();
    if ((!rootNode.isEmpty()) && (!rootNode.endsWith(QLatin1Char('/')))) {
        rootNode.append(QLatin1Char('/'));
    }

    ArchiveRead arch_reader(archive_read_new());

    if (!(arch_reader.data())) {
        return false;
    }

    if (archive_read_support_compression_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_support_format_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    if (!codepage.isEmpty()) {
        QByteArray options("hdrcharset=");
        options += codepage.toLocal8Bit();

        if (archive_read_set_options(arch_reader.data(), options /*"hdrcharset=CP852"*/) != ARCHIVE_OK) {
            emit error(archive_error_string(arch_reader.data()));
            qDebug("Error %i: %s", archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
            qDebug("This system cannot convert character-set for %s.",options.constData());
            return false;
        }
    }

    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(tr("Could not open the archive <i>%1</i>, libarchive can't handle it.", "@info").arg(filename()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    ArchiveWrite writer(archive_write_disk_new());
    if (!(writer.data())) {
        return false;
    }

    archive_write_disk_set_options(writer.data(), extractionFlags());

    int entryNr = 0;
    int totalCount = 0;

    if (extractAll) {
        if (!m_cachedArchiveEntryCount) {
            emit totalProgress(0);
            qDebug() << "For getting progress information, the archive will be listed once";
            m_emitNoEntries = true;
            list();
            m_emitNoEntries = false;
        }
        totalCount = m_cachedArchiveEntryCount;
    } else {
        totalCount = files.size();
    }

    m_currentExtractedFilesSize = 0;

    //bool overwriteAll = false; // Whether to overwrite all files
    bool overwriteAll = options.value(QLatin1String( "AutoOverwrite" ), false).toBool(); // Whether to overwrite all files
    bool skipAll = false; // Whether to skip all files
    struct archive_entry *entry;

    QString fileBeingRenamed;

    while (archive_read_next_header(arch_reader.data(), &entry) == ARCHIVE_OK) {
        fileBeingRenamed.clear();

        // retry with renamed entry, fire an overwrite query again
        // if the new entry also exists
    retry:
        /*const*/ bool entryIsDir = S_ISDIR(archive_entry_mode(entry));//U Zipů z OS win vrací false i když položka je adresář!!!!!
        //mode_t mode  = archive_entry_filetype(entry);
        //qDebug() << "Mode:" << mode;
        //const char *strmode = archive_entry_strmode(entry); //returns a string representation of the permission as used by the long mode of ls(1).
        //const char *fflags_text = archive_entry_fflags_text(entry);
        //qDebug() << "Mode string:" << strmode << "FFlags:" << fflags_text;
    retry_skip_dirs:
        //we skip directories if not preserving paths
        if (!preservePaths && entryIsDir) {
            archive_read_data_skip(arch_reader.data());
            continue;
        }

        //entryName is the name inside the archive, full path
        QString entryName = QDir::fromNativeSeparators(QFile::decodeName(archive_entry_pathname(entry)));
        qDebug() << entryName;
//        if (changeEncoding) {
//            entryName = decodeName(archive_entry_pathname(entry), codepage);
//            archive_entry_copy_pathname(entry, QFile::encodeName(entryName).constData());
//            qDebug() << "Converting filename:" << entryName;
//        }


        if (entryName.endsWith('/') && !entryIsDir /*&& (archive_entry_size(entry) == 0)*/) {
            qDebug("Warning: %s je možná adresář, ale dále je zpracováván jako soubor!", entryName.toLocal8Bit().constData());
            //ToDo: Zjistit proč S_ISDIR() na zipech z win nefunguje, ...atribut se tam neukládá????
            if (archive_format(arch_reader.data()) == ARCHIVE_FORMAT_ZIP) {
                qDebug("Info: Archive format is ZIP, try handle this entry as dir.");
                entryIsDir = true;
                archive_entry_set_filetype(entry, AE_IFDIR);
                goto retry_skip_dirs;
            }
        }

        if (entryName.startsWith(QLatin1Char( '/' ))) {
            //for now we just can't handle absolute filenames in a tar archive.
            //TODO: find out what to do here!!
            emit error(tr("This archive contains archive entries with absolute paths, which are not yet supported by QArchiver."));

            return false;
        }

        if (files.contains(entryName) || entryName == fileBeingRenamed || extractAll) {
            // entryFI is the fileinfo pointing to where the file will be
            // written from the archive
            QFileInfo entryFI(entryName);
            //qDebug() << "setting path to " << archive_entry_pathname( entry );

            const QString fileWithoutPath(entryFI.fileName());

            //if we DON'T preserve paths, we cut the path and set the entryFI
            //fileinfo to the one without the path
            if (!preservePaths) {
                //empty filenames (ie dirs) should have been skipped already,
                //so asserting
                Q_ASSERT(!fileWithoutPath.isEmpty());

                archive_entry_copy_pathname(entry, QFile::encodeName(fileWithoutPath).constData());
                entryFI = QFileInfo(fileWithoutPath);

                //OR, if the commonBase has been set, then we remove this
                //common base from the filename
            } else if (!rootNode.isEmpty()) {
                qDebug() << "Removing" << rootNode << "from" << entryName;

                const QString truncatedFilename(entryName.remove(0, rootNode.size()));
                archive_entry_copy_pathname(entry, QFile::encodeName(truncatedFilename).constData());

                entryFI = QFileInfo(truncatedFilename);
            }

            // Now check if the file about to be written already exists
            if (!entryIsDir && entryFI.exists()) {
                if (skipAll) {
                    archive_read_data_skip(arch_reader.data());
                    archive_entry_clear(entry);
                    continue;
                } else if (!overwriteAll && !skipAll) {
                    OverwriteQuery query(entryName);
                    emit userQuery(&query);
                    query.waitForResponse();

                    if (query.responseCancelled()) {
                        archive_read_data_skip(arch_reader.data());
                        archive_entry_clear(entry);
                        break;
                    } else if (query.responseSkip()) {
                        archive_read_data_skip(arch_reader.data());
                        archive_entry_clear(entry);
                        continue;
                    } else if (query.responseAutoSkip()) {
                        archive_read_data_skip(arch_reader.data());
                        archive_entry_clear(entry);
                        skipAll = true;
                        continue;
                    } else if (query.responseRename()) {
                        const QString newName(query.newFilename());
                        fileBeingRenamed = newName;
                        archive_entry_copy_pathname(entry, QFile::encodeName(newName).constData());
                        goto retry;
                    } else if (query.responseOverwriteAll()) {
                        overwriteAll = true;
                    }
                }
            }

            //if there is an already existing directory:
            if (entryIsDir && entryFI.exists()) {
                if (entryFI.isWritable()) {
                    qDebug() << "Warning, existing, but writable dir";
                } else {
                    qDebug() << "Warning, existing, but non-writable dir. skipping";
                    archive_entry_clear(entry);
                    archive_read_data_skip(arch_reader.data());
                    continue;
                }
            }

            int header_response;
            qDebug() << "Writing " << fileWithoutPath << " to " << archive_entry_pathname(entry);
            emit currentFile(QFile::decodeName(archive_entry_pathname(entry)));

            if ((header_response = archive_write_header(writer.data(), entry)) == ARCHIVE_OK) {
                //if the whole archive is extracted and the total filesize is available, we use partial progress
                int data_response = copyData(arch_reader.data(), writer.data(), archive_entry_size(entry), (extractAll && m_extractedFilesSize));
                if (data_response != ARCHIVE_OK) {
                    qDebug() << "Copy data failed with response" << data_response << "==" << responseName(data_response);
                    return false;
                }
            } else {
                qDebug() << "Writing header failed with response" << header_response << "==" << responseName(header_response)
                         << "While attempting to write " << entryName;

                qDebug() << "Error" << archive_errno(writer.data()) << ':'<< archive_error_string(writer.data());
                emit error(archive_error_string(writer.data()), "Error detail");
                return false;
            }

            //if we only partially extract the archive and the number of
            //archive entries is available we use a simple progress based on
            //number of items extracted
            if (!extractAll && m_cachedArchiveEntryCount) {
                ++entryNr;
                emit totalProgress(100 * entryNr / totalCount);
            }
            archive_entry_clear(entry);
        } else {
            archive_read_data_skip(arch_reader.data());
        }
    }

    bool reader_closed = ( archive_read_close(arch_reader.data()) == ARCHIVE_OK );
    bool writer_closed = (archive_write_close(writer.data()) == ARCHIVE_OK);

    return reader_closed && writer_closed;
}

int QLibArchive::countFiles(const QStringList &files, QStringList *filesList)
{
    int f_count = 0;
    QStringList f_list;

    foreach(const QString& selectedFile, files) {
        f_count ++;
        f_list.push_back(selectedFile);

        if (QFileInfo(selectedFile).isDir()) {
            QDirIterator it(selectedFile,
                            QDir::AllEntries | QDir::Readable | QDir::Hidden | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (it.hasNext()) {
                const QString path = it.next();

                if ((it.fileName() == QLatin1String("..")) ||
                    (it.fileName() == QLatin1String("."))) {
                    continue;
                }

                f_list.push_back(path);
                f_count++;
            }
        } //END selectedFile.isDir()
    }

    filesList->clear();
    filesList->append(f_list);
    return f_count;

}

bool QLibArchive::addFiles(const QStringList &files, const CompressionOptions &options)
{
    const bool creatingNewFile = !QFileInfo(filename()).exists();
    const QString tempFilename = filename() + QLatin1String( ".Temp" );
    const QString globalWorkDir = options.value(QLatin1String( "GlobalWorkDir" )).toString();
    const QString codepage = getArchive()->codePage();
    //QStringList entry_list;

    if (!globalWorkDir.isEmpty()) {
        qDebug() << "GlobalWorkDir is set, changing dir to " << globalWorkDir;
        m_workDir.setPath(globalWorkDir);
        QDir::setCurrent(globalWorkDir);
    }

    m_writtenFiles.clear();

    // If archive existing, create archive reader object
    ArchiveRead arch_reader(NULL);
    if (!creatingNewFile) {
        arch_reader.reset(archive_read_new());
        if (!(arch_reader.data())) {
            emit error(tr("The archive reader could not be initialized."));
            qDebug("Error %i: %s", archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );// tohle je nesmysl, nic to nevypise => reader je null
            return false;
        }

        if (archive_read_support_format_all(arch_reader.data()) != ARCHIVE_OK) {
            emit error(archive_error_string(arch_reader.data()));
            qDebug("Error %i: %s", archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
            return false;
        }

        if (archive_read_support_compression_all(arch_reader.data()) != ARCHIVE_OK) {
            emit error(archive_error_string(arch_reader.data()));
            qDebug("Error %i: %s", archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
            return false;
        }

        if (!codepage.isEmpty()) {
            QByteArray options("hdrcharset=");
            options += codepage.toLocal8Bit();

            if (archive_read_set_options(arch_reader.data(), options /*"hdrcharset=CP852"*/) != ARCHIVE_OK) {
                emit error(archive_error_string(arch_reader.data()));
                qDebug("Error %i: %s", archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
                qDebug("This system cannot convert character-set for %s.", options.constData());
                return false;
            }
        }

        // Open reader
        if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
            emit error(tr("The source file could not be read."), archive_error_string(arch_reader.data()));
            qDebug("Error %i: %s",archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
            return false;
        }
    }

    // Create archive writer object
    ArchiveWrite arch_writer(archive_write_new());
    if (!(arch_writer.data())) {
        emit error(tr("The archive writer could not be initialized."));
        qDebug("Error %i: %s", archive_errno(arch_writer.data()), archive_error_string(arch_writer.data()) );
        return false;
    }

    // Base Format
    int arch_format_base = (m_archFormat & ARCHIVE_FORMAT_BASE_MASK);
    int ret = 0;

    switch (arch_format_base) {
    case ARCHIVE_FORMAT_ZIP:
        ret = archive_write_set_format_zip(arch_writer.data());
        break;
    case ARCHIVE_FORMAT_CPIO:
        switch (m_archFormat) {
        case ARCHIVE_FORMAT_CPIO_SVR4_NOCRC:
        case ARCHIVE_FORMAT_CPIO_SVR4_CRC:
            ret = archive_write_set_format_cpio_newc(arch_writer.data());
            break;
        default:
            ret = archive_write_set_format_cpio(arch_writer.data());
            break;
        }
        break;
    case ARCHIVE_FORMAT_TAR:
        switch (m_archFormat) {
        case ARCHIVE_FORMAT_TAR_USTAR:
            ret = archive_write_set_format_ustar(arch_writer.data());
            break;
        case ARCHIVE_FORMAT_TAR_PAX_RESTRICTED:
            ret = archive_write_set_format_pax_restricted(arch_writer.data());
            break;
        case ARCHIVE_FORMAT_TAR_PAX_INTERCHANGE:
            ret = archive_write_set_format_pax(arch_writer.data());
            break;
        case ARCHIVE_FORMAT_TAR_GNUTAR:
            ret = archive_write_set_format_gnutar(arch_writer.data());
            break;
        default:
            ret = archive_write_set_format_pax_restricted(arch_writer.data());
            break;
        }
        break;
    case ARCHIVE_FORMAT_AR:
        switch (m_archFormat) {
        case ARCHIVE_FORMAT_AR_BSD:
            ret = archive_write_set_format_ar_bsd(arch_writer.data());
            break;
        case ARCHIVE_FORMAT_AR_GNU:
            ret = archive_write_set_format_ar_svr4(arch_writer.data());
            break;
        default:
            ret = archive_write_set_format_ar_bsd(arch_writer.data());
            break;
        }
        break;
    case ARCHIVE_FORMAT_LHA:
        emit error(tr("Libarchive can not write archive in this format."));
        return false;
        break;
    }
    if (ret != ARCHIVE_OK) {
        emit error(QString::fromLocal8Bit(archive_error_string(arch_writer.data())));
        qDebug("Error %i: %s", archive_errno(arch_writer.data()), archive_error_string(arch_writer.data()) );
        return false;
    }

    switch (m_archFilter) {
    case ARCHIVE_COMPRESSION_GZIP:
        ret = archive_write_set_compression_gzip(arch_writer.data());
        break;
    case ARCHIVE_COMPRESSION_BZIP2:
        ret = archive_write_set_compression_bzip2(arch_writer.data());
        break;
    case ARCHIVE_COMPRESSION_XZ:
        ret = archive_write_set_compression_xz(arch_writer.data());
        break;
    case ARCHIVE_COMPRESSION_LZMA:
        ret = archive_write_set_compression_lzma(arch_writer.data());
        break;
    case ARCHIVE_COMPRESSION_COMPRESS:
        ret = archive_write_set_compression_compress(arch_writer.data());
        break;
    case ARCHIVE_COMPRESSION_NONE:
        ret = archive_write_set_compression_none(arch_writer.data());
        break;
    default:
        emit error(tr("The compression type '%1' is not supported by LibArchive.")
                   .arg( QString::fromLocal8Bit(archive_compression_name(arch_reader.data())) ));
        return false;
    }
    if (ret != ARCHIVE_OK) {
        emit error(QString::fromLocal8Bit(archive_error_string(arch_writer.data())));
        qDebug("Error %i: %s", archive_errno(arch_writer.data()), archive_error_string(arch_writer.data()) );
        return false;
    }

    if (!codepage.isEmpty()) {
        QByteArray options("hdrcharset=");
        options += codepage.toLocal8Bit();

        if (archive_write_set_options(arch_writer.data(), options /*"hdrcharset=CP852"*/) != ARCHIVE_OK) {
            emit error(tr("Libarchive can't convert character set."), archive_error_string(arch_writer.data()));
            qDebug("Error %i: %s", archive_errno(arch_writer.data()), archive_error_string(arch_writer.data()) );
            qDebug("This system cannot convert character-set for %s.",options.constData());
            return false;
        }
    }

    // Open Writer
    ret = archive_write_open_filename(arch_writer.data(), QFile::encodeName(tempFilename));
    if (ret != ARCHIVE_OK) {
        emit error(tr("Opening the archive for writing failed with the following error: <message>%1</message>","@info")
                   .arg(QLatin1String(archive_error_string(arch_writer.data()))));
        qDebug("Error %i: %s", archive_errno(arch_writer.data()), archive_error_string(arch_writer.data()) );
        return false;
    }

    // Write new files, copy files from old to new archive...
    //int files_count = files.count();
    QStringList f_list;
    int files_count = countFiles(files, &f_list);
    int writen_count = 0;
    m_extractedFilesSize = 0;
    emit totalProgress(100 * writen_count/files_count);


    // ********** Write the new files **********
    foreach(const QString& selectedFile, files) {
        bool success;

        success = fileToArchive(selectedFile, arch_writer.data());

        if (!success) {
            QFile::remove(tempFilename);
            return false;
        }
        writen_count++;
        emit totalProgress(100 * writen_count / files_count);

        if (QFileInfo(selectedFile).isDir()) {
            QDirIterator it(selectedFile,
                            QDir::AllEntries | QDir::Readable | QDir::Hidden | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (it.hasNext()) {
                const QString path = it.next();

                if ((it.fileName() == QLatin1String("..")) ||
                    (it.fileName() == QLatin1String("."))) {
                    continue;
                }

                //files_count++;
                emit totalProgress(100* writen_count / files_count);

                success = fileToArchive(path + (it.fileInfo().isDir() ? QLatin1String( "/" ) : QLatin1String( "" )),
                                    arch_writer.data());

                if (!success) {
                    QFile::remove(tempFilename);
                    return false;
                }
                writen_count++;
                emit totalProgress(100 * writen_count / files_count);
            }
        } //END selectedFile.isDir()
    }


    // ********** If we have old elements... **********
    if (!creatingNewFile) {
        //const bool changeEncoding = (!codepage.isEmpty() && arch_format == ARCHIVE_FORMAT_ZIP) && (m_options & AutoConvertFilenames);
        //********** copy old elements from previous archive to new archive
        int r;
        struct archive_entry *entry;

        while ((r = archive_read_next_header(arch_reader.data(), &entry)) == ARCHIVE_OK) {
            files_count++;
            emit totalProgress(100 * writen_count / files_count);
            emit currentFile(QString::fromLocal8Bit(archive_entry_pathname(entry)));

            if (m_writtenFiles.contains(QFile::decodeName(archive_entry_pathname(entry)))) {
                // Nahrad položku nově přidávaným souborem
                emit currentFileProgress(0);
                archive_read_data_skip(arch_reader.data());
                qDebug() << "Existing entry, will be refresh: =>" << archive_entry_pathname(entry);
                files_count--;
                emit currentFileProgress(100);
                continue;
            }

            const char* pathname = archive_entry_pathname(entry);
            const wchar_t* pathname_w = archive_entry_pathname_w(entry);
            if (pathname_w == NULL) {
                if (pathname == NULL) {
                    // Empty Filename
                    emit error(tr("Operation terminated."
                                  "During add operation were found entries with an empty file name." ,"@info"));
                } else {
                    // Filename encoding is not in current locale
                    emit error(tr("Currently opened archive contains the names of files in a unknown "
                                  "encoding or encoding currently set is not valid. LibArchive can't add files "
                                  "to archive in this encoding. Please select valid encoding from menu." ,"@info"));
                }
                // Close Writer
                archive_write_close(arch_writer.data());
                arch_writer.reset(NULL);
                QFile::remove(tempFilename);
                return false;
            }

            qDebug() << "Writing entry " << archive_entry_pathname(entry);
            int header_response;
            if ((header_response = archive_write_header(arch_writer.data(), entry)) == ARCHIVE_OK) {
                // count writen bytes
                m_extractedFilesSize += archive_entry_size(entry);
                int data_response = copyData(arch_reader.data(), arch_writer.data(), archive_entry_size(entry), false);
                if (data_response != ARCHIVE_OK) {
                    // Close Writer
                    archive_write_close(arch_writer.data());
                    arch_writer.reset(NULL);
                    QFile::remove(tempFilename);
                    return false;
                }
            } else {
                qDebug() << "Writing header failed with error code " << header_response << "==" << responseName(header_response);
                // Close Writer
                archive_write_close(arch_writer.data());
                arch_writer.reset(NULL);
                QFile::remove(tempFilename);
                return false;
            }
            writen_count++;
            emit totalProgress(100 * writen_count / files_count);

            // Clear entry for reuse
            archive_entry_clear(entry);
        }
        qDebug() << "Komprese:" << archive_compression(arch_reader.data());

        // Close Reader
        archive_read_close(arch_reader.data());
        arch_reader.reset(NULL);
    }
    emit totalProgress(100 * writen_count / files_count);

    // Close Writer
    archive_write_close(arch_writer.data());
    qint64 compressed = archive_filter_bytes(arch_writer.data(), -1);
    qint64 uncompressed = archive_filter_bytes(arch_writer.data(), 0);
    arch_writer.reset(NULL);

    if (!creatingNewFile) {
        // Remove old archive
        bool remove_ok = QFile::remove(filename());
        qDebug() << "Removing file" << filename() << remove_ok;
    }
    bool rename_ok = QFile::rename(tempFilename, filename());
    qDebug() << "Rename file" << tempFilename << "to" << filename() << rename_ok;

    Archive* arch_info = getArchive();
    arch_info->setEntryCount(files_count);
    arch_info->setExtractedSize(qMax(uncompressed, m_extractedFilesSize));
    arch_info->setCompressedSize(compressed);

    return true;
}


bool QLibArchive::deleteFiles(const QVariantList &files)
{
    Archive* arch_info = getArchive();
    const QString tempFilename = arch_info->fileName() + QLatin1String( ".Temp" );
    const QString codepage = arch_info->codePage();

    // Create Reader
    ArchiveRead arch_reader(archive_read_new());
    if (!(arch_reader.data())) {
        emit error(tr("The archive reader could not be initialized."));
        qDebug("Error %i: %s", archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_support_format_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug("Error %i: %s", archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
        return false;
    }

    if (archive_read_support_compression_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug("Error %i: %s", archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
        return false;
    }

    if (!codepage.isEmpty()) {
        QByteArray options("hdrcharset=");
        options += codepage.toLocal8Bit();

        if (archive_read_set_options(arch_reader.data(), options /*"hdrcharset=CP852"*/) != ARCHIVE_OK) {
            emit error(tr("Libarchive can't convert character set."), archive_error_string(arch_reader.data()) );
            qDebug("Error %i: %s", archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
            qDebug("This system can't convert character-set for %s.",options.constData());
            return false;
        }
    }

    // Open Reader
    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
        emit error(tr("The source file could not be read."), archive_error_string(arch_reader.data()));
        qDebug("Error %i: %s",archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
        return false;
    }

    // Create Writer
    ArchiveWrite arch_writer(archive_write_new());
    if (!(arch_writer.data())) {
        emit error(tr("The archive writer could not be initialized."));
        return false;
    }

    // Base Format
    int arch_format_base = (m_archFormat & ARCHIVE_FORMAT_BASE_MASK);
    int ret = 0;

    switch (arch_format_base) {
    case ARCHIVE_FORMAT_ZIP:
        ret = archive_write_set_format_zip(arch_writer.data());
        break;
    case ARCHIVE_FORMAT_CPIO:
        switch (m_archFormat) {
        case ARCHIVE_FORMAT_CPIO_SVR4_NOCRC:
        case ARCHIVE_FORMAT_CPIO_SVR4_CRC:
            ret = archive_write_set_format_cpio_newc(arch_writer.data());
            break;
        default:
            ret = archive_write_set_format_cpio(arch_writer.data());
            break;
        }
        break;
    case ARCHIVE_FORMAT_TAR:
        switch (m_archFormat) {
        case ARCHIVE_FORMAT_TAR_USTAR:
            ret = archive_write_set_format_ustar(arch_writer.data());
            break;
        case ARCHIVE_FORMAT_TAR_PAX_RESTRICTED:
            ret = archive_write_set_format_pax_restricted(arch_writer.data());
            break;
        case ARCHIVE_FORMAT_TAR_PAX_INTERCHANGE:
            ret = archive_write_set_format_pax(arch_writer.data());
            break;
        case ARCHIVE_FORMAT_TAR_GNUTAR:
            ret = archive_write_set_format_gnutar(arch_writer.data());
            break;
        default:
            ret = archive_write_set_format_pax_restricted(arch_writer.data());
            break;
        }
        break;
    case ARCHIVE_FORMAT_AR:
        switch (m_archFormat) {
        case ARCHIVE_FORMAT_AR_BSD:
            ret = archive_write_set_format_ar_bsd(arch_writer.data());
            break;
        case ARCHIVE_FORMAT_AR_GNU:
            ret = archive_write_set_format_ar_svr4(arch_writer.data());
            break;
        default:
            ret = archive_write_set_format_ar_bsd(arch_writer.data());
            break;
        }
        break;
    case ARCHIVE_FORMAT_LHA:
        emit error(tr("Libarchive can not write archive in this format."));
        return false;
        break;
    }
    if (ret != ARCHIVE_OK) {
        emit error(QString::fromLocal8Bit(archive_error_string(arch_writer.data())));
        qDebug("Error %i: %s", archive_errno(arch_writer.data()), archive_error_string(arch_writer.data()) );
        return false;
    }

    switch (m_archFilter) {
    case ARCHIVE_COMPRESSION_GZIP:
        ret = archive_write_set_compression_gzip(arch_writer.data());
        break;
    case ARCHIVE_COMPRESSION_BZIP2:
        ret = archive_write_set_compression_bzip2(arch_writer.data());
        break;
    case ARCHIVE_COMPRESSION_XZ:
        ret = archive_write_set_compression_xz(arch_writer.data());
        break;
    case ARCHIVE_COMPRESSION_LZMA:
        ret = archive_write_set_compression_lzma(arch_writer.data());
        break;
    case ARCHIVE_COMPRESSION_COMPRESS:
        ret = archive_write_set_compression_compress(arch_writer.data());
        break;
    case ARCHIVE_COMPRESSION_NONE:
        ret = archive_write_set_compression_none(arch_writer.data());
        break;
    default:
        emit error(tr("The compression type '%1' is not supported by LibArchive.")
                   .arg( QString::fromLocal8Bit(archive_compression_name(arch_reader.data())) ));
        return false;
    }
    if (ret != ARCHIVE_OK) {
        emit error(QString::fromLocal8Bit(archive_error_string(arch_writer.data())));
        qDebug("Error %i: %s", archive_errno(arch_writer.data()), archive_error_string(arch_writer.data()) );
        return false;
    }

    if (!codepage.isEmpty()) {
        QByteArray options("hdrcharset=");
        options += codepage.toLocal8Bit();

        if (archive_write_set_options(arch_writer.data(), options) != ARCHIVE_OK) {
            emit error(tr("Libarchive can't convert character set."), archive_error_string(arch_writer.data()));
            qDebug("Error %i: %s", archive_errno(arch_writer.data()), archive_error_string(arch_writer.data()) );
            qDebug("This system cannot convert character-set for %s.",options.constData());
            return false;
        }
    }

    // Open Writer
    ret = archive_write_open_filename(arch_writer.data(), QFile::encodeName(tempFilename));
    if (ret != ARCHIVE_OK) {
        emit error(tr("Opening the archive for writing failed with the following error: <message>%1</message>","@info")
                   .arg(archive_error_string(arch_writer.data())) );
        return false;
    }

    int files_count = arch_info->entryCount();
    int writen_count = 0;
    int deleted_count = 0;
    m_extractedFilesSize = 0;
    emit totalProgress(0);

    struct archive_entry *entry;

    /* Copy old elements from previous archive to new archive. */
    while (archive_read_next_header(arch_reader.data(), &entry) == ARCHIVE_OK) {

        const char* pathname = archive_entry_pathname(entry);
        const wchar_t* pathname_w = archive_entry_pathname_w(entry);
        if (pathname_w == NULL) {
            if (pathname == NULL) {
                // Empty Filename
                emit error(tr("Operation terminated."
                              "During delete operation were found entries with an empty file name." ,"@info"));
            } else {
                // Filename encoding is not in current locale
                emit error(tr("Currently opened archive contains the names of files in a unknown "
                              "encoding or encoding currently set is not valid. LibArchive can't add files "
                              "to archive in this encoding. Please select valid encoding from menu." ,"@info"));
            }
            // Close Reader
            archive_read_close(arch_reader.data());
            // Close writer
            archive_write_close(arch_writer.data());
            // Delete writer
            arch_writer.reset(NULL);
            QFile::remove(tempFilename);
            return false;
        }

        if (files.contains(QFile::decodeName(archive_entry_pathname(entry)))) {
            emit currentFile(tr("Delleting: %1").arg(QFile::decodeName(archive_entry_pathname(entry))));
            qDebug() << "Entry to be deleted, skipping" << archive_entry_pathname(entry);
            archive_read_data_skip(arch_reader.data());
            writen_count++;
            deleted_count++;
            emit totalProgress(100 * writen_count / files_count);
            emit entryRemoved(QFile::decodeName(archive_entry_pathname(entry)));
            continue;
        }

        int header_response;
        emit currentFile(QFile::decodeName(archive_entry_pathname(entry)));
        qDebug() << "Writing entry " << archive_entry_pathname(entry);
        if ((header_response = archive_write_header(arch_writer.data(), entry)) == ARCHIVE_OK) {
            // Count writen bytes
            m_extractedFilesSize += archive_entry_size(entry);
            int data_response = copyData(arch_reader.data(), arch_writer.data(), archive_entry_size(entry), false);
            if (data_response != ARCHIVE_OK) {
                // Close Writer
                archive_write_close(arch_writer.data());
                arch_writer.reset(NULL);
                QFile::remove(tempFilename);
                return false;
            }
            writen_count++;
            emit totalProgress(100 * writen_count / files_count);
        } else {
            qDebug() << "Writing header failed with error code" << header_response << "==" << responseName(header_response);
            return false;
        }
    }

    // Close Writer
    archive_write_close(arch_writer.data());
    qint64 compressed = archive_filter_bytes(arch_writer.data(), -1);
    qint64 uncompressed = archive_filter_bytes(arch_writer.data(), 0);
    arch_writer.reset(NULL);

    // Close Reader
    archive_read_close(arch_reader.data());
    arch_reader.reset(NULL);

    QFile::remove(filename());
    QFile::rename(tempFilename, filename());
    arch_info->setEntryCount(files_count - deleted_count);
    arch_info->setExtractedSize(qMax(uncompressed, m_extractedFilesSize));
    arch_info->setCompressedSize(compressed);

    return true;
}


bool QLibArchive::test()
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

    if (archive_read_support_format_all(arch_reader.data()) != ARCHIVE_OK) {
        emit error(archive_error_string(arch_reader.data()));
        qDebug( archive_error_string(arch_reader.data()) );
        return false;
    }

    QString codepage = getArchive()->codePage();
    if (!codepage.isEmpty()) {
        QByteArray options("hdrcharset=");
        options += codepage.toLocal8Bit();

        if (archive_read_set_options(arch_reader.data(), options /*"hdrcharset=CP852"*/) != ARCHIVE_OK) {
            emit error(archive_error_string(arch_reader.data()));
            qDebug("Error %i: %s", archive_errno(arch_reader.data()), archive_error_string(arch_reader.data()) );
            qDebug("This system cannot convert character-set for %s.",options.constData());
            return false;
        }
    }

    if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(filename()), 10240) != ARCHIVE_OK) {
    //if (archive_read_open_filename(arch_reader.data(), QFile::encodeName(archive->fileName()), 10240) != ARCHIVE_OK) {
        emit error(tr("Could not open the archive <i>%1</i>.", "@info").arg(filename()));
        //emit error(tr("Could not open the archive <i>%1</i>.", "@info").arg( archive->fileName() ));
        qDebug("%i: %s", archive_errno( arch_reader.data() ), strerror(archive_errno( arch_reader.data() )));
        qDebug( archive_error_string( arch_reader.data() ) );
        return false;
    }

    emit totalProgress(0);
    //m_cachedArchiveEntryCount;
    m_extractedFilesSize = getArchive()->extractedSize();
    qint64 procesedSize = 0;

    struct archive_entry *entry;
    qint64 entry_size;

    int error_count = 0;
    int result = 0;
    //while ((result = archive_read_next_header(arch_reader.data(), &entry)) == ARCHIVE_OK) {
    while ((result = archive_read_next_header(arch_reader.data(), &entry)) < ARCHIVE_EOF) {
        if (result == ARCHIVE_FATAL) {
            /* Krytické poškození archivu, další operace nejsou možné */
            break;
        }

        const char* pathname = archive_entry_pathname(entry);
        const wchar_t* pathname_w = archive_entry_pathname_w(entry);
        if (pathname_w == NULL && pathname != NULL) {
            qDebug() << "QLibArchive::detectEncoding" << "Filenames in unknown encoding";
        }

        entry_size = archive_entry_size(entry);
        //m_extractedFilesSize += entry_size;

        m_archFormat = archive_format(arch_reader.data());
        qDebug() << "ArchiveFormat:" << m_archFormat;
        m_archFormatName = QString::fromLocal8Bit(archive_format_name(arch_reader.data()));
        qDebug() << "ArchiveFormatName:" << m_archFormatName;

        m_archFilesCount = archive_file_count(arch_reader.data());
        //m_cachedArchiveEntryCount++;
        qDebug() << "ArchiveFileCount:" << m_archFilesCount << "m_cachedArchiveEntryCount:" << m_cachedArchiveEntryCount;

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
            emit currentFileProgress(static_cast<unsigned long>(100.0 * double(readed_size) / entry_size));
            qDebug() << "EntrySize:" << entry_size << "Size:" << size << "ReadedSize" << readed_size;
            //emit testResult(QString("%1, %2, %3, %4").arg(r).arg(pathname).arg(archive_errno(arch_reader.data())).arg(archive_error_string(arch_reader.data())));
            if (r < ARCHIVE_OK) {
                error = 1;
                error_count++;
                emit testResult(QString("\n  Error: %1\n").arg(archive_error_string(arch_reader.data())));
            }
            qDebug() << "ReadData Returning:" <<  responseName(r);
            if (r==ARCHIVE_EOF || r==ARCHIVE_FATAL) {
                break;
            }
        }

        emit testResult(error == 0 ? QString("OK\n") : QString("\n"));
        if (pathname_w == NULL && pathname == NULL) {
            emit testResult(tr("  Warning: Found entry with empty filename.\n"));
        }
        //int prgrc = int(100.0 * double(m_archFilesCount) / double(m_cachedArchiveEntryCount));
        //int prgrs = int(100.0 * double(procesedSize) / double(m_extractedFilesSize));
        //qDebug() << "ProgrCount:" << prgrc << "ProgrSize:" << prgrs;

        emit totalProgress(100 * procesedSize / m_extractedFilesSize); // Přesnější
    }

    if (result != ARCHIVE_EOF) {
        error_count++;
        emit testResult(QString("Error: %1").arg(archive_error_string(arch_reader.data())));
        emit error(tr("<qt>The archive reading failed with the following error: <i>%1</i></qt>", "@info")
                   .arg( archive_error_string(arch_reader.data()) ) );
        //return false;
    }
    m_archFormat = archive_format(arch_reader.data());
    bool ok = (archive_read_close(arch_reader.data()) == ARCHIVE_OK);


    emit testResult(error_count == 0 ?
                        QString("\nThere were no errors in the archive: %1\n").arg(filename()) :
                        QString("\nTotal errors: %1\n").arg(error_count));
    return ok;
}


bool QLibArchive::isReadOnly() const
{
    QStringList readonly_mimetypes = readOnlyMimeTypes().split(';', QString::SkipEmptyParts);
    return readonly_mimetypes.contains(getArchive()->mimeType().name());
}


void QLibArchive::emitEntryFromArchiveEntry(archive_entry *aentry)
{
    ArchiveEntry e;

    const wchar_t *pathname_w = archive_entry_pathname_w(aentry);// Nefunguje s cz názvy v windows Zipu -> vrací null
    const char *pathname = archive_entry_pathname(aentry); // multibyte pathname in curent locale
    // -> Problém: v zipu vytvořeném na OS Win názvy souborů v CP852 != Linux curentLocal obvykle UTF-8
    //qDebug() << "Filename" << pathname_w;
    //qDebug() << "Filename" << pathname;
    //qDebug() << "FromNativeSep" << QDir::fromNativeSeparators(QString::fromLocal8Bit(pathname));

    if (pathname_w != NULL) {
        // Název je v curent locale, použij ho
        //e[FileName] = QDir::fromNativeSeparators(QString::fromWCharArray(archive_entry_pathname_w(aentry)));
        e[FileName] = QDir::fromNativeSeparators(QString::fromWCharArray(pathname_w));        
    }
    else {
        // použij fromLocal8Bit(), soubor se alespoň zobrazí, ale názvy budou chybné
        e[FileName] = QDir::fromNativeSeparators(QString::fromLocal8Bit(pathname));
    }

    e[InternalID] = e[FileName];

    e[Permissions] = QString::fromLocal8Bit(archive_entry_strmode(aentry));

    const QString owner = QString::fromLocal8Bit(archive_entry_uname(aentry));
    if (!owner.isEmpty()) {
        e[Owner] = owner;
    }

    const QString group = QString::fromLocal8Bit(archive_entry_gname(aentry));
    if (!group.isEmpty()) {
        e[Group] = group;
    }

    e[Size] = (qlonglong)archive_entry_size(aentry);
    e[IsDirectory] = S_ISDIR(archive_entry_mode(aentry));
    //qDebug() << "Filename:" << e[FileName] << "IsDir:" << e[IsDirectory] << "FileType" << archive_entry_filetype(aentry) << S_IFREG << AE_IFREG;

    if (archive_entry_symlink(aentry)) {
        e[Link] = QString::fromLocal8Bit( archive_entry_symlink(aentry) );
    }

    /* LibArchive supported time fields are
     *  atime (access time),
     *  birthtime (creation time),
     *  ctime (last time an inode property was changed) and
     *  mtime (modification time)
     */
    e[Timestamp] = QDateTime::fromTime_t(archive_entry_mtime(aentry));

    emit entry(e);
}

/*
QString QLibarchive::dec()
{

    // Pokus se převést název pomocí zadané codepage
    char *utf8_str = 0;
    int utf8_len = 0;
    // QString codepage => const char* codepage
    // QString::toLocal8Bit().constData() vraci const char*
    // QString::toLocal8Bit() vraci QByteArray, lze rovnou použít všude, kde je očekáván const char*
    if (str_to_utf8(archive_entry_pathname(aentry), codepage.toLocal8Bit(), &utf8_str, &utf8_len) == 0)
    {
        //utf8_str[utf8_len] = '\0'; //Může selhat, ve funkci není zaručeno, že pole bude mít na konci volný byte.
        qDebug() << "Filename" << utf8_str << "Lenth"<< utf8_len << QString::fromUtf8(utf8_str, utf8_len);

        e[FileName] = QDir::fromNativeSeparators(QString::fromUtf8(utf8_str, utf8_len));

        free(utf8_str); // utf8_str alokován pomocí malloc(),nebo realloc() proto free()
    } else {
        // Nelze převést do zadané zadané codepage, použij fromLocal8Bit()
        e[FileName] = QDir::fromNativeSeparators(QString::fromLocal8Bit(pathname));
    }

}*/


int QLibArchive::extractionFlags() const
{
    int result = ARCHIVE_EXTRACT_TIME;
    result |= ARCHIVE_EXTRACT_SECURE_NODOTDOT;

    /*
    result &= ARCHIVE_EXTRACT_PERM;

    result &= ARCHIVE_EXTRACT_NO_OVERWRITE;
    */

    return result;
}


int QLibArchive::copyData(struct archive *source, struct archive *dest, qint64 entry_size, bool partialprogress)
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
            qDebug() << "Error while extracting..."
                     << "Error" << archive_errno(dest) << ':' << archive_error_string(dest);
            emit error(archive_error_string(dest), "Error detail");
            return -1;
        }

        if (entry_size > 0) {
            writen_size += readBytes;
            emit currentFileProgress(100 * writen_size / entry_size);
        }

        if (partialprogress) {//ToDo:
            m_currentExtractedFilesSize += readBytes;
            emit totalProgress(100 * m_currentExtractedFilesSize / m_extractedFilesSize);
        }

        readBytes = archive_read_data(source, buff, sizeof(buff));
    }

    emit currentFileProgress(100);

    /*ZIP : pokud data šifrována nelze je číst*/
    if (readBytes < 0) {
        /* Error: archive_read_data() returns  ARCHIVE_FATAL, ARCHIVE_WARN, or ARCHIVE_RETRY (dle dokumentace)*/
        /* !!! ale v případě zaheslovaného zip vrací -25 == ARCHIVE_FAILED !!! */
        qDebug() << "Error while reading archive data..." << readBytes;
        qDebug() << "Error" << archive_errno(source) << ':' << archive_error_string(source);
        emit error(archive_error_string(source));
        return readBytes;
    }
    /*END ZIP*/

    return ARCHIVE_OK;
}


bool QLibArchive::fileToArchive(const QString &fileName, archive *arch_writer)
{
    // Create disk reader
    ArchiveRead arch_read_disk(archive_read_disk_new());
    if (!(arch_read_disk.data())) {
        emit error(tr("The disk reader could not be initialized."));
        qDebug("Error %i: %s", archive_errno(arch_read_disk.data()), archive_error_string(arch_read_disk.data()) );
        return false;
    }
    archive_read_disk_set_standard_lookup(arch_read_disk.data());

    emit currentFile(fileName);
    emit currentFileProgress(0);

    const bool trailingSlash = fileName.endsWith(QLatin1Char( '/' ));
    const QString relativeName = m_workDir.relativeFilePath(fileName) + (trailingSlash ? QLatin1String( "/" ) : QLatin1String( "" ));

    struct stat st;
    memset(&st, 0, sizeof(st)); // nulovat asi neni potreba
#ifdef Q_OS_WIN
    stat(QFile::encodeName(fileName).constData(), &st);
#else
    lstat(QFile::encodeName(fileName).constData(), &st);
#endif

    int fd = ::open(fileName.toLocal8Bit(), O_RDONLY);
    if (fd < 0) {
        qDebug("%s: open(%s) failed:", __func__, fileName.toLocal8Bit().constData());
        qDebug("- Errno: %d: %s\n",  errno, strerror(errno));
    }

    struct archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, QFile::encodeName(relativeName).constData());
    archive_entry_copy_sourcepath(entry, QFile::encodeName(fileName).constData());

    //int r = archive_read_disk_entry_from_file(arch_read_disk.data(), entry, -1, &st);
    //int r = archive_read_disk_entry_from_file(arch_read_disk.data(), entry, -1, 0);
    int r = archive_read_disk_entry_from_file(arch_read_disk.data(), entry, fd, &st);
    if (r != ARCHIVE_OK) {
        qDebug("Error %i: %s", archive_errno(arch_read_disk.data()), archive_error_string(arch_read_disk.data()) );
    }

    //archive_entry_set_pathname(entry, encodeName(relativeName, codepage).constData());
    // LibArchive prefer UTF-8 names if present UTF-8 name field
    // Zápis v jiné codepage je sice možný, ale libarchive nedokéže převést název do UTF-8
    // proto UTF-8 alternativa názvu zůstane prázdná ==>> problém!
    // Při čtení preferuje UTF-8 název, pokud zip patřičnou položku obsahuje,
    // ale ten je prázdný!! V jiném sw, který čte názvy ze standardní položky pathname
    // jsou soubory přístupné, ale v libarchive nikoliv.
    // Mžné řešení:
    // 1) Nedovolit zápis do archivů s jinou codePage, než je currentLocale
    // 2) Při zápisu archiv convertovat do aktualní codepage.
    // 3) Nastavit setlocale() s codepage použité v archivu
    //     - vše by sice fungovalo ok, ale  => cs_CZ.cp852 s codepage cp852 neexistuje!!!
    // 4) Použit libarchive parametr hdrcharset => ověřeno, funguje!

    qDebug() << "Writing new entry " << archive_entry_pathname(entry);
    qint64 entry_size = archive_entry_size(entry);
    qint64 writen_size = 0;
    int header_response;
    if ((header_response = archive_write_header(arch_writer, entry)) == ARCHIVE_OK) {
        //if the whole archive is extracted and the total filesize is
        //available, we use partial progress
        //copyData(fileName, arch_writer, false);

        bool partialprogress = false;
        char buff[10240];
        ssize_t readBytes;
        if (fd != -1) {
            readBytes = read(fd, buff, sizeof(buff));
            while (readBytes > 0) {
                /* int writeBytes = */
                archive_write_data(arch_writer, buff, readBytes);
                if (archive_errno(arch_writer) != ARCHIVE_OK) {
                    qDebug() << "Error while writing..." << archive_error_string(arch_writer) << "(error nb =" << archive_errno(arch_writer) << ')';
                }

                writen_size += readBytes;
                m_extractedFilesSize += readBytes;
                emit currentFileProgress(100 * writen_size / entry_size);

                if (partialprogress) {
                    m_currentExtractedFilesSize += readBytes;
                    emit totalProgress(100 * m_currentExtractedFilesSize / m_extractedFilesSize);
                }

                readBytes = read(fd, buff, sizeof(buff));
            }
            close(fd);
        }
    } else {
        qDebug() << "Writing header failed with error code " << header_response;
        qDebug() << "Error while writing..." << fileName
                 << endl << archive_error_string(arch_writer) << "(error nb =" << archive_errno(arch_writer) << ')';

        emit error(tr("<qt><br/>QArchiver could not compress: %1<br/>%2.</qt>").arg(
                       fileName, archive_error_string(arch_writer)));

        archive_entry_free(entry);
        return false;
    }

    m_writtenFiles.push_back(relativeName);

    emitEntryFromArchiveEntry(entry);

    archive_entry_free(entry);
    return true;
}


const QString QLibArchive::relativeToLocalWD(const QString &fileName)
{
    const bool trailingSlash = fileName.endsWith(QLatin1Char( '/' ));
    const QString relativeName = m_workDir.relativeFilePath(fileName) + (trailingSlash ? QLatin1String( "/" ) : QLatin1String( "" ));

    return relativeName;
}


QString QLibArchive::absoluteFilePathLocalWD(const QString &relFilePath)
{
    return m_workDir.absoluteFilePath(relFilePath);
}


QStringList QLibArchive::archiveEntryList(struct archive *a_reader,const QString &fileName)
{
    struct archive_entry *entry;

    if (archive_read_open_filename(a_reader, QFile::encodeName(fileName), 10240) != ARCHIVE_OK) {
        emit error(tr("The source file could not be read."));
        qDebug("Error %i: %s", archive_errno(a_reader), archive_error_string(a_reader) );
        return QStringList();
    }

    QStringList entry_list;
    while (archive_read_next_header(a_reader, &entry) == ARCHIVE_OK) {
        entry_list << archive_entry_pathname(entry);
        archive_read_data_skip(a_reader);
    }

    if (archive_read_close(a_reader) != ARCHIVE_OK) {
        qDebug("Error %i: %s", archive_errno(a_reader), archive_error_string(a_reader) );
    }

    return entry_list;
}
