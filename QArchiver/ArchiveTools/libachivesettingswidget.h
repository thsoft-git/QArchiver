#ifndef LIBACHIVESETTINGSWIDGET_H
#define LIBACHIVESETTINGSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QSettings>

#include "archivetoolmanager.h"

class LibAchiveSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LibAchiveSettingsWidget(QWidget *parent = 0);
    void setToolManager(ArchiveToolManager * manager);
    void apply();
    void reset();
    
signals:
    
public slots:
    
private:
    QLabel *nameLabel;
    QCheckBox *libArchiveZip;
    QCheckBox *libArchiveRar;
    QCheckBox *libArchiveLha;
    ArchiveToolManager *m_manager;
};

#endif // LIBACHIVESETTINGSWIDGET_H
