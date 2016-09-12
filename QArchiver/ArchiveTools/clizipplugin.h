#ifndef CLIZIPPLUGIN_H
#define CLIZIPPLUGIN_H

#include "cliinterface.h"

class CliZipPlugin : public CliInterface
{
    Q_OBJECT
public:
    explicit CliZipPlugin(QObject *parent);
    virtual ~CliZipPlugin();
    static QStringList supportedMimetypes(ArchiveOpenMode mode = ReadOnly);
    virtual bool open();
    virtual bool addFiles(const QStringList & files, const CompressionOptions& options);
    virtual bool deleteFiles(const QList<QVariant> & files);
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
    bool analyze(Archive *archive);
    bool analyzeLine(const QByteArray &line);
    bool readListLine(const QString &line);
    bool readTestLine(const QString &line);
    void handleLine(const QByteArray& lineBa);

    enum {
        Header = 0,
        Entry
    } m_status;

    QByteArray outputForAnalyze;
};

#endif // CLIZIPPLUGIN_H
