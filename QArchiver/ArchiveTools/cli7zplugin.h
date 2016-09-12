#ifndef CLI7ZPLUGIN_H
#define CLI7ZPLUGIN_H

#include "cliinterface.h"

class Cli7zPlugin : public CliInterface
{
    Q_OBJECT
public:
    explicit Cli7zPlugin(QObject *parent);
    virtual ~Cli7zPlugin();
    static QStringList readOnlyMimeTypes();
    static QStringList supportedMimetypes(ArchiveOpenMode mode = ReadOnly);
    virtual bool open();
    virtual bool isReadOnly() const;

protected:
    virtual ParameterList parameterList() const;
    virtual void listModeInit();
    virtual bool analyzeOutput();

protected slots:
    virtual void readStdout(bool handleAll);

private:
    bool analyze(Archive *archive);
    bool analyzeLine(const QByteArray &line);
    bool readListLine(const QString &line);
    bool readTestLine(const QString &line);
    void handleLine(const QByteArray& lineBa);

    enum ArchiveType {
    ArchiveType7z = 0,
    ArchiveTypeBZip2,
    ArchiveTypeGZip,
    ArchiveTypeTar,
    ArchiveTypeZip,
    ArchiveTypeArj
    };

    enum ReadState {
    ReadStateHeader = 0,
    ReadStateArchiveInformation,
    ReadStateEntryInformation
    };

    ArchiveType m_archiveType;
    ArchiveEntry m_currentArchiveEntry;
    ReadState m_state;
};

#endif // CLI7ZPLUGIN_H
