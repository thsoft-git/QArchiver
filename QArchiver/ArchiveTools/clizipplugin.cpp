#include "clizipplugin.h"

#include "cliinterface.h"

#include <QDebug>

#include <QDateTime>
#include <QDir>
#include <QLatin1String>
#include <QRegExp>
#include <QString>
#include <QStringList>

#include "Codecs/qenca.h"
#include "Codecs/unzipreverseencoder.h"

CliZipPlugin::CliZipPlugin(QObject *parent)
    : CliInterface(parent)
    , m_status(Header)
{
}

CliZipPlugin::~CliZipPlugin()
{
}

QStringList CliZipPlugin::supportedMimetypes(ArchiveOpenMode mode)
{
    switch (mode) {
    case ReadOnly:
        return QStringList() << "application/zip" << "application/x-java-archive";
        break;
    case ReadWrite:
        return QStringList() << "application/zip";
        break;
    default:
        return QStringList();
        break;
    }
}

bool CliZipPlugin::analyze(Archive *archive)
{
    cacheParameterList();
    m_operationMode = Analyze;
    m_archiveEntryCount = 0;
    m_totalEntryCount = m_archive->entryCount();
    outputForAnalyze.clear();

    // "-2" vypiš jen názvy souborů
    // "-h" zobrazit hlavičku, "-z" zip komentář, "-t" total
    QStringList args;
    args << "-2" << "-h" << "-z" << "-t" << archive->fileName();

    if (!runProcess(m_param.value(ListProgram).toStringList(), args)) {
        failOperation();
        emit finished(false);
        return false;
    }

    return true;
}

bool CliZipPlugin::open()
{
    return analyze(m_archive);
}

bool CliZipPlugin::addFiles(const QStringList &files, const CompressionOptions &options)
{
    // The archive contains the names of files encoded in CP852, ....
    // It is not recommended to modify a archive with this encoding by \"zip\" program.
    // Do you really want to continue?

    bool can_continue;

    QString codepage = getArchive()->codePage();
    if ((can_continue = codepage.isEmpty()) == false) {

        EncodingWarnQuery query(codepage);
        emit userQuery(&query);
        query.waitForResponse();

        can_continue = query.canContinue();
    }

    if (can_continue) {
        return CliInterface::addFiles(files, options);
    } else {
        emit finished(false);
        return false;
    }
}

bool CliZipPlugin::deleteFiles(const QList<QVariant> &files)
{
    bool can_continue;

    QString codepage = getArchive()->codePage();
    if ((can_continue = codepage.isEmpty()) == false) {

        EncodingWarnQuery query(codepage);
        emit userQuery(&query);
        query.waitForResponse();

        can_continue = query.canContinue();
    }

    if (can_continue) {
        return CliInterface::deleteFiles(files);
    } else {
        emit finished(false);
        return false;
    }
}

QString CliZipPlugin::escapeFileName(const QString &fileName) const
{
    // Některé znaky maji speciálni vyznam, přidat '\'
    // Soubor: match.c ve zdrojovem kodu InfoZip
    const QString escapedCharacters(QLatin1String("[]*?^-\\!"));

    QString quoted;
    const int len = fileName.length();
    const QLatin1Char backslash('\\');
    quoted.reserve(len * 2);

    for (int i = 0; i < len; ++i) {
        if (escapedCharacters.contains(fileName.at(i))) {
            quoted.append(backslash);
        }

        quoted.append(fileName.at(i));
    }

    return quoted;
}

bool CliZipPlugin::isReadOnly() const
{
    return false;
}


ParameterList CliZipPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        p[ListProgram] = QStringList() << QLatin1String( "zipinfo" );
        p[ExtractProgram] = QStringList() << QLatin1String( "unzip" );
        p[DeleteProgram] = p[AddProgram] = QStringList() << QLatin1String( "zip" );

        p[ListArgs] = QStringList() << QLatin1String( "$EncodingSwitch" ) << QLatin1String( "-l" ) << QLatin1String( "-T" ) << "-h" << "-t" << QLatin1String( "$Archive" );

        p[ExtractArgs] = QStringList() << QLatin1String( "$EncodingSwitch" ) << QLatin1String( "$PreservePathSwitch" )
                                       << QLatin1String( "$PasswordSwitch" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );
        p[PreservePathSwitch] = QStringList() << QLatin1String( "" ) << QLatin1String( "-j" );
        p[PasswordSwitch] = QString( "-P$Password" );
        p[EncodingSwitch] = QString( "-O $Encoding" );

        p[DeleteArgs] = QStringList() << QLatin1String( "-d" ) << QLatin1String( "$PasswordSwitch" )
                                      << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );

        p[FileExistsExpression] = QLatin1String( "^replace (.+)\\?" );
        p[FileExistsInput] = QStringList()
                             << QLatin1String( "y" ) //overwrite
                             << QLatin1String( "n" ) //skip
                             << QLatin1String( "A" ) //overwrite all
                             << QLatin1String( "N" ) //autoskip
                             ;

        p[AddArgs] = QStringList() << QLatin1String( "-r" ) << QLatin1String( "$PasswordSwitch" )
                                   << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );

        p[PasswordPromptPattern] = QLatin1String(" password: ");
        p[WrongPasswordPatterns] = QStringList() << QLatin1String( "incorrect password" );
        p[ExtractionFailedPatterns] = QStringList() << "bad CRC";

        p[TestArgs] = QStringList()  << QLatin1String("-t") << QLatin1String( "$EncodingSwitch" )
                                     << QLatin1String( "$PasswordSwitch" ) << QLatin1String( "$Archive" );
    }
    return p;
}


void CliZipPlugin::readStdout(bool handleAll)
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

    foreach(const QByteArray& line, lines)
    {
        handleLine(line);
    }
}


bool CliZipPlugin::analyzeLine(const QByteArray &line)
{
    switch (m_status) {
    case Header:
        m_status = Entry;
        break;
    case Entry:
        UnzipReverseEncoder encoder;
        //QByteArray ba(line);
        //encoder.reverse(ba.data());
        //outputForAnalyze.append(ba);
        outputForAnalyze.append(encoder.reverse1(line));
        outputForAnalyze.append('\n');
        break;
    }

    return true;
}


bool CliZipPlugin::analyzeOutput()
{
    QEnca enca;
    if (enca.analyse(outputForAnalyze)) {
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
            //emit charset(enca.charsetName(ENCA_NAME_STYLE_ICONV), enca.charsetName(ENCA_NAME_STYLE_HUMAN));
            emit encodingInfo(tr("Filename encoding is not in current locale codepage! QArchiver detected: %1."
                                 "If it is not valid encoding of archive, please select correct encoding from menu.")
                              .arg(QString::fromLocal8Bit(enca.charsetName(ENCA_NAME_STYLE_HUMAN))));

            m_archive->setCodePage(QString::fromLocal8Bit(enca.charsetName(ENCA_NAME_STYLE_ICONV)));
        } else {
            m_archive->setCodePage("");
        }
        return true;
    } else {
        emit encodingInfo(tr("Filename encoding is not in current locale codepage!"
                             " QArchiver can't detect the encoding: %1.").arg(enca.strError()));
    }
    return false;
}

/*
Archive:  czFileNames-OS-win.zip
Zip file size: 548499 bytes, number of entries: 7
-rw-a--     2.0 fat     7334 b-     5541 defN 20150117.130024 ?????????.odt
-rw-a--     2.0 fat    63546 b-    59617 defN 20150323.213646 Tu???k.png
-rw-a--     2.0 fat    71265 b-    71141 defN 20150323.213744 Vodop?d.jpg
-rw-a--     2.0 fat     7334 b-     5541 defN 20150117.130024 Slo?ka1/?????????.odt
-rw-a--     3.0 fat  1388544 bx   404742 defN 20150610.042826 vc90.pdb
-rw-a--     3.0 fat     2003 tx      736 defN 20150610.034404 ui_overwritedialog.h
drwx---     3.0 fat        0 bx        0 stor 20150403.164125 release/
7 files, 1540026 bytes uncompressed, 547318 bytes compressed:  64.5%
*/

bool CliZipPlugin::readListLine(const QString &line)
{
    static const QRegExp entryPattern("^(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\d{8}).(\\d{6})\\s+(.+)$");
    // exactMatch() přidává ^ a $ automaticky
    static const QRegExp hdrFileInfo("Zip file size:\\s+(\\d+) bytes, number of entries:\\s(\\d+)");
    static const QRegExp archiveTotal("(\\d+) files, (\\d+) bytes uncompressed, (\\d+) bytes compressed:\\s+([0-9]*\\.?[0-9]*)%");

    switch (m_status) {
    case Header:
        if (line.startsWith("Archive:")) {
            // První řádek, hlavičky -> ignorovat
            m_status = Header;
            break;
        } else if (hdrFileInfo.exactMatch(line)) {
            //int zip_size = hdrFileInfo.cap(1).toInt();
            int num_entries = hdrFileInfo.cap(2).toInt();
            //m_archive->setSize(zip_size);
            m_archive->setEntryCount(num_entries);
            //qDebug() << "Header line 2:" << zip_size << num_entries;
            m_status = Entry;
            break;
        }
        m_status = Entry;
    case Entry:
        if (entryPattern.indexIn(line) != -1) {
            ArchiveEntry e;
            e[Permissions] = entryPattern.cap(1);
            // V některých zip souborech chybí atribut 'd' u adresářů
            // => použít koncový znak '/' namísto atributu 'd'.
            e[IsDirectory] = entryPattern.cap(10).endsWith(QLatin1Char('/'));
            e[Size] = entryPattern.cap(4).toInt();
            QString status = entryPattern.cap(5);
            if (status[0].isUpper()) {
                e[IsPasswordProtected] = true;
            }
            e[CompressedSize] = entryPattern.cap(6).toInt();

            const QDateTime ts(QDate::fromString(entryPattern.cap(8), QLatin1String( "yyyyMMdd" )),
                               QTime::fromString(entryPattern.cap(9), QLatin1String( "hhmmss" )));
            e[Timestamp] = ts;

            e[FileName] = e[InternalID] = entryPattern.cap(10);
            emit entry(e);
            m_uncompSize += e[Size].toLongLong();
            m_archiveEntryCount++;
        }
        else if (archiveTotal.exactMatch(line)) {
            int num_entries = archiveTotal.cap(1).toInt();
            qint64 bytes_uncompressed = archiveTotal.cap(2).toLongLong();
            qint64 bytes_compressed = archiveTotal.cap(3).toLongLong();
            //float compression = archiveTotal.cap(4).toFloat();
            //qDebug() << archiveTotal.cap(0) << num_entries << bytes_uncompressed << bytes_compressed << compression << QString().setNum(100.0F - (100.0F * bytes_compressed / bytes_uncompressed),'f', 2);
            m_archive->setEntryCount(num_entries);
            m_archive->setExtractedSize(bytes_uncompressed);
            m_archive->setCompressedSize(bytes_compressed);
        }
        //else {qDebug() << "Entry no match";}
        break;
    }

    return true;
}

bool CliZipPlugin::readTestLine(const QString &line)
{
    static const QRegExp rxTestingFile( QLatin1String("^\\s+(\\S+)\\s+(.+)\\s+(.+)$") );

    if (line.startsWith(QLatin1String("    testing:"))) {
        int pos = rxTestingFile.indexIn(line);
        if (pos > -1) {
            QString fileName = rxTestingFile.cap(2); // "filename.ext       "
            fileName = fileName.trimmed();  // mezery na zacatku/konci pryc
            emit currentFile(fileName);
            qDebug() << fileName;

            QString ok = rxTestingFile.cap(3);  // "OK"
            qDebug() << ok;
        }

        emit testResult(line);
        return true;
    }
    emit testResult(line);

    return true;
}


void CliZipPlugin::handleLine(const QByteArray& lineBa)
{
    QString line = QString::fromLocal8Bit(lineBa);
    qDebug() << "CliZipPlugin::handleLine(" << line << ")";

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

    if (m_operationMode == Analyze)
    {
        analyzeLine(lineBa);
    }
    else if (m_operationMode == Add)
    {
        if (line.startsWith("  adding:")) {
            int startPos = 9;
            int endPos = line.length()-1;
            if (line.contains("(deflated ")) {
                endPos = line.indexOf("(deflated ");
            } else if (line.contains("(stored ")) {
                endPos = line.indexOf("(stored ");
            }
            QString fileName = line.mid(startPos, endPos - startPos).trimmed();
            emit currentFile(fileName);
            qDebug() << fileName;
        }
        else if (line.startsWith("updating:")) {
            int startPos = 9;
            int endPos = line.length()-1;
            if (line.contains("(deflated ")) {
                endPos = line.indexOf("(deflated ");
            } else if (line.contains("(stored ")) {
                endPos = line.indexOf("(stored ");
            }
            QString fileName = line.mid(startPos, endPos - startPos).trimmed();
            emit currentFile(fileName);
            qDebug() << fileName;
        }
    }
    else if (m_operationMode == Copy)
    {
        if (handleFileExistsMessage(line)) {
            return;
        }
        if (line.startsWith("  inflating:")) {
            int startPos = 12;
            QLatin1String badCRC("bad CRC");
            QString fileName;
            if (line.contains(badCRC)) {
                int endPos = line.indexOf(badCRC);
                fileName = line.mid(startPos, endPos - startPos).trimmed();
                emit currentFile(fileName);
                emit error(tr("CRC error detected while extracting the file: %1").arg(fileName));
            } else {
                 fileName = line.mid(startPos).trimmed();
                 emit currentFile(fileName);
            }
        }
        return;
    }
    else if (m_operationMode == List)
    {
        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            qDebug() << "Error in extraction!!";
            emit error(tr("Extraction failed because of an unexpected error."));
            failOperation();
            return;
        }

        readListLine(line);
        return;
    }
    else if (m_operationMode == Test)
    {
        readTestLine(line + '\n');
        return;
    }
}

void CliZipPlugin::listModeInit()
{
    m_status = Header;
}
