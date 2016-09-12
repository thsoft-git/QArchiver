#include "qenca.h"

#include <stdio.h>
#include <langinfo.h>

#include <QDebug>

QEnca::QEnca(QObject *parent) :
    QObject(parent)
{
    const char *curren_locale = setlocale(LC_ALL, NULL);
    char lang_name[3];
    strncpy(lang_name, curren_locale, 2);
    lang_name[2] = '\0';
    //delete []curren_locale; Nemazat!! -> Pád aplikace při dalším použití Enca

    analyser = enca_analyser_alloc(lang_name);
}

QEnca::~QEnca()
{
    enca_analyser_free(analyser);
}

bool QEnca::isOK()
{
    return enca_errno(analyser) == ENCA_EOK;
}

int QEnca::errno()
{
    return enca_errno(analyser);
}

QString QEnca::strError()
{
    return enca_strerror(analyser, enca_errno(analyser));
}


bool QEnca::analyse(const QByteArray &data)
{
    encoding = enca_analyse(analyser, (unsigned char *)data.data(), data.size());
    if (enca_errno(analyser) == ENCA_EOK) {
        qDebug("Charset: %s", enca_charset_name(encoding.charset, ENCA_NAME_STYLE_HUMAN));
        QByteArray charset = enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV);
        return true;
    } else {
        /* Unrecognized encoding */
        qDebug("Charset: %s", enca_charset_name(encoding.charset, ENCA_NAME_STYLE_HUMAN));
        qDebug("Error: %s\n", enca_strerror(analyser, enca_errno(analyser)));
        return false;
    }
}

QByteArray QEnca::charsetName(EncaNameStyle whatname)
{
    return enca_charset_name(encoding.charset, whatname);
}

char *QEnca::systemCharset()
{
    return nl_langinfo(CODESET);
}

int QEnca::test(unsigned char *buffer, size_t buflen)
{
    EncaAnalyser analyser;
    EncaEncoding encoding;
    //unsigned char buffer[4096];
    //size_t buflen;
    //buflen = fread(buffer, 1, 4096, stdin);
    analyser = enca_analyser_alloc("cs");
    encoding = enca_analyse(analyser, buffer, buflen);

    qDebug("Charset: %s\n", enca_charset_name(encoding.charset, ENCA_NAME_STYLE_HUMAN));
    qDebug("Error: %s\n", enca_strerror(analyser, enca_errno(analyser)));
    enca_analyser_free(analyser);
    return 0;
}

int QEnca::test_const(const unsigned char *buffer, size_t buflen)
{
    EncaAnalyser analyser;
    //qDebug() << analyser;
    EncaEncoding encoding;
    analyser = enca_analyser_alloc("cs");
    //analyser = enca_analyser_alloc(0);
    //qDebug() << analyser;
    encoding = enca_analyse_const(analyser, buffer, buflen);
    qDebug("Charset: %s\n", enca_charset_name(encoding.charset, ENCA_NAME_STYLE_HUMAN));
    qDebug("Error: %s\n", enca_strerror(analyser, enca_errno(analyser)));
    enca_analyser_free(analyser);
    return 0;
}
