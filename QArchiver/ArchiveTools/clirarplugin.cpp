#include "clirarplugin.h"

#include <QDebug>

#include <QDateTime>
#include <QDir>
#include <QString>
#include <QStringList>
#ifdef Q_OS_WIN
#include "textencoder.h"
#endif

CliRarPlugin::CliRarPlugin(QObject *parent)
        : CliInterface(parent)
        , m_parseState(ParseStateColumnDescription1)
        , m_isPasswordProtected(false)
        , m_remainingIgnoredSubHeaderLines(0)
        , m_isUnrarFree(false)
{
    m_emitEntries = true;
}

CliRarPlugin::~CliRarPlugin()
{
}

QStringList CliRarPlugin::supportedMimetypes(ArchiveOpenMode mode)
{
    Q_UNUSED(mode)
    return QStringList() << "application/x-rar";
}

QString CliRarPlugin::escapeFileName(const QString &fileName) const
{
    if (fileName.endsWith(QLatin1Char('/'))) {
        return fileName.left(fileName.length() - 1);
    }

    return fileName;
}

bool CliRarPlugin::isReadOnly() const
{
    return false;
}

ParameterList CliRarPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        p[ListProgram] = p[ExtractProgram] = QStringList() << QLatin1String( "unrar" );
        p[DeleteProgram] = p[AddProgram] = QStringList() << QLatin1String( "rar" );

        p[ListArgs] = QStringList() << QLatin1String( "vt" ) << QLatin1String( "-c-" ) << QLatin1String( "-v" )
                                    << QLatin1String("$PasswordSwitch") << QLatin1String( "$Archive" );
        p[ExtractArgs] = QStringList() << QLatin1String( "-kb" )        // << QLatin1String( "-p-" )
                                       << QLatin1String( "$PreservePathSwitch" )
                                       << QLatin1String( "$PasswordSwitch" )
                                       << QLatin1String( "$RootNodeSwitch" )
                                       << QLatin1String( "$Archive" )
                                       << QLatin1String( "$Files" );
        p[PreservePathSwitch] = QStringList() << QLatin1String( "x" ) << QLatin1String( "e" );
        p[RootNodeSwitch] = QString( "-ap$Path" );
        p[PasswordSwitch] = QString( "-p$Password" );

        p[DeleteArgs] = QStringList() << QLatin1String( "d" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );

        p[FileExistsExpression] = QLatin1String( "^(.+) already exists. Overwrite it" );
        p[FileExistsInput] = QStringList()
                             << QLatin1String( "Y" ) //overwrite
                             << QLatin1String( "N" ) //skip
                             << QLatin1String( "A" ) //overwrite all
                             << QLatin1String( "E" ) //autoskip
                             << QLatin1String( "Q" ) //cancel
                             ;

        p[AddArgs] = QStringList() << QLatin1String( "a" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );

        p[PasswordPromptPattern] = QLatin1String("Enter password \\(will not be echoed\\) for");

        p[WrongPasswordPatterns] = QStringList() << QLatin1String("password incorrect") << QLatin1String("wrong password");
        p[ExtractionFailedPatterns] = QStringList() << QLatin1String( "CRC failed" ) << QLatin1String( "Cannot find volume" );

        p[TestArgs] = QStringList() << QLatin1String("t") << QLatin1String("$PasswordSwitch") << QLatin1String( "$Archive" );
    }

    return p;
}

bool CliRarPlugin::analyze()
{
    cacheParameterList();
    m_operationMode = Analyze;
    m_emitEntries = false;
    m_archiveEntryCount = 0;
    m_totalEntryCount = m_archive->entryCount();

    QStringList args = m_param.value(ListArgs).toStringList();
    prepareListArgs(args);

    if (!runProcess(m_param.value(ListProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliRarPlugin::open()
{
    return analyze();
}

bool CliRarPlugin::readListLine(const QString &line)
{
    static const QLatin1String headerString("----------------------");
    static const QLatin1String subHeaderString("Data header type: ");
    static const QLatin1String columnDescription1String("                  Size   Packed Ratio  Date   Time     Attr      CRC   Meth Ver");
    static const QLatin1String columnDescription2String("               Host OS    Solid   Old"); // Jen v unrar-nonfree

    switch (m_parseState)
    {
    case ParseStateColumnDescription1:
        if (line.startsWith(columnDescription1String)) {
            m_parseState = ParseStateColumnDescription2;
        }

        break;

    case ParseStateColumnDescription2:
        if (line.startsWith(columnDescription2String)) {
            m_parseState = ParseStateHeader;
        } else if (line.startsWith(headerString)) {
            m_parseState = ParseStateEntryFileName;
            m_isUnrarFree = true;
        }

        break;

    case ParseStateHeader:
        if (line.startsWith(headerString)) {
            m_parseState = ParseStateEntryFileName;
        }

        break;

    case ParseStateEntryFileName:
        if (m_remainingIgnoredSubHeaderLines > 0) {
            --m_remainingIgnoredSubHeaderLines;
            return true;
        }

        if (line.startsWith(subHeaderString)) {
            // subHeaderString's length is 18
            const QString subHeaderType(line.mid(18));

            if (subHeaderType == QLatin1String("STM")) {
                m_remainingIgnoredSubHeaderLines = 4;
            } else {
                m_remainingIgnoredSubHeaderLines = 3;
            }

            qDebug() << "Found a subheader of type" << subHeaderType;
            qDebug() << "The next" << m_remainingIgnoredSubHeaderLines
                     << "lines will be ignored";

            return true;
        } else if (line.startsWith(headerString)) {
            m_parseState = ParseStateHeader;

            return true;
        }

        m_isPasswordProtected = (line.at(0) == QLatin1Char( '*' ));

        // Start from 1 because the first character is either ' ' or '*'
        m_entryFileName = QDir::fromNativeSeparators(line.mid(1));

        m_parseState = ParseStateEntryDetails;

        break;

    case ParseStateEntryIgnoredDetails:
        m_parseState = ParseStateEntryFileName;

        break;

    case ParseStateEntryDetails:
        if (line.startsWith(headerString)) {
            m_parseState = ParseStateHeader;
            return true;
        }

        const QStringList details = line.split(QLatin1Char( ' ' ), QString::SkipEmptyParts);

        QDateTime ts(QDate::fromString(details.at(3), QLatin1String("dd-MM-yy")),
                     QTime::fromString(details.at(4), QLatin1String("hh:mm")));

        // unrar outputs dates with a 2-digit year but QDate takes it as 19??
        // let's take 1950 is cut-off; similar to KDateTime
        if (ts.date().year() < 1950) {
            ts = ts.addYears(100);
        }

        bool isDirectory = ((details.at(5).at(0) == QLatin1Char( 'd' )) ||
                            (details.at(5).at(1) == QLatin1Char( 'D' )));
        if (isDirectory && !m_entryFileName.endsWith(QLatin1Char( '/' ))) {
            m_entryFileName += QLatin1Char( '/' );
        }

        // If the archive is a multivolume archive, a string indicating
        // whether the archive's position in the volume is displayed
        // instead of the compression ratio.
        QString compressionRatio = details.at(2);
        if ((compressionRatio == QLatin1String("<--")) ||
            (compressionRatio == QLatin1String("<->")) ||
            (compressionRatio == QLatin1String("-->"))) {
            compressionRatio = QLatin1String("0");
        } else {
            compressionRatio.chop(1); // Remove the '%'
        }

        ArchiveEntry e;
        e[FileName] = m_entryFileName;
        e[InternalID] = m_entryFileName;
        e[Size] = details.at(0);
        e[CompressedSize] = details.at(1);
        // ToDo:
        // unrar zobrazuje kompresní poměr jako ((compressed size * 100) / size);
        // ostatní programy jako (100 * ((size - compressed size) / size)).
        e[Ratio] = compressionRatio;
        e[Timestamp] = ts;
        e[IsDirectory] = isDirectory;
        e[Permissions] = details.at(5);
        e[CRC] = details.at(6);
        e[Method] = details.at(7);
        e[Version] = details.at(8);
        e[IsPasswordProtected] = m_isPasswordProtected;
        qDebug() << "CliRarPlugin::readListLine()\n" << "Added entry: " << e << "\n";

        if (m_emitEntries) {
            emit entry(e);
        }
        m_uncompSize += e[Size].toLongLong();
        m_archiveEntryCount++;

        if (m_isUnrarFree) {
            // unrar-free nevypisuje 3. řádku informací u souborů.
            m_parseState = ParseStateEntryFileName;
        } else {
            m_parseState = ParseStateEntryIgnoredDetails;
        }

        break;
    }

    return true;
}

bool CliRarPlugin::readTestLine(const QString &line)
{
    QChar backspace = QChar('\b');

    if (line.startsWith(QLatin1String("Testing     "))) {
        QString fileName;
        const QChar* lineData = line.constData();
        lineData+=12; // Zacatek na znaku 12
        // Nazev souboru se nachazi mezi 12 znakem a prvnim BS
        while (!lineData->isNull() && *lineData != backspace) {
            //qDebug() << *lineData;
            fileName.append(*lineData);
            ++lineData;
        }
        fileName = fileName.trimmed();
        emit currentFile(fileName);
        //qDebug() << fileName << fileName.capacity() << line.size();

        QChar*const resultLine = new QChar[line.size()+1]();
        QChar* actual_char = resultLine;
        const QChar* data = line.constData();

        while(!data->isNull()) {
            if (*data==backspace) {
                *--actual_char = QChar('\0');
            }else {
                *actual_char = *data;
                actual_char++;
            }
            data++;
        }
        *actual_char = QChar('\0');
        QString stringLine(resultLine);
        stringLine.append('\n');
        qDebug() << "SL:"<< stringLine;
        //stringLine.replace(QRegExp("\\s{24}"),"\t");
        delete[]  resultLine;

        emit testResult(stringLine);
        return true;
    }
    return false;
}

bool CliRarPlugin::analyzeLine(const QByteArray &line)
{
    qDebug() << "CliRarPlugin::analyzeLine()";
    qDebug() << line;

    QString sLine = QString::fromLocal8Bit(line);
    if (sLine.isEmpty()) {
        return true;
    }
    if (checkForPasswordPromptMessage(sLine)) {
        haldlePasswordPromptMessage(sLine);
        return true;
    }

    if (checkForErrorMessage(sLine, WrongPasswordPatterns)) {
        qDebug() << "Wrong password!";
        emit error(tr("Incorrect password."));
        failOperation();
        return true;
    }

    return readListLine(sLine);
}

bool CliRarPlugin::analyzeOutput()
{
    m_archive->setCodePage("");
    return true;
}

void CliRarPlugin::readStdout(bool handleAll)
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

void CliRarPlugin::handleLine(const QByteArray &lineBa)
{
    QString line = QString::fromLocal8Bit(lineBa);
    qDebug() << "CliRarPlugin::handleLine(" + line + ")";

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

    if (m_operationMode == Analyze) {
        analyzeLine(lineBa);
    }

    if (!line.isEmpty())
    {
        if (m_operationMode == Copy || m_operationMode == Add) {
            QChar backspace = QChar('\b');

            if (line.startsWith(QLatin1String("Extracting  "))) {
                QString fileName;
                const QChar *lineData = line.constData();
                lineData+=12; // Zacatek na znaku 12
                // Nazev souboru se nechazi mezi 12 znakem a prvnim BS
                while (!lineData->isNull() && *lineData != backspace) {
                    //qDebug() << *lineData;
                    fileName.append(*lineData);
                    ++lineData;
                }
                fileName = fileName.trimmed();
                emit currentFile(fileName);
                //qDebug() << fileName;
            }
            else if (line.startsWith(QLatin1String("Adding    "))) {
                QString fileName;
                const QChar *lineData = line.constData();
                lineData+=10;
                // Nazev souboru je mezi 10 znakem a prvnim BS
                while (!lineData->isNull() && *lineData != backspace) {
                    qDebug() << *lineData;
                    fileName.append(*lineData);
                    ++lineData;
                }
                fileName = fileName.trimmed();
                emit currentFile(fileName);
                qDebug() << fileName;
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


        if (readTestLine(line)) {
            return;
        }
        emit testResult(line + '\n');
    }
}

void CliRarPlugin::listModeInit()
{
    m_parseState = ParseStateColumnDescription1;
    m_emitEntries = true;
}

void CliRarPlugin::haldlePasswordPromptMessage(const QString &line)
{
    Q_UNUSED(line);
    qDebug() << "Found a password prompt";

    PasswordNeededQuery query(filename());
    emit userQuery(&query);
    query.waitForResponse();

    if (query.responseCancelled()) {
        failOperation();
        return;
    }

    setPassword(query.password());

    // TODO: na OS Win chyba i se spravnym heslem.
    const QString response(password() + QLatin1Char('\n'));
    writeToProcess(response.toLocal8Bit());

}
