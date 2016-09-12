#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>

#include "jobinterface.h"

namespace Ui {
class ProgressDialog;
}

class ProgressDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ProgressDialog(QWidget *parent = 0);
    ~ProgressDialog();
    void resetUi();

public slots:
    void setCurrentLabelText(const QString &text);
    void setCurrentProgress(int value);
    void setTotalDescription(const QString &text);
    void setTotalProgress(int value);
    void setTotalProgress(QJob *job, ulong value);

    void onJobStarted(QJob *job);
    void onJobFinished(QJob *job);

    void unregisterJob(QJob *job);
    void registerJob(QJob *job);

    void jobFinished();
    void onJobPercent(QJob *job, unsigned long  percent);
    void setDescription(QJob *job, const QString &title, const QPair< QString, QString > &f1, const QPair< QString, QString > &f2);
    void setInformation(QJob *job, const QString &plain, const QString &rich);

private slots:
    void setCompletedValue(int value);
    void updateElapsed();
    void updateTimeLeft();
private:
    Ui::ProgressDialog *ui;
    QTimer *timer;
    QPixmap icon;
    long int elapsed;
};

#endif // PROGRESSDIALOG_H
