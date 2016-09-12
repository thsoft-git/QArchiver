#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QPushButton>
#include "archivetoolmanager.h"
#include "ArchiveTools/libachivesettingswidget.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    createIcons();
    createPages();
    readSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::createIcons()
{
    QListWidgetItem *settingsButton = new QListWidgetItem(ui->listWidget);
    settingsButton->setIcon(QIcon(":/tools/settings"));
    settingsButton->setText(tr("Archivers"));
    settingsButton->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    settingsButton->setSizeHint(QSize(0, 32));
    settingsButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *settingsButton2 = new QListWidgetItem(ui->listWidget);
    settingsButton2->setIcon(QIcon(":/tools/encoding"));
    settingsButton2->setText(tr("Filename encoding"));
    settingsButton2->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    settingsButton2->setSizeHint(QSize(0, 32));
    settingsButton2->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(ui->listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));
}

void SettingsDialog::createPages()
{
    /* Page 1 */
    QVBoxLayout *vertical_layout = new QVBoxLayout(ui->page);

    la_settings = new LibAchiveSettingsWidget(ui->page);
    la_settings->setObjectName("la_settings");
    la_settings->setToolManager(ArchiveToolManager::Instance());
    vertical_layout->addWidget(la_settings);

    /* Page 2 */
    QVBoxLayout *vertical_layout_p2 = new QVBoxLayout(ui->page_2);

//    analyseEncoding = new QCheckBox(ui->page_2);
//    analyseEncoding->setText(tr("Analyse filename encoding"));
//    analyseEncoding->setChecked(true);
//    vertical_layout_p2->addWidget(analyseEncoding);

    encodingInfo = new QCheckBox(ui->page_2);
    encodingInfo->setText(tr("Show encoding information messages"));
    encodingInfo->setChecked(true);
    vertical_layout_p2->addWidget(encodingInfo);

    vertical_layout_p2->addStretch();
}

void SettingsDialog::readSettings()
{
    QSettings settings;
    encodingInfo->setChecked(settings.value("showEncodingInfo", true).toBool());
//    analyseEncoding->setChecked(settings.value("analyseEncoding", true).toBool());
}

void SettingsDialog::writeSettings()
{
    QSettings settings;
    settings.setValue("showEncodingInfo", encodingInfo->isChecked());
//    settings.setValue("analyseEncoding", analyseEncoding->isChecked());
}

void SettingsDialog::defaultSettings()
{
    encodingInfo->setChecked(true);
//    analyseEncoding->setChecked(true);
}

void SettingsDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    ui->stackedWidget->setCurrentIndex(ui->listWidget->row(current));
    ui->iconLabel->setPixmap(current->icon().pixmap(ui->iconLabel->height()));
    ui->label->setText(current->text());

}

void SettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton *btn = qobject_cast<QPushButton *>(button);

    if (btn == ui->buttonBox->button(QDialogButtonBox::Ok) || btn == ui->buttonBox->button(QDialogButtonBox::Apply)) {
        qDebug("Settings applyed");
        //LibAchiveSettingsWidget *la_settings = ui->page->findChild<LibAchiveSettingsWidget *>(QString("la_settings"));
        la_settings->apply();
        writeSettings();
    } else if (btn == ui->buttonBox->button(QDialogButtonBox::Reset)) {
        la_settings->reset();
        readSettings();
    }
}

void SettingsDialog::showEvent(QShowEvent *event)
{
    readSettings();
    QDialog::showEvent(event);
}
