#include "libachivesettingswidget.h"

#include <QVBoxLayout>

#include "qlibarchive.h"

LibAchiveSettingsWidget::LibAchiveSettingsWidget(QWidget *parent) :
    QWidget(parent)
{
    nameLabel = new QLabel(tr("LibArchive:"), this);
    libArchiveZip = new QCheckBox(tr("Enable ZIP format (read, write)."),this);
    libArchiveRar = new QCheckBox(tr("Enable RAR format (read only, experimental)."),this);
#ifdef LIBARCHIVE_NO_RAR
    libArchiveRar->setChecked(false);
    libArchiveRar->setEnabled(false);
#endif
    libArchiveLha = new QCheckBox(tr("Enable LHA format (read only)."),this);
    libArchiveLha->setEnabled(false);

    QVBoxLayout *vertical_layout = new QVBoxLayout(this);
    vertical_layout->addWidget(nameLabel);
    vertical_layout->addWidget(libArchiveZip);
    vertical_layout->addWidget(libArchiveRar);
    vertical_layout->addWidget(libArchiveLha);
    QSpacerItem *verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    vertical_layout->addSpacerItem(verticalSpacer);
}

void LibAchiveSettingsWidget::apply()
{
    m_manager->setLaZipEnabled(libArchiveZip->isChecked());
    m_manager->setLaRarEnabled(libArchiveRar->isChecked());
    m_manager->setLaLhaEnabled(libArchiveLha->isChecked());
}

void LibAchiveSettingsWidget::reset()
{
    libArchiveZip->setChecked(m_manager->laZipEnabled());
    libArchiveRar->setChecked(m_manager->laRarEnabled());
    libArchiveLha->setChecked(m_manager->laLhaEnabled());
}

void LibAchiveSettingsWidget::setToolManager(ArchiveToolManager *manager)
{
    m_manager = manager;
    reset();
}

