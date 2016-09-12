#ifndef JOBINTERFACE_H
#define JOBINTERFACE_H

#include <QObject>
#include <QPair>

/**
 * QJob
 */
class QJob : public QObject
{
    Q_OBJECT
    Q_ENUMS( KillVerbosity Capability Unit )
    Q_FLAGS( Capabilities )
public:
    enum KillVerbosity { Quietly, EmitResult };

    enum Unit { Bytes, Files, Directories };

    enum Capability { NoCapabilities = 0x0000,
                      Killable = 0x0001,
                      Suspendable = 0x0002 };

    Q_DECLARE_FLAGS( Capabilities, Capability )

    explicit QJob(QObject *parent = 0);
    virtual ~QJob();

    virtual void start()=0;

    Capabilities capabilities() const;

    enum
    {
        /*** Indicates there is no error */
        NoError = 0,
        /*** Indicates the job was killed */
        KilledJobError = 1,
        /*** Subclasses should define error codes starting at this value */
        UserDefinedError = 100
    };

    int error() const;
    QString errorText() const;
    virtual QString errorString() const;
    unsigned long percent() const;

    bool isAutoDelete() const;
    void setAutoDelete(bool autoDelete);

public slots:
    bool kill(KillVerbosity verbosity=Quietly);

protected:
    virtual bool doKill();
    virtual void emitResult();
    void setError(int errorCode);
    void setErrorText(const QString &errorText);
    void setPercent(unsigned long percentage);

    void setCapabilities(Capabilities capabilities);

signals:
    void started(QJob* job);
    void finished(QJob* job);
    void suspended(QJob *job);
    void resumed(QJob *job);
    void result(QJob *job);

    void description(QJob *job, const QString &title, const QPair< QString, QString > &field1=qMakePair(QString(), QString()), const QPair< QString, QString > &field2=qMakePair(QString(), QString()));
    void infoMessage(QJob *job, const QString &plain, const QString &rich=QString());
    void warning(QJob *job, const QString &plain, const QString &rich = QString());

    void totalAmount(QJob *job, QJob::Unit unit, qulonglong amount);
    void processedAmount(QJob *job, QJob::Unit unit, qulonglong amount);
    void totalSize(QJob *job, qulonglong size);
    void processedSize(QJob *job, qulonglong size);
    void percent(QJob *job, unsigned long percent);
    void speed(QJob *job, unsigned long speed);

public slots:

protected slots:

private:
    Capabilities m_capabilities;
    int m_error;
    QString m_errorText;
    bool m_autoDelete;
    bool m_finished;
    unsigned long m_percentage;
    
};

Q_DECLARE_OPERATORS_FOR_FLAGS( QJob::Capabilities )

/**
 * QMultijob -> QJob with subjobs
 */
class QMultiJob : public QJob
{
    Q_OBJECT
public:
    explicit QMultiJob(QObject *parent = 0);
    virtual ~QMultiJob();

protected:
    virtual bool addSubjob(QJob *job);
    virtual bool removeSubjob(QJob *job);
    bool hasSubjobs();
    const QList<QJob *> &subjobs() const;
    void clearSubjobs();

protected slots:
    virtual void onSubjobResult(QJob *job);
    virtual void onInfoMessage(QJob *job, const QString &plain, const QString &rich);

signals:

public slots:

private:
    QList<QJob*> m_subjobs;
};

#endif // JOBINTERFACE_H
