#ifndef CLIRARPLUGIN_H
#define CLIRARPLUGIN_H

#include "cliinterface.h"

class CliRarPlugin : public CliInterface
{
    Q_OBJECT

public:
    explicit CliRarPlugin(QObject *parent);
    virtual ~CliRarPlugin();
    static QStringList supportedMimetypes(ArchiveOpenMode mode = ReadOnly);
    virtual bool open();
    virtual QString escapeFileName(const QString &fileName) const;
    virtual bool isReadOnly() const;

signals:
    
public slots:

protected:
    virtual ParameterList parameterList() const;
    virtual void listModeInit();
    virtual bool analyzeOutput();

protected slots:
    virtual void readStdout(bool handleAll);

private:
    bool analyze();
    bool analyzeLine(const QByteArray &line);
    bool readListLine(const QString &line);
    bool readTestLine(const QString &line);
    void handleLine(const QByteArray& lineBa);
    void haldlePasswordPromptMessage(const QString& line);

    enum {
        ParseStateColumnDescription1 = 0,
        ParseStateColumnDescription2,
        ParseStateHeader,
        ParseStateEntryFileName,
        ParseStateEntryDetails,
        ParseStateEntryIgnoredDetails
    } m_parseState;

    QString m_entryFileName;
    bool m_isPasswordProtected;

    int m_remainingIgnoredSubHeaderLines;

    bool m_isUnrarFree;
};

#endif // CLIRARPLUGIN_H
