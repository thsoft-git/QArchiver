#ifndef UNZIPREVERSEENCODER_H
#define UNZIPREVERSEENCODER_H

#define MAX_CP_NAME 25

#include <QByteArray>

class UnzipReverseEncoder
{
    struct CharsetMap {
        const char *local_charset;
        const char *archive_charset;
    };

    static const CharsetMap dos_charset_map[];

public:
    UnzipReverseEncoder();
    void reverse(char *string);
    QByteArray reverse1(const QByteArray &instr);

private:
    char *local_charset;
    char OEM_CP[MAX_CP_NAME];
};

#endif // UNZIPREVERSEENCODER_H
