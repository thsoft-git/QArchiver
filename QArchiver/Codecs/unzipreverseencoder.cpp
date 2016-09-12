#include "unzipreverseencoder.h"

#include <iconv.h>
#include <langinfo.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>      // errno

/*
 * A mapping of local <-> archive charsets used by default to convert filenames
 * of DOS/Windows Zip archives. Currently very basic.
 * Source: UnZip 6.10b file unzip.c
 */
const UnzipReverseEncoder::CharsetMap UnzipReverseEncoder::dos_charset_map[] = {
    { "ANSI_X3.4-1968", "CP850" },
    { "ISO-8859-1", "CP850" },
    { "CP1252", "CP850" },
    { "UTF-8", "CP866" },
    { "KOI8-R", "CP866" },
    { "KOI8-U", "CP866" },
    { "ISO-8859-5", "CP866" }
};

UnzipReverseEncoder::UnzipReverseEncoder()
{
    local_charset = nl_langinfo(CODESET);

    for(uint i = 0; i < sizeof(dos_charset_map)/sizeof(CharsetMap); i++)
    {
        if(!strcasecmp(local_charset, dos_charset_map[i].local_charset)) {
            strncpy(OEM_CP, dos_charset_map[i].archive_charset, sizeof(OEM_CP));
            break;
        }
    }
}


/* Convert a string from one encoding to the current locale using iconv().
 * Be as non-intrusive as possible. If error is encountered during
 * convertion just leave the string intact. */
void UnzipReverseEncoder::reverse(char *string)
{
    iconv_t cd;
    char *s,*d, *buf;
    size_t slen, dlen, stlen, buflen;
    //const char *local_charset;

    buf = NULL;
    //local_charset = nl_langinfo(CODESET);

    if ((cd = iconv_open(OEM_CP, local_charset)) == (iconv_t)-1)
        return;

    stlen = slen = strlen(string);
    s = string;
    dlen = buflen = 2 * slen;
    d = buf = (char *)malloc(buflen + 1);
    if (d) {
        bzero(buf, buflen);
        if(iconv(cd, &s, &slen, &d, &dlen) != (size_t)-1){
            //strncpy(string, buf, buflen);
            strncpy(string, buf, stlen);
        }
        free(buf);
    }
    iconv_close(cd);

}

QByteArray UnzipReverseEncoder::reverse1(const QByteArray &instr)
{
    iconv_t cd;
    char *inbuf;
    char *outbuf;
    char *out;
    int out_len;
    size_t inbytesleft, outbytesleft, nchars, out_buf_len;

    cd = iconv_open(OEM_CP, local_charset);
    if (cd == (iconv_t)-1) {
        qDebug("%s: iconv_open failed: %d\n", __func__, errno);
        return QByteArray(instr);
    }

    if (instr.isNull()) {
        qDebug("%s: NULL input string!\n", __func__);
        return QByteArray();
    }

    inbytesleft = strlen(instr);
    if (inbytesleft == 0) {
        qDebug("%s: empty string\n", __func__);
        iconv_close(cd);
        return QByteArray();
    }
    inbuf = (char *)instr.constData();
    out_buf_len = inbytesleft;
    out = (char *)malloc(out_buf_len);
    if (!out) {
        qDebug("%s: malloc failed\n", __func__);
        iconv_close(cd);
        return QByteArray(instr);
    }
    outbytesleft = out_buf_len;
    outbuf = out;

    nchars = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    while (nchars == (size_t)-1 && errno == E2BIG) {
        char *ptr;
        size_t increase = 10;                   // increase length a bit
        size_t len;
        out_buf_len += increase;
        outbytesleft += increase;
        ptr = (char*)realloc(out, out_buf_len);
        if (!ptr) {
            qDebug("%s: realloc failed\n", __func__);
            qDebug("realloc: %s", strerror(errno));
            free(out);
            iconv_close(cd);
            return QByteArray(instr);
        }
        len = outbuf - out;
        out = ptr;
        outbuf = out + len;
        nchars = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    }

    if (nchars == (size_t)-1) {
        //qDebug("\n%s: iconv failed: %d %s", __func__, errno, strerror(errno));
        free(out);
        iconv_close(cd);
        return QByteArray(instr);
    }

    if (nchars > 0) {
        qDebug("%s: iconv nonreversible conversions: %lu errno: %d\n", __func__, nchars, errno);
    }

    iconv_close(cd);
    out_len = out_buf_len - outbytesleft;

    // Free space for null terminating??
    if (outbytesleft < sizeof(char)) {
        out_buf_len += sizeof(char);
        char *ptr = (char*)realloc(out, out_buf_len);
        if (!ptr) {
            qDebug("%s: realloc failed\n", __func__);
            free(out);
            return QByteArray(instr);
        }
        out = ptr;
    }
    // NULL terminate
    char *end_c = out + out_len;
    *end_c = '\0';

    QByteArray outBa(out);
    free(out);

    return outBa;
}
