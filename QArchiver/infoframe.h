#ifndef INFOFRAME_H
#define INFOFRAME_H

#include <QFrame>

#include "archivemodel.h"

namespace Ui {
class InfoFrame;
}

class InfoFrame : public QFrame
{
    Q_OBJECT
    
public:
    explicit InfoFrame(QWidget *parent = 0);
    ~InfoFrame();

    void setArchiveModel(ArchiveModel *model);
    ArchiveModel* archiveModel();

    void setIndex(const QModelIndex &index);
    void setIndexes(const QModelIndexList &list);
    
    QString metadataTextFor(const QModelIndex &index);
    void showMetaData();
    void hideMetaData();
    void showActions();
    void hideActions();
    /* Alternative file name */
    QString altFileName();
    void setAltFileName(const QString &fileName);

public slots:
    void setDefaultInfo();

private:
    Ui::InfoFrame *ui;

    ArchiveModel *m_model;
    QString m_altFileName;
};

#endif // INFOFRAME_H
