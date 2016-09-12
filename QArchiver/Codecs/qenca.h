#ifndef QENCA_H
#define QENCA_H

#include <QObject>
#include <QByteArray>
#include <enca.h>

class QEnca : public QObject
{
    Q_OBJECT
public:
    explicit QEnca(QObject *parent = 0);
    virtual ~QEnca();
    bool isOK();
    int errno();
    QString strError();
    bool analyse(const QByteArray &data);
    QByteArray charsetName(EncaNameStyle whatname);
    static char *systemCharset();
    static int test(unsigned char *buffer, size_t buflen);
    static int test_const(const unsigned char *buffer, size_t buflen);
signals:
    
public slots:
    
private:
    EncaAnalyser analyser;
    EncaEncoding encoding;
};

#endif // QENCA_H
