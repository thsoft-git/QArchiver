#include "cli7zplugin.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLatin1String>
#include <QString>

#include <QDebug>

Cli7zPlugin::Cli7zPlugin(QObject *parent) :
    CliInterface(parent)
    , m_archiveType(ArchiveType7z)
    , m_state(ReadStateHeader)
{
}

Cli7zPlugin::~Cli7zPlugin()
{
}

QStringList Cli7zPlugin::readOnlyMimeTypes()
{
    return QStringList() << "application/x-java-archive" << "application/x-arj" << "application/x-lha" << "application/x-rpm";
}

QStringList Cli7zPlugin::supportedMimetypes(ArchiveOpenMode mode)
{
    switch (mode) {
    case ReadOnly:
        return QStringList() << "application/x-7z-compressed"
                             << "application/x-java-archive"
                             << "application/x-tar"
                             << "application/x-tarz"
                             << "application/x-compressed-tar"
                             << "application/x-bzip-compressed-tar"
                             << "application/x-xz-compressed-tar"
                             << "application/x-lzma-compressed-tar"
                             << "application/x-rpm"
                             << "application/x-arj";
        break;
    case ReadWrite:
        return QStringList() << "application/x-7z-compressed"
                             << "application/x-tar"
                             << "application/x-tarz"
                             << "application/x-compressed-tar"
                             << "application/x-bzip-compressed-tar"
                             << "application/x-xz-compressed-tar"
                             << "application/x-lzma-compressed-tar";
        break;
    default:
        return QStringList();
        break;
    }

}

bool Cli7zPlugin::open()
{
    return analyze(m_archive);
}

bool Cli7zPlugin::isReadOnly() const
{
    QStringList readonly = readOnlyMimeTypes();
    return readonly.contains(m_archive->mimeType().name());
}

ParameterList Cli7zPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        p[ListProgram] = p[ExtractProgram] = p[DeleteProgram] = p[AddProgram] = QStringList() << QLatin1String( "7z" ) << QLatin1String( "7zr" ) << QLatin1String( "7za" );

        p[ListArgs] = QStringList() << QLatin1String( "l" ) << QLatin1String( "-slt" ) << QLatin1String( "$Archive" );
        p[ExtractArgs] = QStringList() << QLatin1String( "$PreservePathSwitch" ) << QLatin1String("$AutoOverwriteSwitch") << QLatin1String( "$PasswordSwitch" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );
        p[PreservePathSwitch] = QStringList() << QLatin1String( "x" ) << QLatin1String( "e" );
        p[AutoOverwriteSwitch] = QStringList() << QLatin1String("-aos") << QLatin1String("-aoa");// overwrite Skip, All
        p[PasswordSwitch] = QString( "-p$Password" );
        p[FileExistsExpression] = QLatin1String( "already exists. Overwrite with" );
        p[WrongPasswordPatterns] = QStringList() << QLatin1String( "Wrong password" );
        p[AddArgs] = QStringList() << QLatin1String( "a" )  << QLatin1String( "$PasswordSwitch" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );
        p[DeleteArgs] = QStringList() << QLatin1String( "d" ) << QLatin1String( "$PasswordSwitch" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );
        p[TestArgs] = QStringList() << QLatin1String("t")  << QLatin1String( "$PasswordSwitch" ) << QLatin1String( "$Archive" );

        p[FileExistsInput] = QStringList()
                             << QLatin1String( "Y" ) //overwrite
                             << QLatin1String( "N" ) //skip
                             << QLatin1String( "A" ) //overwrite all
                             << QLatin1String( "S" ) //autoskip
                             << QLatin1String( "Q" ) //cancel
                             ;

        p[PasswordPromptPattern] = QLatin1String("Enter password \\(will not be echoed\\) :");
    }

    return p;
}

bool Cli7zPlugin::readListLine(const QString& line)
{
    static const QLatin1String archiveInfoDelimiter1("--"); // 7z 9.13+
    static const QLatin1String archiveInfoDelimiter2("----"); // 7z 9.04
    static const QLatin1String entryInfoDelimiter("----------");

    switch (m_state) {
    case ReadStateHeader:
        if (line.startsWith(QLatin1String("Listing archive:"))) {
            qDebug() << "Archive name: "
                     << line.right(line.size() - 16).trimmed();
        } else if ((line == archiveInfoDelimiter1) ||
                   (line == archiveInfoDelimiter2)) {
            m_state = ReadStateArchiveInformation;
        } else if (line.contains(QLatin1String( "Error:" ))) {
            qDebug() << line.mid(6);
        }
        break;

    case ReadStateArchiveInformation:
        if (line == entryInfoDelimiter) {
            m_state = ReadStateEntryInformation;
        } else if (line.startsWith(QLatin1String("Type ="))) {
            const QString type = line.mid(7).trimmed();
            qDebug() << "Archive type: " << type;

            if (type == QLatin1String("7z")) {
                m_archiveType = ArchiveType7z;
            } else if (type == QLatin1String("BZip2") || type == QLatin1String("bzip2")) {
                m_archiveType = ArchiveTypeBZip2;
            } else if (type == QLatin1String("GZip") || type == QLatin1String("gzip")) {
                m_archiveType = ArchiveTypeGZip;
            } else if (type == QLatin1String("Tar") || type == QLatin1String("tar")) {
                m_archiveType = ArchiveTypeTar;
            } else if (type == QLatin1String("Zip") || type == QLatin1String("zip")) {
                m_archiveType = ArchiveTypeZip;
            } else if (type == QLatin1String("Arj") || type == QLatin1String("arj")) {
                m_archiveType = ArchiveTypeArj;
            } else {
                // Should not happen
                qWarning() << "Unsupported archive type";
                return false;
            }
        }

        break;

    case ReadStateEntryInformation:
        if (line.startsWith(QLatin1String("Path ="))) {
            const QString entryFilename =
                QDir::fromNativeSeparators(line.mid(6).trimmed());
            m_currentArchiveEntry.clear();
            m_currentArchiveEntry[FileName] = entryFilename;
            m_currentArchiveEntry[InternalID] = entryFilename;
        } else if (line.startsWith(QLatin1String("Size = "))) {
            m_currentArchiveEntry[ Size ] = line.mid(7).trimmed();
        } else if (line.startsWith(QLatin1String("Packed Size = "))) {
            // 7z files only show a single Packed Size value corresponding to the whole archive.
            if (m_archiveType != ArchiveType7z) {
                m_currentArchiveEntry[CompressedSize] = line.mid(14).trimmed();
                // BZip2 archive neobsahuje dalsi info => emit entry
                if (m_archiveType == ArchiveTypeBZip2 ) {
                    const QString entryFilename = QFileInfo(filename()).completeBaseName();
                    m_currentArchiveEntry[FileName] = entryFilename;
                    m_currentArchiveEntry[InternalID] = entryFilename;
                    emit entry(m_currentArchiveEntry);
                    m_uncompSize += m_currentArchiveEntry[Size].toLongLong();
                    m_archiveEntryCount++;
                    if (m_totalEntryCount > 0) {
                        emit totalProgress(100 * m_archiveEntryCount / m_totalEntryCount);
                    }
                }
            }
        } else if (line.startsWith(QLatin1String("Modified = "))) {
            m_currentArchiveEntry[ Timestamp ] =
                QDateTime::fromString(line.mid(11).trimmed(),
                                      QLatin1String( "yyyy-MM-dd hh:mm:ss" ));
        } else if (line.startsWith(QLatin1String("Attributes = "))) {
            const QString attributes = line.mid(13).trimmed();

            const bool isDirectory = attributes.startsWith(QLatin1Char( 'D' ));
            m_currentArchiveEntry[ IsDirectory ] = isDirectory;
            if (isDirectory) {
                const QString directoryName =
                    m_currentArchiveEntry[FileName].toString();
                if (!directoryName.endsWith(QLatin1Char( '/' ))) {
                    const bool isPasswordProtected = (line.at(12) == QLatin1Char( '+' ));
                    m_currentArchiveEntry[FileName] =
                        m_currentArchiveEntry[InternalID] = QString(directoryName + QLatin1Char( '/' ));
                    m_currentArchiveEntry[ IsPasswordProtected ] =
                        isPasswordProtected;
                }
            }

            m_currentArchiveEntry[ Permissions ] = attributes.mid(1);
        } else if (line.startsWith(QLatin1String("CRC = "))) {
            m_currentArchiveEntry[ CRC ] = line.mid(6).trimmed();
            // GZip archive neobsahuje dalsi info => emit entry
            if (m_archiveType == ArchiveTypeGZip && m_currentArchiveEntry.contains(FileName)) {
                emit entry(m_currentArchiveEntry);
                m_uncompSize += m_currentArchiveEntry[Size].toLongLong();
                m_archiveEntryCount++;
                if (m_totalEntryCount > 0) {
                    emit totalProgress(100 * m_archiveEntryCount / m_totalEntryCount);
                }
            }
        } else if (line.startsWith(QLatin1String("Method = "))) {
            m_currentArchiveEntry[ Method ] = line.mid(9).trimmed();
        } else if (line.startsWith(QLatin1String("Encrypted = ")) &&
                   line.size() >= 13) {
            m_currentArchiveEntry[ IsPasswordProtected ] = (line.at(12) == QLatin1Char( '+' ));
        } else if (line.startsWith(QLatin1String("Block = "))) {
            if (m_currentArchiveEntry.contains(FileName)) {
                emit entry(m_currentArchiveEntry);
                m_uncompSize += m_currentArchiveEntry[Size].toLongLong();
                m_archiveEntryCount++;
                if (m_totalEntryCount > 0) {
                    emit totalProgress(100 * m_archiveEntryCount / m_totalEntryCount);
                }
            }
        } else if (line.startsWith(QLatin1String("Comment = "))) {
            m_currentArchiveEntry[ Comment ] = line.mid(10).trimmed();
            // Arj archive => emit entry
            if (m_archiveType == ArchiveTypeArj && m_currentArchiveEntry.contains(FileName)) {
                emit entry(m_currentArchiveEntry);
                m_uncompSize += m_currentArchiveEntry[Size].toLongLong();
                m_archiveEntryCount++;
                if (m_totalEntryCount > 0) {
                    emit totalProgress(100 * m_archiveEntryCount / m_totalEntryCount);
                }
            }
        }
        break;
    }

    return true;
}

void Cli7zPlugin::listModeInit()
{
    m_state = ReadStateHeader;
}


//Testing     ěščřžýáíé.odt     CRC Failed
//Testing     Tučňák.png
//Testing     Vodopád.jpg
//Testing     Složka1/ěščřžýáíé.odt
//Testing     vc90.pdb
//Testing     ui_overwritedialog.h
//Testing     release

bool Cli7zPlugin::readTestLine(const QString &line)
{
    static const QRegExp rxTestingFile( QLatin1String("^(\\bTesting\\b)\\s+(.+)(?:$|\\s+(.+)$)" ));
    if (line.startsWith("Testing")) {
        int pos = rxTestingFile.indexIn(line);
        if (pos > -1) {
            QString fileName = rxTestingFile.cap(2); // "filename.ext       "
            fileName = fileName.trimmed();  // mezery na zacatku/konci pryc
            emit currentFile(fileName);
            //qDebug() << fileName;
            m_archiveEntryCount++;
            if (m_totalEntryCount > 0) {
                emit totalProgress(100 * m_archiveEntryCount / m_totalEntryCount);
            }

            //QString ok = rxTestingFile.cap(3);  // "OK"
            //qDebug() << ok;
        }
    }
    emit testResult(line + '\n');
    return true;
}

bool Cli7zPlugin::analyzeLine(const QByteArray &line)
{
    Q_UNUSED(line)
    return true;
}

bool Cli7zPlugin::analyzeOutput()
{
    return true;
}

bool Cli7zPlugin::analyze(Archive *archive)
{
    Q_UNUSED(archive)
    cacheParameterList();
    m_operationMode = Analyze;
    m_emitEntries = false;
    m_archiveEntryCount = 0;
    m_totalEntryCount = m_archive->entryCount();
    m_uncompSize = 0;

    QStringList args = m_param.value(ListArgs).toStringList();
    prepareListArgs(args);

    if (!runProcess(m_param.value(ListProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return true;
}


void Cli7zPlugin::readStdout(bool handleAll)
{
    Q_ASSERT(m_process);

    if (!m_process->bytesAvailable()) {
        //if process has no more data, we can just bail out
        return;
    }

    //Q_ASSERT(QThread::currentThread() != QApplication::instance()->thread());

    QByteArray dd = m_process->readAllStandardOutput();
    m_stdOutData += dd;
    qDebug() << m_stdOutData;
    QList<QByteArray> lines = m_stdOutData.split('\n');

    bool foundErrorMessage =
        (checkForErrorMessage(QLatin1String( lines.last() ), WrongPasswordPatterns) ||
         checkForErrorMessage(QLatin1String( lines.last() ), ExtractionFailedPatterns) ||
         checkForPasswordPromptMessage(QLatin1String(lines.last())) ||
         checkForFileExistsMessage(QLatin1String( lines.last() )));

    if (foundErrorMessage) {
        handleAll = true;
    }

    // Vystup jen 1 radka, nemusí být kompletní => return
    if (lines.size() == 1 && !handleAll) {
        return;
    }

    if (handleAll) {
        m_stdOutData.clear();
    } else {
        // poslední radka nemusí být kompletní, vrátit zpět, nechat na příště
        m_stdOutData = lines.takeLast();
    }

    foreach(const QByteArray& line, lines) {
        if (m_operationMode == Analyze) {
            analyzeLine(line);
        }
        else {
            handleLine(line);
        }
    }
}

void Cli7zPlugin::handleLine(const QByteArray& lineBa)
{
    QString line = QString::fromLocal8Bit(lineBa);
    //qDebug() << "handleLine:" << line;
    if (!line.isEmpty()) {
        if (checkForPasswordPromptMessage(line)) {
            qDebug() << "Found a password prompt";

            PasswordNeededQuery query(filename());
            emit userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                failOperation();
                return;
            }

            setPassword(query.password());

            const QString response(password() + QLatin1Char('\n'));
            writeToProcess(response.toLocal8Bit());
            return;
        }

        if (checkForErrorMessage(line, WrongPasswordPatterns)) {
            qDebug() << "Wrong password!";
            emit error(tr("Incorrect password."));
            failOperation();
            return;
        }

        if (m_operationMode == Add) {
            if (line.startsWith("Updating archive")) {
                emit currentFile(tr("Updating archive..."));
                emit totalProgress(-1);
            }
            if (line.startsWith("Compressing ")) {
                QString file_name = line.mid(12).trimmed();
                //qDebug() << "Adding file:" << file_name;
                emit currentFile(file_name);
            }

            //read the percentage
            int pos = line.indexOf(QLatin1Char( '%' ));
            if (pos != -1 && pos > 1) {
                int percentage = line.mid(pos - 2, 2).toInt();
                emit totalProgress(percentage);
                return;
            }
        }

        if (m_operationMode == Copy) {
            if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
                qDebug() << "Error in extraction!!";
                emit error(tr("Extraction failed because of an unexpected error."));
                failOperation();
                return;
            }

            if (handleFileExistsMessage(line)) {
                return;
            }

            if (line.startsWith("Extracting ")) {
                QString file_name = line.mid(11).trimmed();
                //qDebug() << "Extractimg file:" << file_name;
                emit currentFile(file_name);
            }else if (line.startsWith("Skipping    ")) {
                QString file_name = line.mid(11).trimmed();
                emit currentFile(tr("Skipping: ") + file_name);
            }

            //read the percentage
            int pos = line.indexOf(QLatin1Char( '%' ));
            if (pos != -1 && pos > 1) {
                int percentage = line.mid(pos - 2, 2).toInt();
                emit totalProgress(percentage);
                return;
            }
        }

        if (m_operationMode == Delete) {
            if (line.startsWith("Updating archive")) {
                emit currentFile(tr("Updating archive..."));
                emit totalProgress(-1);
            }
        }

        if (m_operationMode == List) {
            if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
                qDebug() << "Error in extraction!!";
                emit error(tr("Extraction failed because of an unexpected error."));
                failOperation();
                return;
            }

            readListLine(line);
            return;
        }
    }

    if (m_operationMode == Test) {
        readTestLine(line);
        return;
    }
}
