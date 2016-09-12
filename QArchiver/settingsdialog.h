#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QAbstractButton>

class QCheckBox;
class QListWidgetItem;
class LibAchiveSettingsWidget;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

private:
    void createIcons();
    void createPages();
    void readSettings();
    void writeSettings();
    void defaultSettings();

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);
    
private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

protected:
    virtual void showEvent(QShowEvent *event);

private:
    Ui::SettingsDialog *ui;
    LibAchiveSettingsWidget *la_settings;
    //QCheckBox *analyseEncoding;
    QCheckBox *encodingInfo;
};

#endif // SETTINGSDIALOG_H
