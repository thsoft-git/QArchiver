#include "textencoder.h"

#include <QtGlobal>
#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#ifdef Q_OS_UNIX
#include <iconv.h>
#endif

#include <errno.h>

#include <QMap>
#include <QTextCodec>
#include <QRegExp>
#include <QFile>
#include <QDebug>

TextEncoder::TextEncoder()
{
}

TextEncoder::~TextEncoder()
{
}


#ifdef Q_OS_WIN
wchar_t *TextEncoder::CodePageToUnicode( UINT codePage, const char * sz )
{
    Q_ASSERT( sz != NULL );

    // Get size of destination buffer
    int size = ::MultiByteToWideChar(

                codePage,

                0, // default flags

                sz, -1, // input string (\0 terminated)

                NULL,

                0 // ask required size for conversion

                );

    Q_ASSERT( size > 0 );

    // Buffer for destination wide string
    wchar_t *ws = new wchar_t[size+1];
    ws[size] = 0;

    // Convert
    int result = ::MultiByteToWideChar( codePage, 0, sz, -1, ws, size );
    Q_ASSERT( result != 0 );
    if (!result)
    {
        delete [] ws;
        return 0;
    }

    // Return wchar_t*
    return ws;
}


char *TextEncoder::UnicodeToCodePage(UINT codePage, const wchar_t *src)
{
    if (!src) return 0;

    int size = WideCharToMultiByte(codePage,
                                   0,
                                   src, -1,
                                   NULL,    // Pointer to a buffer that receives the converted string.
                                   0,       // Size, in bytes, of the buffer indicated by lpMultiByteStr.
                                            // If this parameter is set to 0, the function returns the required buffer size
                                            // for lpMultiByteStr and makes no use of the output parameter itself
                                   NULL, NULL);

    if (!size) {
        return 0;
    }

    // Buffer for destination string
    char *x = new char[size];
    x[size-1] = 0;

    // Convert
    int result = WideCharToMultiByte(codePage,
                                     0,
                                     src, -1,
                                     x, size,
                                     0,0);
    if (!result)
    {
        delete [] x;
        return 0;
    }

    // Return char*
    return x;
}

QByteArray TextEncoder::toLocat8BitOEM(const QString &str)
{
    int strLen = str.length();
    wchar_t *ws = new wchar_t[strLen+1];
    ws[strLen] = 0;
    int l = str.toWCharArray(ws);
    Q_ASSERT( strLen == l );

    UINT oemCodePage = GetOEMCP();  // OEM codepage:  852
    //UINT ansiCodePage = GetACP();   // ANSI codepage:  1250
    //qDebug() << "OEM codepage: " << oemCodePage << " ANSI codepage: " << ansiCodePage;
    char* string = TextEncoder::UnicodeToCodePage(oemCodePage, ws);
    QByteArray retVal = QByteArray(string);

    delete [] ws;
    delete [] string;

    return retVal;
}
#endif


int TextEncoder::multiByteToUtf8(const char *instr, const char *charset, char **utf8, int *utf8_len)
{
    iconv_t cd;
    char *inbuf, *outbuf;
    size_t inbytesleft, outbytesleft, nchars, utf8_buf_len;

    cd = iconv_open("UTF8",  charset);
    if (cd == (iconv_t)-1) {
        printf("%s: iconv_open failed: %d\n", __func__, errno);
        return -1;
    }

    if (instr == NULL) {
        printf("%s: NULL input string!\n", __func__);
        return -1;
    }

    inbytesleft = strlen(instr);
    if (inbytesleft == 0) {
        printf("%s: empty string\n", __func__);
        iconv_close(cd);
        return -1;
    }
    inbuf = (char *)instr;
    utf8_buf_len = 2 * inbytesleft;            // sufficient in many cases, i.e. if the input string is ASCII
    *utf8 = (char *)malloc(utf8_buf_len);
    if (!*utf8) {
        printf("%s: malloc failed\n", __func__);
        iconv_close(cd);
        return -1;
    }
    outbytesleft = utf8_buf_len;
    outbuf = *utf8;

    nchars = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    while (nchars == (size_t)-1 && errno == E2BIG) {
        char *ptr;
        size_t increase = 10;                   // increase length a bit
        size_t len;
        utf8_buf_len += increase;
        outbytesleft += increase;
        ptr = (char*)realloc(*utf8, utf8_buf_len);
        if (!ptr) {
            qDebug("%s: realloc failed\n", __func__);
            perror("realloc");
            free(*utf8);
            iconv_close(cd);
            return -1;
        }
        len = outbuf - *utf8;
        *utf8 = ptr;
        outbuf = *utf8 + len;
        nchars = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    }

    if (nchars == (size_t)-1) {
        qDebug("%s: iconv failed: %d\n", __func__, errno);
        perror("iconv");
        free(*utf8);
        iconv_close(cd);
        *utf8 = NULL;
        return -1;
    }

    if (nchars > 0) {
        qDebug("%s: iconv nonreversible conversions: %lu errno: %d\n", __func__, nchars, errno);
    }

    iconv_close(cd);
    *utf8_len = utf8_buf_len - outbytesleft;

    // Free space for null terminating??
    if (outbytesleft < sizeof(char)) {
        utf8_buf_len += sizeof(char);
        char *ptr = (char*)realloc(*utf8, utf8_buf_len);
        if (!ptr) {
            qDebug("%s: realloc failed\n", __func__);
            perror("realloc");
            free(*utf8);
            return -1;
        }
        *utf8 = ptr;
    }
    // NULL terminate
    char *end_c = *utf8 + *utf8_len;
    *end_c = '\0';

    return 0;
}


int TextEncoder::utf8ToMultiByte(const char *instr, const char *charset, char **out, int *out_len)
{
    iconv_t cd;
    char *inbuf, *outbuf;
    size_t inbytesleft, outbytesleft, nchars, out_buf_len;

    cd = iconv_open(charset, "UTF8");
    if (cd == (iconv_t)-1) {
        printf("%s: iconv_open failed: %d\n", __func__, errno);
        return -1;
    }

    if (instr == NULL) {
        printf("%s: NULL input string!\n", __func__);
        return -1;
    }

    inbytesleft = strlen(instr);
    if (inbytesleft == 0) {
        printf("%s: empty string\n", __func__);
        iconv_close(cd);
        return -1;
    }
    inbuf = (char *)instr;
    out_buf_len = inbytesleft;
    *out = (char *)malloc(out_buf_len);
    if (!*out) {
        printf("%s: malloc failed\n", __func__);
        iconv_close(cd);
        return -1;
    }
    outbytesleft = out_buf_len;
    outbuf = *out;

    nchars = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    while (nchars == (size_t)-1 && errno == E2BIG) {
        char *ptr;
        size_t increase = 10;                   // increase length a bit
        size_t len;
        out_buf_len += increase;
        outbytesleft += increase;
        ptr = (char*)realloc(*out, out_buf_len);
        if (!ptr) {
            qDebug("%s: realloc failed\n", __func__);
            perror("realloc");
            free(*out);
            iconv_close(cd);
            return -1;
        }
        len = outbuf - *out;
        *out = ptr;
        outbuf = *out + len;
        nchars = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    }

    if (nchars == (size_t)-1) {
        qDebug("%s: iconv failed: %d\n", __func__, errno);
        perror("iconv");
        free(*out);
        iconv_close(cd);
        return -1;
    }

    if (nchars > 0) {
        qDebug("%s: iconv nonreversible conversions: %lu errno: %d\n", __func__, nchars, errno);
    }

    iconv_close(cd);
    *out_len = out_buf_len - outbytesleft;

    // Free space for null terminating??
    if (outbytesleft < sizeof(char)) {
        out_buf_len += sizeof(char);
        char *ptr = (char*)realloc(*out, out_buf_len);
        if (!ptr) {
            qDebug("%s: realloc failed\n", __func__);
            perror("realloc");
            free(*out);
            return -1;
        }
        *out = ptr;
    }
    // NULL terminate
    char *end_c = *out + *out_len;
    *end_c = '\0';

    return 0;
}

void TextEncoder::findCodecs()
 {
     QMap<QString, QTextCodec *> codecMap;
     QRegExp iso8859RegExp("ISO[- ]8859-([0-9]+).*");

     foreach (int mib, QTextCodec::availableMibs()) {
         QTextCodec *codec = QTextCodec::codecForMib(mib);

         QString sortKey = codec->name().toUpper();
         int rank;

         if (sortKey.startsWith("UTF-8")) {
             rank = 1;
         } else if (sortKey.startsWith("UTF-16")) {
             rank = 2;
         } else if (iso8859RegExp.exactMatch(sortKey)) {
             if (iso8859RegExp.cap(1).size() == 1)
                 rank = 3;
             else
                 rank = 4;
         } else {
             rank = 5;
         }
         sortKey.prepend(QChar('0' + rank));

         codecMap.insert(sortKey, codec);
     }
     QList<QTextCodec *> codecs = codecMap.values();

     foreach (QTextCodec *codec, codecs)
         qDebug() << codec->name();
         //encodingComboBox->addItem(codec->name(), codec->mibEnum());
}


QString TextEncoder::decodeName(const QByteArray &fileName, const QString &codepage)
{
    if (codepage.isEmpty()) {
        return QFile::decodeName(fileName);
    }

    // Pokus se převést název pomocí zadané codepage
    char *utf8_str = 0;
    int utf8_len = 0;
    // QString codepage => const char* codepage
    // QString::toLocal8Bit().constData() vraci const char*
    // QString::toLocal8Bit() vraci QByteArray, lze rovnou použít všude, kde je očekáván const char*
    if (TextEncoder::multiByteToUtf8(fileName, codepage.toLocal8Bit(), &utf8_str, &utf8_len) < 0)
    {
        qDebug("%s: str_to_utf8 failed.", __func__);
        //error(message, details);
    }
    //utf8_str[utf8_len] = '\0'; //Může selhat, ve funkci není zaručeno, že pole bude mít na konci volný byte.
    //qDebug() << "Filename" << utf8_str << "Lenth"<< utf8_len << QString::fromUtf8(utf8_str, utf8_len);

    QString qstr_file_name = QString::fromUtf8(utf8_str, utf8_len);

    free(utf8_str); // utf8_str alokován pomocí malloc(),nebo realloc() proto free()
    return qstr_file_name;
}



QByteArray TextEncoder::encodeName(const QString &fileName, const QString &codepage)
{
    char *enc_str;
    int enc_len;

    if (TextEncoder::utf8ToMultiByte(fileName.toUtf8(), codepage.toLocal8Bit(), &enc_str, &enc_len) < 0)
    {
        qDebug("%s: utf8_to_str failed.", __func__);
        //error(message, details);
    }

    QByteArray ba_file_name = enc_str;

    free(enc_str); // utf8_str alokován pomocí malloc(),nebo realloc() proto free()
    return ba_file_name;
}




/* Na OS Mageia2 dostupné následující kodeky (IBM852 == QIBM852Codec)
"UTF-8"
"UTF-16"
"UTF-16BE"
"UTF-16LE"
"ISO-8859-1"
"ISO-8859-2"
"ISO-8859-3"
"ISO-8859-4"
"ISO-8859-5"
"ISO-8859-6"
"ISO-8859-7"
"ISO-8859-8"
"ISO-8859-9"
"ISO-8859-10"
"ISO-8859-13"
"ISO-8859-14"
"ISO-8859-15"
"ISO-8859-16"
"Apple Roman"
"Big5"
"big5-0"
"Big5-HKSCS"
"big5hkscs-0"
"cp949"
"EUC-JP"
"EUC-KR"
"GB18030"
"GB2312"
"gb2312.1980-0"
"GBK"
"gbk-0"
"IBM850"
"IBM852" //QIBM852Codec
"IBM866"
"IBM874"
"Iscii-Bng"
"Iscii-Dev"
"Iscii-Gjr"
"Iscii-Knd"
"Iscii-Mlm"
"Iscii-Ori"
"Iscii-Pnj"
"Iscii-Tlg"
"Iscii-Tml"
"ISO-2022-JP"
"jisx0201*-0"
"jisx0208*-0"
"KOI8-R"
"KOI8-U"
"ksc5601.1987-0"
"mulelao-1"
"roman8"
"Shift_JIS"
"System"
"TIS-620"
"TSCII"
"UTF-32"
"UTF-32BE"
"UTF-32LE"
"windows-1250"
"windows-1251"
"windows-1252"
"windows-1253"
"windows-1254"
"windows-1255"
"windows-1256"
"windows-1257"
"windows-1258"
"WINSAMI2" */


#if !(defined(Q_OS_WIN) || defined(Q_OS_UNIX) || defined(Q_OS_LINUX))

/*
// CP866 : OEM Russian; Cyrillic (DOS)
UINT codePage = 866;

const char * oemString = ... ;
wchar_t* wstr = CodePageToUnicode( codePage, oemString );
*/


/*
 * https://www.chilkatsoft.com/p/p_348.asp
 */
// 65001 is utf-8.
wchar_t *CodePageToUnicode(int codePage, const char *src)
{
    if (!src) return 0;
    int srcLen = strlen(src);
    if (!srcLen)
    {
        wchar_t *w = new wchar_t[1];
        w[0] = 0;
        return w;
    }

    int requiredSize = MultiByteToWideChar(codePage,
                                           0,
                                           src,srcLen,0,0);

    if (!requiredSize)
    {
        return 0;
    }

    wchar_t *w = new wchar_t[requiredSize+1];
    w[requiredSize] = 0;

    int retval = MultiByteToWideChar(codePage,
                                     0,
                                     src,srcLen,w,requiredSize);
    if (!retval)
    {
        delete [] w;
        return 0;
    }

    return w;
}

char *UnicodeToCodePage(int codePage, const wchar_t *src)
{
    if (!src) return 0;
    int srcLen = wcslen(src);
    if (!srcLen)
    {
        char *x = new char[1];
        x[0] = '\0';
        return x;
    }

    int requiredSize = WideCharToMultiByte(codePage,
                                           0,
                                           src,srcLen,0,0,0,0);

    if (!requiredSize)
    {
        return 0;
    }

    char *x = new char[requiredSize+1];
    x[requiredSize] = 0;

    int retval = WideCharToMultiByte(codePage,
                                     0,
                                     src,srcLen,x,requiredSize,0,0);
    if (!retval)
    {
        delete [] x;
        return 0;
    }

    return x;
}
#endif
