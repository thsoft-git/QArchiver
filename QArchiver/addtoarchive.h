#ifndef ADDTOARCHIVE_H
#define ADDTOARCHIVE_H

#include "jobinterface.h"
#include <QObject>
#include <QStringList>
#include <QVariantList>

class Job;
class ProgressDialog;

class AddToArchive : public QJob
{
    Q_OBJECT
public:
    explicit AddToArchive(QObject *parent = 0);
    ~AddToArchive();

    bool showAddDialog();
    void setStorePaths(bool value);
    void connectDialog(ProgressDialog *dialog);

signals:

public slots:
    void addInput(const QString& path);
    void addFiles(const QVariantList& paths);
    void setAutoFilenameSuffix(const QString& suffix);
    void setFilename(const QString &path);
    void setMimeType(const QString& mimeType);
    void start();

private slots:
    void slotFinished(QJob* job);
    void slotStartJob(void);

private:
    ProgressDialog* m_dialog;
    QString m_filename;
    QString m_strippedPath;
    QString m_autoFilenameSuffix;
    QString m_firstPath;
    QString m_mimeType;
    QStringList m_inputs;
    bool m_storePaths;
};

#endif // ADDTOARCHIVE_H
