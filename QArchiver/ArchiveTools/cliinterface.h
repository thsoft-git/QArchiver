#ifndef CLIINTERFACE_H
#define CLIINTERFACE_H

#include "archiveinterface.h"
#include "queries.h" // pouziva se i v potomcich, proto zde namisto .cpp
#include <QProcess> 

enum CliInterfaceParameters {
    CaptureProgress = 0,
    PasswordPromptPattern,
    ListProgram,
    ListArgs,
    ExtractProgram,
    ExtractArgs,
    NoTrailingSlashes,
    PreservePathSwitch,
    RootNodeSwitch,
    PasswordSwitch,
    EncodingSwitch,
    FileExistsExpression,
    FileExistsMode,
    FileExistsInput,
    DeleteProgram,
    DeleteArgs,
    ExtractionFailedPatterns,
    WrongPasswordPatterns,
    AddProgram,
    AddArgs,
    TestArgs,
    AutoOverwriteSwitch
};

typedef QHash<int, QVariant> ParameterList;

class CliInterface : public ArchiveInterface
{
    Q_OBJECT
public:
    enum OperationMode  {
        List, Copy, Add, Delete, Test, Analyze
    };

    explicit CliInterface(QObject *parent);
    virtual ~CliInterface();

    virtual bool list();
    virtual bool copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options);
    virtual bool addFiles(const QStringList & files, const CompressionOptions& options);
    virtual bool deleteFiles(const QList<QVariant> & files);
    virtual bool test();

    virtual ParameterList parameterList() const = 0;
    bool doKill();

protected:
    virtual void listModeInit() = 0;
    virtual bool analyzeOutput() = 0;

    void prepareListArgs(QStringList& params);
    void cacheParameterList();

    bool checkForPasswordPromptMessage(const QString& line);
    bool checkForFileExistsMessage(const QString& line);
    bool handleFileExistsMessage(const QString& filename);
    bool checkForErrorMessage(const QString& line, int parameterIndex);
    //virtual bool checkForProgress(const QString& line) = 0;

    void failOperation();

    /*
     * Run programName with the given arguments.
     * The method waits until programName is finished to exit.
     */
    virtual bool runProcess(const QStringList& programNames, const QStringList& arguments);
    /*
     * Performs any additional escaping and processing on fileName
     * before passing it to the underlying process.
     */
    virtual QString escapeFileName(const QString &fileName) const;
    void writeToProcess(const QByteArray& data);

    QByteArray m_stdOutData;
    QRegExp m_existsPattern;
    QRegExp m_passwordPromptPattern;
    QHash<int, QList<QRegExp> > m_patternCache;
    OperationMode m_operationMode;
    bool m_emitEntries;
    int m_archiveEntryCount;
    int m_totalEntryCount;
    qint64 m_uncompSize;

    QProcess *m_process;
    ParameterList m_param;
    QVariantList m_removedFiles;

protected slots:
    virtual void readStdout(bool handleAll = false) = 0;
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // CLIINTERFACE_H
