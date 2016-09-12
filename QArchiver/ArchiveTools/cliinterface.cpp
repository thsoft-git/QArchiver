#include "cliinterface.h"

#include <QDebug>
#include <QTextCodec>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QProcess>
#include <QThread>
#include <QTimer>

#ifdef Q_OS_WIN
#include "textencoder.h"
#endif

#include "qstandarddirs.h"

CliInterface::CliInterface(QObject *parent) :
    ArchiveInterface(parent),
    m_process(0)
{
    // Interface používá eventLoop
    // Interface uses eventloop
    setWaitForFinishedSignal(true);
    m_archiveEntryCount = 0;
    m_totalEntryCount = 0;
    if (QMetaType::type("QProcess::ExitStatus") == 0) {
        qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    }
}

void CliInterface::cacheParameterList()
{
    m_param = parameterList();
    Q_ASSERT(m_param.contains(ExtractProgram));
    Q_ASSERT(m_param.contains(ListProgram));
    Q_ASSERT(m_param.contains(PreservePathSwitch));
    Q_ASSERT(m_param.contains(FileExistsExpression));
    Q_ASSERT(m_param.contains(FileExistsInput));
    //Q_ASSERT(m_param.contains(AutoOverwriteSwitch));
}

CliInterface::~CliInterface()
{
    if (m_process) {
        qDebug() << "Proces state"<< m_process->state();
    }
    Q_ASSERT(!m_process);
}

bool CliInterface::list()
{
    cacheParameterList();
    m_operationMode = List;
    m_archiveEntryCount = 0;
    m_totalEntryCount = m_archive->entryCount();
    m_uncompSize = 0;
    listModeInit();

    QStringList args = m_param.value(ListArgs).toStringList();
    prepareListArgs(args);

    if (!runProcess(m_param.value(ListProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options)
{
    cacheParameterList();
    m_operationMode = Copy;

    // Start preparing the argument list
    QStringList args = m_param.value(ExtractArgs).toStringList();

    // Now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        qDebug() << "Processing argument " << argument;

        if (argument == QLatin1String( "$Archive" )) {
            args[i] = filename();
        }

        if (argument == QLatin1String( "$Files" )) {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                args.insert(i + j, escapeFileName(files.at(j).toString()));
                ++i;
            }
            --i;
        }

        if (argument == QLatin1String( "$EncodingSwitch" )) {
            Q_ASSERT(m_param.contains(EncodingSwitch));

            QString codepage = getArchive()->codePage();

            if (codepage.isEmpty()) {
                args.removeAt(i);
                --i; // decrement to compensate for the variable we removed
            } else {
                QString enc_switch = m_param.value(EncodingSwitch).toString();

                // substitute the $Encoding
                enc_switch.replace(QLatin1String("$Encoding" ), codepage);
                args[i] = enc_switch;
            }
        }

        if (argument == QLatin1String( "$PasswordSwitch" )) {
            Q_ASSERT(m_param.contains(PasswordSwitch));

            // if Archive is password protected and password is empty:
            if (getArchive()->isPasswordProtected() && password().isEmpty()) {
                qDebug() << "Password protected archive, querying user";

                PasswordNeededQuery query(filename());
                emit userQuery(&query);
                query.waitForResponse();

                if (query.responseCancelled()) {
                    failOperation();
                    emit finished(false);
                    return false;
                }
                setPassword(query.password());
            }

            QString pass = password();

            if (pass.isEmpty()) {
                args.removeAt(i);
                --i; // decrement to compensate for the variable we removed
            } else {
                QString newArg = m_param.value(PasswordSwitch).toString();

                //substitute the $Password
                newArg.replace(QLatin1String( "$Password" ), pass);
                //put it in the arg list
                args[i] = newArg;
            }
        }

        if (argument == QLatin1String( "$PreservePathSwitch" )) {
            QStringList replacementFlags = m_param.value(PreservePathSwitch).toStringList();
            Q_ASSERT(replacementFlags.size() == 2);

            bool preservePaths = options.value(QLatin1String( "PreservePaths" )).toBool();
            QString theReplacement;
            if (preservePaths) {
                theReplacement = replacementFlags.at(0);
            } else {
                theReplacement = replacementFlags.at(1);
            }

            if (theReplacement.isEmpty()) {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                // but in this case we don't have to decrement, we just replace it
                args[i] = theReplacement;
            }
        }

        // Automatické přepsání existujících souborů
        if (argument == QLatin1String("$AutoOverwriteSwitch")) {
            if (options.contains(QLatin1String( "AutoOverwrite" ))) {
                int auto_overwrite = options.value(QLatin1String( "AutoOverwrite" )).toInt();
                QStringList replacementFlags = m_param.value(AutoOverwriteSwitch).toStringList();
                args[i] = replacementFlags.at(auto_overwrite);
                qDebug() << "AotoOverwrite existing files, do not query user.";
            }
            else {
                args.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            }
        }

        if (argument == QLatin1String( "$RootNodeSwitch" )) {
            Q_ASSERT(m_param.contains(RootNodeSwitch));

            QString rootNode;
            if (options.contains(QLatin1String( "RootNode" ))) {
                rootNode = options.value(QLatin1String( "RootNode" )).toString();
                qDebug() << "Set root node " << rootNode;
            }

            if (rootNode.isEmpty()) {
                //we will decrement i afterwards
                args.removeAt(i);
                --i; //decrement to compensate for the variable we replaced
            } else {
                QString newArg  = m_param.value(RootNodeSwitch).toString();

                //substitute the $Path
                newArg.replace(QLatin1String( "$Path" ), rootNode);

                //put it in the arg list
                args[i] = newArg;
            }
        }
    } // END for()

    qDebug() << "Setting current dir to " << destinationDirectory;
    bool ok = QDir::setCurrent(destinationDirectory);
    if (ok) {
        qDebug() << "Setting current dir OK";
    } else {
        qDebug() << "Setting current dir Error";
    }

    if (!runProcess(m_param.value(ExtractProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::addFiles(const QStringList & files, const CompressionOptions& options)
{
    cacheParameterList();
    m_operationMode = Add;

    const QString globalWorkDir = options.value(QLatin1String( "GlobalWorkDir" )).toString();
    const QDir workDir = globalWorkDir.isEmpty() ? QDir::current() : QDir(globalWorkDir);
    if (!globalWorkDir.isEmpty()) {
        qDebug() << "GlobalWorkDir is set, changing dir to " << globalWorkDir;
        QDir::setCurrent(globalWorkDir);
    }

    //start preparing the argument list
    QStringList args = m_param.value(AddArgs).toStringList();

    //now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        const QString argument = args.at(i);
        qDebug() << "Processing argument " << argument;

        if (argument == QLatin1String( "$Archive" )) {
            args[i] = filename();
        }

        if (argument == QLatin1String( "$Files" )) {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                const QString relativeName = workDir.relativeFilePath(files.at(j));
                args.insert(i + j, relativeName);
                ++i;
            }
            --i;
        }

        if (argument == QLatin1String( "$PasswordSwitch" )) {
            Q_ASSERT(m_param.contains(PasswordSwitch));

            // if Archive is password protected and password is empty:
            if (getArchive()->isPasswordProtected() && password().isEmpty()) {
                qDebug() << "Password protected archive, querying user";

                PasswordNeededQuery query(filename());
                emit userQuery(&query);
                query.waitForResponse();

                if (query.responseCancelled()) {
                    failOperation();
                    return false;
                }
                setPassword(query.password());
            }

            QString pass = password();

            if (pass.isEmpty()) {
                args.removeAt(i);
                --i; // decrement to compensate for the variable we removed
            } else {
                QString newArg = m_param.value(PasswordSwitch).toString();

                //substitute the $Password
                newArg.replace(QLatin1String( "$Password" ), pass);
                //put it in the arg list
                args[i] = newArg;
            }
        }
    } // END for()

    if (!runProcess(m_param.value(AddProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::deleteFiles(const QList<QVariant> & files)
{
    cacheParameterList();
    m_operationMode = Delete;

    // Start preparing the argument list
    QStringList args = m_param.value(DeleteArgs).toStringList();

    // Now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        qDebug() << "Processing argument " << argument;

        if (argument == QLatin1String( "$Archive" )) {
            args[i] = filename();
        }
        else if (argument == QLatin1String( "$Files" )) {
            args.removeAt(i);
            for (int j = 0; j < files.count(); ++j) {
                args.insert(i + j, escapeFileName(files.at(j).toString()));
                ++i;
            }
            --i;
        }
        else if (argument == QLatin1String( "$PasswordSwitch" )) {
            Q_ASSERT(m_param.contains(PasswordSwitch));

            // if Archive is password protected and password is empty:
            if (getArchive()->isPasswordProtected() && password().isEmpty()) {
                qDebug() << "Password protected archive, querying user";

                PasswordNeededQuery query(filename());
                emit userQuery(&query);
                query.waitForResponse();

                if (query.responseCancelled()) {
                    failOperation();
                    emit finished(false);
                    return false;
                }
                setPassword(query.password());
            }

            QString pass = password();

            if (pass.isEmpty()) {
                args.removeAt(i);
                --i; // decrement to compensate for the variable we removed
            } else {
                QString newArg = m_param.value(PasswordSwitch).toString();

                //substitute the $Password
                newArg.replace(QLatin1String( "$Password" ), pass);
                //put it in the arg list
                args[i] = newArg;
            }
        }
    } // END for()

    m_removedFiles = files;

    if (!runProcess(m_param.value(DeleteProgram).toStringList(), args)) {
        failOperation();
        return false;
    }

    return true;
}

bool CliInterface::test()
{
    cacheParameterList();
    m_operationMode = Test;
    m_archiveEntryCount = 0;
    m_totalEntryCount = m_archive->entryCount();

    // Start preparing the argument list
    QStringList args = m_param.value(TestArgs).toStringList();

    // Now replace the various elements in the list
    for (int i = 0; i < args.size(); ++i) {
        QString argument = args.at(i);
        qDebug() << "Processing argument " << argument;

        if (argument == QLatin1String( "$Archive" )) {
            args[i] = filename();
        }

        if(argument == QLatin1String( "$EncodingSwitch" )) {
            Q_ASSERT(m_param.contains(EncodingSwitch));

            const QString codepage = getArchive()->codePage();
            if (codepage.isEmpty()) {
                args.removeAt(i);
                --i; // decrement to compensate for the variable we removed
            } else {
                QString enc_switch = m_param.value(EncodingSwitch).toString();

                //substitute the $Encoding
                enc_switch.replace(QLatin1String("$Encoding" ), codepage);
                args[i] = enc_switch;
            }
        }

        if (argument == QLatin1String( "$PasswordSwitch" )) {
            Q_ASSERT(m_param.contains(PasswordSwitch));

            // if Archive is password protected and password is empty:
            if (getArchive()->isPasswordProtected() && password().isEmpty()) {
                qDebug() << "Password protected archive, querying user";

                PasswordNeededQuery query(filename());
                emit userQuery(&query);
                query.waitForResponse();

                if (query.responseCancelled()) {
                    failOperation();
                    emit finished(false);
                    return false;
                }
                setPassword(query.password());
            }

            QString pass = password();

            if (pass.isEmpty()) {
                args.removeAt(i);
                --i; // decrement to compensate for the variable we removed
            } else {
                QString newArg = m_param.value(PasswordSwitch).toString();

                //substitute the $Password
                newArg.replace(QLatin1String( "$Password" ), pass);
                //put it in the arg list
                args[i] = newArg;
            }
        }
    } // END for()

    if (!runProcess(m_param.value(ExtractProgram).toStringList(), args)) {
        failOperation();
        emit finished(false);
        return false;
    }

    return true;
}

// Find program by name, and run it
bool CliInterface::runProcess(const QStringList& programNames, const QStringList& arguments)
{
    QString programPath;
    for (int i = 0; i < programNames.count(); i++) {
        programPath = QStandardDirs::findExe(programNames.at(i));
        if (!programPath.isEmpty())
            break;
    }
    qDebug() << "Program path: " << programPath;

    // Debug: program not found.
    //programPath = "";

    if (programPath.isEmpty()) {
        const QString names = programNames.join(QLatin1String(", "));
        emit error(tr("\nFailed to locate program(s) \"%1\" on disk.\n", "@info", programNames.count()).arg(names));

        //and we're finished
        emit finished(false);
        return false;
    }

    qDebug() << "Executing" << programPath << arguments;

    if (m_process) {
        m_process->waitForFinished();
        qDebug() << "Proces State:" <<  m_process->state();
        delete m_process;
        m_process = NULL;
    }

    m_stdOutData.clear();
    m_process = new QProcess;
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, SIGNAL(readyReadStandardOutput()), SLOT(readStdout()), Qt::DirectConnection);
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onProcessFinished(int,QProcess::ExitStatus)), Qt::DirectConnection);

    m_process->start(programPath, arguments, QIODevice::ReadWrite | QIODevice::Unbuffered /*| QIODevice::Text*/);

//    m_process->waitForStarted(-1);
//    while(m_process->state()==QProcess::Running)
//    {
//        QByteArray ba(512, 0);
//        int readed = m_process->read(ba.data(), 512);
//        if (readed > -1) {
//            qDebug() << "ProcesOut:" << QString::fromLocal8Bit(ba.data(), readed);
//        }
//        m_process->waitForReadyRead(50);
//    }

    bool ret = m_process->waitForFinished(-1);

    return ret;

}

void CliInterface::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Process finished. ExitCode:" <<  exitCode << "QProcess::ExitStatus:" << exitStatus;
    if (!m_process) {
        return;
    }

    // Handle all the remaining data in the process
    readStdout(true);

    if (m_operationMode == Delete) {
        foreach(const QVariant& v, m_removedFiles) {
            emit entryRemoved(v.toString());
        }
        int count = m_archive->entryCount();
        count = count - m_removedFiles.count();
        m_archive->setEntryCount(count);
        list();
        return;
    }

    if (m_operationMode == Add) {
        list();
        return;
    }

    if (m_operationMode == Analyze) {
        analyzeOutput();
        m_archive->setEntryCount(m_archiveEntryCount);
    }

    if (m_operationMode == List) {
        m_archive->setEntryCount(m_archiveEntryCount);
        m_archive->setExtractedSize(m_uncompSize);
    }

    // Set progress to 100%
    emit totalProgress(100);

    // Delete process object
    delete m_process;
    m_process = NULL;

    //and we're finished
    //emit finished(true);
    emit finished(exitCode == 0);
}

void CliInterface::failOperation()
{
    doKill();
}

bool CliInterface::checkForPasswordPromptMessage(const QString& line)
{
    const QString passwordPromptPattern(m_param.value(PasswordPromptPattern).toString());

    if (passwordPromptPattern.isEmpty())
        return false;

    if (m_passwordPromptPattern.isEmpty()) {
        m_passwordPromptPattern.setPattern(m_param.value(PasswordPromptPattern).toString());
    }

    if (m_passwordPromptPattern.indexIn(line) != -1) {
        return true;
    }

    return false;
}

bool CliInterface::checkForFileExistsMessage(const QString& line)
{
    if (m_existsPattern.isEmpty()) {
        m_existsPattern.setPattern(m_param.value(FileExistsExpression).toString());
    }
    if (m_existsPattern.indexIn(line) != -1) {
        qDebug() << "Detected file existing!! Filename " << m_existsPattern.cap(1);
        return true;
    }

    return false;
}

bool CliInterface::handleFileExistsMessage(const QString& line)
{
    if (!checkForFileExistsMessage(line)) {
        return false;
    }

    const QString filename = m_existsPattern.cap(1);

    OverwriteQuery query(QDir::current().path() + QLatin1Char( '/' ) + filename);
    query.setNoRenameMode(true);
    emit userQuery(&query);
    qDebug() << "Waiting response";
    query.waitForResponse();

    qDebug() << "Finished response";

    QString responseToProcess;
    const QStringList choices = m_param.value(FileExistsInput).toStringList();

    if (query.responseOverwrite()) {
        responseToProcess = choices.at(0);
    } else if (query.responseSkip()) {
        responseToProcess = choices.at(1);
    } else if (query.responseOverwriteAll()) {
        responseToProcess = choices.at(2);
    } else if (query.responseAutoSkip()) {
        responseToProcess = choices.at(3);
    } else if (query.responseCancelled()) {
        // If the program has no way to cancel the extraction, kill it
        if (choices.count() < 5) {
            return doKill();
        }
        responseToProcess = choices.at(4);
    }

    Q_ASSERT(!responseToProcess.isEmpty());

    responseToProcess += QLatin1Char( '\n' );

    writeToProcess(responseToProcess.toLocal8Bit());

    return true;
}

bool CliInterface::checkForErrorMessage(const QString& line, int parameterIndex)
{
    QList<QRegExp> patterns;

    if (m_patternCache.contains(parameterIndex)) {
        patterns = m_patternCache.value(parameterIndex);
    } else {
        if (!m_param.contains(parameterIndex)) {
            return false;
        }

        foreach(const QString& rawPattern, m_param.value(parameterIndex).toStringList()) {
            patterns << QRegExp(rawPattern);
        }
        m_patternCache[parameterIndex] = patterns;
    }

    foreach(const QRegExp& pattern, patterns) {
        if (pattern.indexIn(line) != -1) {
            return true;
        }
    }
    return false;
}

bool CliInterface::doKill()
{
    if (m_process != NULL) {
        m_process->terminate();
        if (!m_process->waitForFinished(100)) {
            m_process->kill();
        }
        //emit finished(false);
        return true;
    }

    return false;
}

void CliInterface::prepareListArgs(QStringList& params)
{
    for (int i = 0; i < params.size(); ++i) {
        const QString parameter = params.at(i);

        if (parameter == QLatin1String( "$Archive" )) {
            params[i] = filename();
        }
        if(parameter == QLatin1String( "$EncodingSwitch" )) {
            Q_ASSERT(m_param.contains(EncodingSwitch));

            QString encoding = getArchive()->codePage();

            if (encoding.isEmpty()) {
                params.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                QString enc_switch = m_param.value(EncodingSwitch).toString();

                //substitute the $Encoding
                enc_switch.replace(QLatin1String("$Encoding" ), encoding);
                params[i] = enc_switch;
            }
        }
        if (parameter == QLatin1String( "$PasswordSwitch" )) {
            Q_ASSERT(m_param.contains(PasswordSwitch));

            QString pass = password();

            if (pass.isEmpty()) {
                params.removeAt(i);
                --i; //decrement to compensate for the variable we removed
            } else {
                QString pass_switch = m_param.value(PasswordSwitch).toString();

                //substitute the $Password
                pass_switch.replace(QLatin1String("$Password" ), pass);
                params[i] = pass_switch;
            }
        }
    } // END for()
}

QString CliInterface::escapeFileName(const QString& fileName) const
{
    return fileName;
}

void CliInterface::writeToProcess(const QByteArray& data)
{
    Q_ASSERT(m_process);
    Q_ASSERT(!data.isNull());

    qDebug() << "Writing" << data << "to the process";

    m_process->write(data);
}
