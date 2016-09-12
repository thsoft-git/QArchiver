#ifndef QSTANDARDDIRS_H
#define QSTANDARDDIRS_H

#include <QObject>
#include <QString>

class QStandardDirs : public QObject
{
    Q_OBJECT
public:
    explicit QStandardDirs(QObject *parent = 0);
    static QString findExe(const QString &appname);
    static QString testPath();

signals:
    
public slots:
    
};

#endif // QSTANDARDDIRS_H
