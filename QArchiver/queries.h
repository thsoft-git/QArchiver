#ifndef QUERIES_H
#define QUERIES_H

#include <QString>
#include <QHash>
#include <QWaitCondition>
#include <QMutex>
#include <QVariant>


typedef QHash<QString, QVariant> QueryData;

class Query
{
public:
     /* Execute the query. Must call setResponse when done. */
    virtual void execute(QWidget * parent) = 0;

    /* Will block until the response have been set */
    void waitForResponse();

    QVariant response();

protected:
    Query();
    virtual ~Query() {}

    void setResponse(QVariant response);

    QueryData m_data;

private:
    QWaitCondition m_responseCondition;
    QMutex m_responseMutex;
};

class OverwriteQuery : public Query
{
public:
    explicit OverwriteQuery(const QString& filename);
    void execute(QWidget *parent);
    bool responseCancelled();
    bool responseOverwriteAll();
    bool responseOverwrite();
    bool responseRename();
    bool responseSkip();
    bool responseAutoSkip();
    QString newFilename();

    void setNoRenameMode(bool enableNoRenameMode);
    bool noRenameMode();
    void setMultiMode(bool enableMultiMode);
    bool multiMode();
private:
    bool m_noRenameMode;
    bool m_multiMode;
};

class PasswordNeededQuery : public Query
{
public:
    explicit PasswordNeededQuery(const QString& archiveFilename, bool incorrectTryAgain = false);
    void execute(QWidget *parent);

    bool responseCancelled();
    QString password();
};


class EncodingWarnQuery : public Query
{
public:
    explicit EncodingWarnQuery(const QString& codepage);
    void execute(QWidget *parent);
    bool canContinue();
};

#endif /* ifndef QUERIES_H */
