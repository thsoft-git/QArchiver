#include "filesystemmodel.h"

FileSystemModel::FileSystemModel(QObject *parent) :
    QFileSystemModel(parent)
{
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DecorationRole) {
        // Qt::DecorationRole QFileSystemModelu způsobuje rozhození vzhledu header sekce QTreeView
        // -- zvětšená výška header sekce, rozhozené vertikální zarovnání textu
        // -- problém se objevuje jen na OS linux, v OS Windows Vista OK
        return QVariant();
    }else {
        return QFileSystemModel::headerData(section, orientation, role);
    }
}
