#ifndef DIRFILTERPROXYMODEL_H
#define DIRFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class DirFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DirFilterProxyModel(QObject *parent = 0);
    
signals:
    
public slots:
    
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

#endif // DIRFILTERPROXYMODEL_H
