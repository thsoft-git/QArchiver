#include "dirfilterproxymodel.h"

#include "archivemodel.h"
#include <QDebug>

DirFilterProxyModel::DirFilterProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
}

bool DirFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    ArchiveModel* source_model =  dynamic_cast<ArchiveModel *>(sourceModel());
    if (source_model) {
        QModelIndex index0 = source_model->index(source_row, 0, source_parent);
        ArchiveEntry entry = source_model->entryForIndex(index0);
        return entry[IsDirectory].toBool();
    }
    // Pokud model není ArchiveModel, nic nefiltruj
    // If source modei is not subclass of ArchiveModel, filter nothing
    return true;
}
