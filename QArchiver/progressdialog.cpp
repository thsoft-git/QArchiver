#include "progressdialog.h"
#include "ui_progressdialog.h"

#include "iconprovider.h"

ProgressDialog::ProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);
    this->setWindowTitle(qApp->applicationName());
    ui->informationLabel->setText("");

    ui->currentFileProgressBar->setMinimum(0);
    ui->currentFileProgressBar->setMaximum(0);
    ui->currentFileProgressBar->setValue(0);

    ui->totalProgressBar->setMinimum(0);
    ui->totalProgressBar->setMaximum(0);
    ui->totalProgressBar->setValue(0);

    connect(ui->totalProgressBar, SIGNAL(valueChanged(int)), this, SLOT(setCompletedValue(int)));
    connect(ui->totalProgressBar, SIGNAL(valueChanged(int)), this, SLOT(updateTimeLeft()));


    elapsed = 0;
    timer = new QTimer(this);
    timer->setInterval(1000); // 1s
    connect(timer, SIGNAL(timeout()), this, SLOT(updateElapsed()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateTimeLeft()));
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
    delete timer;
}

void ProgressDialog::resetUi()
{
    ui->titleLabel->hide();
    ui->informationLabel->hide();

    ui->totalLabel->setText("");
    ui->totalProgressBar->setValue(0);
    ui->totalProgressBar->setMaximum(0);
    ui->totalProgressBar->setMinimum(0);

    ui->currentFileLabel->setText("");
    ui->currentFileProgressBar->setValue(0);
    ui->currentFileProgressBar->setMinimum(0);
    ui->currentFileProgressBar->setMaximum(0);
}

void ProgressDialog::setCurrentLabelText(const QString &text)
{
    ui->currentFileLabel->setText(text);
}

// Hodnota < -1 skryje progresbar
// Value < -1 hide progressbar
void ProgressDialog::setCurrentProgress(int value)
{
    if (value < -1) {
        ui->currentFileProgressBar->setMinimum(0);
        ui->currentFileProgressBar->setMaximum(0);
        ui->currentFileProgressBar->setValue(0);
        ui->currentFileProgressBar->hide();
    } else {
        if (ui->currentFileProgressBar->isHidden()) {
            ui->currentFileProgressBar->show();
        }
        ui->currentFileProgressBar->setMinimum(0);
        ui->currentFileProgressBar->setMaximum(100);
        ui->currentFileProgressBar->setValue(value);
    }
}

void ProgressDialog::setTotalDescription(const QString &text)
{
    QPixmap pix = IconProvider::fileIcon(text).pixmap(ui->label->size());
    ui->label->setPixmap(pix);
    ui->totalLabel->setText(tr("Current Archive: ").append(text));
}

void ProgressDialog::setTotalProgress(int value)
{
    ui->totalProgressBar->setMinimum(0);
    ui->totalProgressBar->setMaximum(100);
    ui->totalProgressBar->setValue(value);
}

void ProgressDialog::setTotalProgress(QJob *job, ulong value)
{
    Q_UNUSED(job);
    setTotalProgress(value);
}

void ProgressDialog::onJobStarted(QJob* job)
{
    Q_UNUSED(job);
    elapsed = 0;
    timer->start();
}

void ProgressDialog::onJobFinished(QJob* job)
{
    Q_UNUSED(job);
    this->jobFinished();
    this->hide();
}

void ProgressDialog::registerJob(QJob *job)
{
    this->resetUi();

    connect(job, SIGNAL(percent(QJob*,ulong)), this, SLOT(setTotalProgress(QJob*,ulong)));
    connect(job, SIGNAL(currentArchive(const QString&)), this, SLOT(setTotalDescription(const QString&)));
    connect(job, SIGNAL(currentFile(const QString&)), this, SLOT(setCurrentLabelText(const QString&)));
    connect(job, SIGNAL(currentFileProgress(int)), this, SLOT(setCurrentProgress(int)));


    QObject::connect(job, SIGNAL(started(QJob*)), this, SLOT(onJobStarted(QJob*)));
    // zalezi na poradi, pokud Unregister drive nez Finished => Finished se nespusti
    QObject::connect(job, SIGNAL(finished(QJob*)), this, SLOT(onJobFinished(QJob*)));
    QObject::connect(job, SIGNAL(finished(QJob*)), this, SLOT(unregisterJob(QJob*)));

    //QObject::connect(job, SIGNAL(suspended(QJob*)), this, SLOT(suspended(QJob*)));
    //QObject::connect(job, SIGNAL(resumed(QJob*)), this, SLOT(resumed(QJob*)));

    QObject::connect(job, SIGNAL(description(QJob*, const QString&,
                                             const QPair<QString, QString>&,
                                             const QPair<QString, QString>&)),
                     this, SLOT(setDescription(QJob*, const QString&,
                                            const QPair<QString, QString>&,
                                            const QPair<QString, QString>&)));
    QObject::connect(job, SIGNAL(infoMessage(QJob*,QString,QString)),
                     this, SLOT(setInformation(QJob*,QString,QString)));
    QObject::connect(job, SIGNAL(warning(QJob*,QString,QString)),
                     this, SLOT(setInformation(QJob*,QString,QString)));

    this->show();
}


void ProgressDialog::unregisterJob(QJob *job)
{
    job->disconnect(this);
}

void ProgressDialog::jobFinished()
{
    timer->stop();
}

void ProgressDialog::onJobPercent(QJob *job, unsigned long percent)
{
    Q_UNUSED(job)
    setTotalProgress(percent);
}

void ProgressDialog::setDescription(QJob *job, const QString &title, const QPair<QString, QString> &f1, const QPair<QString, QString> &f2)
{
    Q_UNUSED(job)
    Q_UNUSED(f1)
    Q_UNUSED(f2)
    ui->titleLabel->setText(QString( "<b>%1</b>" ).arg(title));
    ui->titleLabel->show();

    //ui->titleLabel->setText(title);
    //ui->informationLabel->setText(f1.first + " " + f1.second);
}

void ProgressDialog::setInformation(QJob *job, const QString &plain, const QString &rich)
{
    Q_UNUSED(job)
    Q_UNUSED(rich)
    ui->informationLabel->setText(plain);
    ui->informationLabel->show();
}

void ProgressDialog::setCompletedValue(int value)
{
    ui->label_1->setText(QString("%1 %").arg(value));
}

void ProgressDialog::updateElapsed()
{
    elapsed++;
    int hours, minutes, seconds;
    minutes = elapsed / 60;
    seconds = elapsed % 60;
    hours = minutes / 60;
    minutes = minutes % 60;
    QChar fillChar = QLatin1Char('0');
    ui->label_6->setText(QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, fillChar).arg(seconds, 2, 10, fillChar));
}

void ProgressDialog::updateTimeLeft()
{
    int total_count = ui->totalProgressBar->maximum();
    int processed = ui->totalProgressBar->value();

    if (processed > 0) {
        long left = 0;
        int hours, minutes, seconds;
        left = float(total_count) / float(processed) * elapsed - elapsed;
        minutes = left / 60;
        seconds = left % 60;
        hours = minutes / 60;
        minutes = minutes % 60;

        QChar fillChar = QLatin1Char('0');
        ui->label_7->setText(QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, fillChar).arg(seconds, 2, 10, fillChar));
    } else {
        QChar fillChar = QLatin1Char('-');
        ui->label_7->setText(QString("%1:%2:%3").arg("",2, fillChar).arg("",2, fillChar).arg("",2, fillChar));
    }
}
