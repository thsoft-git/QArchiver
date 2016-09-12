#include "qibm852codec.h"
#include <QDebug>
QIBM852Codec::QIBM852Codec() :
    QTextCodec()
{
    IBM850Codec = QTextCodec::codecForName("IBM850");
}

QStringList QIBM852Codec::aliases()
{
    QStringList ret;
    ret << "ibm-852_P100-1995"
        << "ibm-852"
        << "IBM852"
        << "cp852"
        << "852"
        << "csPCp852"
        << "windows-852";
    return ret;
}

QString QIBM852Codec::convertToUnicode(const char *in, int length, ConverterState *state) const
{
    Q_UNUSED(state);
    if (IBM850Codec == NULL) {
        return "";
    } else {
        //unsigned short ch;
        ushort ch;
        QString str;
        for (int i = 0; i<length; i++) {
            switch (in[i]) {
            // Bez přetypování na char nefunguje ...proč?
            case (char)0x85: // ů
                ch = 0x016F;
                break;
            case (char)0x86: // ć
                ch = 0x0107;
                break;
            case (char)0x88: // ł
                ch = 0x0142;
                break;
            case (char)0x8A: // Ő
                ch = 0x0150;
                break;
            case (char)0x8B: // ő
                ch = 0x0151;
                break;
            case (char)0x8D: // Ź
                ch = 0x0179;
                break;
            case (char)0x8F: // Ć
                ch = 0x0106;
                break;
            case (char)0x91: // Ĺ
                ch = 0x0139;
                break;
            case (char)0x92: // ĺ
                ch = 0x013A;
                break;
            case (char)0x95: // Ľ
                ch = 0x013D;
                break;
            case (char)0x96: // ľ
                ch = 0x013E;
                break;
            case (char)0x97: // Ś
                ch = 0x015A;
                break;
            case (char)0x98: // ś
                ch = 0x015B;
                break;
            case (char)0x9B: // Ť
                ch = 0x0164;
                break;
            case (char)0x9C: // ť
                ch = 0x0165;
                break;
            case (char)0x9D: // Ł
                ch = 0x0141;
                break;
            case (char)0x9F: // č
                ch = 0x010D;
                break;
            case (char)0xA4: // Ą
                ch = 0x0104;
                break;
            case (char)0xA5: // ą
                ch = 0x0105;
                break;
            case (char)0xA6: // Ž
                ch = 0x017D;
                break;
            case (char)0xA7: // ž
                ch = 0x017E;
                break;
            case (char)0xA8: // Ę
                ch = 0x0118;
                break;
            case (char)0xA9: // ę
                ch = 0x0119;
                break;
            case (char)0xAB: // ź
                ch = 0x017A;
                break;
            case (char)0xAC: // Č
                ch = 0x010C;
                break;
            case (char)0xAD: // ş
                ch = 0x015F;
                break;
            case (char)0xB7: // Ě
                ch = 0x011A;
                break;
            case (char)0xB8: // Ş
                ch = 0x015E;
                break;
            case (char)0xBD: // Ż
                ch = 0x017B;
                break;
            case (char)0xBE: // ż
                ch = 0x017C;
                break;
            case (char)0xC6: // Ă
                ch = 0x0102;
                break;
            case (char)0xC7: // ă
                ch = 0x0103;
                break;
            case (char)0xD0: // đ
                ch = 0x0111;
                break;
            case (char)0xD1: // Đ
                ch = 0x0110;
                break;
            case (char)0xD2: // Ď
                ch = 0x010E;
                break;
            case (char)0xD4: // ď
                ch = 0x010F;
                break;
            case (char)0xD5: // Ň
                ch = 0x0147;
                break;
            case (char)0xD8: // ě
                ch = 0x011B;
                break;
            case (char)0xDD: // Ţ
                ch = 0x0162;
                break;
            case (char)0xDE: // Ů
                ch = 0x016E;
                break;
            case (char)0xE3: // Ń
                ch = 0x0143;
                break;
            case (char)0xE4: // ń
                ch = 0x0144;
                break;
            case (char)0xE5: // ň
                ch = 0x0148;
                break;
            case (char)0xE6: // Š
                ch = 0x0160;
                break;
            case (char)0xE7: // š
                ch = 0x0161;
                break;
            case (char)0xE8: // Ŕ
                ch = 0x0154;
                break;
            case (char)0xEA: // ŕ
                ch = 0x0155;
                break;
            case (char)0xEB: // Ű
                ch = 0x0170;
                break;
            case (char)0xEE: // ţ
                ch = 0x0163;
                break;
            case (char)0xF1: // ˝
                ch = 0x02DD;
                break;
            case (char)0xF2: // ˛
                ch = 0x02DB;
                break;
            case (char)0xF3: // ˇ
                ch = 0x02C7;
                break;
            case (char)0xF4: // ˘
                ch = 0x02D8;
                break;
            case (char)0xFA: // ˙
                ch = 0x02D9;
                break;
            case (char)0xFB: // ű
                ch = 0x0171;
                break;
            case (char)0xFC: // Ř
                ch = 0x0158;
                break;
            case (char)0xFD: // ř
                ch = 0x0159;
                break;
            default:
                //qDebug() << "IBM852Codec" << "Nic nenalezeno -- předávám IBM850 --";
                ch = IBM850Codec->toUnicode(&in[i], 1).unicode()[0].unicode();
                break;
            }
            str.append(QChar(ch));
            //qDebug() << "String:" << str;
        }
        //return IBM850Codec->toUnicode(in, length, state);
        return str;
    }
}

QByteArray QIBM852Codec::convertFromUnicode(const QChar *in, int length, ConverterState *state) const
{
    Q_UNUSED(state)
    if (IBM850Codec == NULL) {
        return "";
    } else {
        QByteArray ret;
        char ch;
        for (int i = 0; i<length; i++) {
            switch (in[i].unicode()) {
            // přehodit hodnoty
            case 0x016F: // ů
                ch = (char)0x85;
                break;
            case 0x0107: // ć
                ch = (char)0x86;
                break;
            case 0x0142: // ł
                ch = (char)0x88;
                break;
            case 0x0150: // Ő
                ch = (char)0x8A;
                break;
            case 0x0151: // ő
                ch = (char)0x8B;
                break;
            case 0x0179: // Ź
                ch = (char)0x8D;
                break;
            case 0x0106: // Ć
                ch = (char)0x8F;
                break;
            case 0x0139: // Ĺ
                ch = (char)0x91;
                break;
            case 0x013A: // ĺ
                ch = (char)0x92;
                break;
            case 0x013D: // Ľ
                ch = (char)0x95;
                break;
            case 0x013E: // ľ
                ch = (char)0x96;
                break;
            case 0x015A: // Ś
                ch = (char)0x97;
                break;
            case 0x015B: // ś
                ch = (char)0x98;
                break;
            case 0x0164: // Ť
                ch = (char)0x9B;
                break;
            case 0x0165: // ť
                ch = (char)0x9C;
                break;
            case 0x0141: // Ł
                ch = (char)0x9D;
                break;
            case 0x010D: // č
                ch = (char)0x9F;
                break;
            case 0x0104: // Ą
                ch = (char)0xA4;
                break;
            case 0x0105: // ą
                ch = (char)0xA5;
                break;
            case 0x017D: // Ž
                ch = (char)0xA6;
                break;
            case 0x017E: // ž
                ch = (char)0xA7;
                break;
            case 0x0118: // Ę
                ch = (char)0xA8;
                break;
            case 0x0119: // ę
                ch = (char)0xA9;
                break;
            case 0x017A: // ź
                ch = (char)0xAB;
                break;
            case 0x010C: // Č
                ch = (char)0xAC;
                break;
            case 0x015F: // ş
                ch = (char)0xAD;
                break;
            case 0x011A: // Ě
                ch = (char)0xB7;
                break;
            case 0x015E: // Ş
                ch = (char)0xB8;
                break;
            case 0x017B: // Ż
                ch = (char)0xBD;
                break;
            case 0x017C: // ż
                ch = (char)0xBE;
                break;
            case 0x0102: // Ă
                ch = (char)0xC6;
                break;
            case 0x0103: // ă
                ch = (char)0xC7;
                break;
            case 0x0111: // đ
                ch = (char)0xD0;
                break;
            case 0x0110: // Đ
                ch = (char)0xD1;
                break;
            case 0x010E: // Ď
                ch = (char)0xD2;
                break;
            case 0x010F: // ď
                ch = (char)0xD4;
                break;
            case 0x0147: // Ň
                ch = (char)0xD5;
                break;
            case 0x011B: // ě
                ch = (char)0xD8;
                break;
            case 0x0162: // Ţ
                ch = (char)0xDD;
                break;
            case 0x016E: // Ů
                ch = (char)0xDE;
                break;
            case 0x0143: // Ń
                ch = (char)0xE3;
                break;
            case 0x0144: // ń
                ch = (char)0xE4;
                break;
            case 0x0148: // ň
                ch = (char)0xE5;
                break;
            case 0x0160: // Š
                ch = (char)0xE6;
                break;
            case 0x0161: // š
                ch = (char)0xE7;
                break;
            case 0x0154: // Ŕ
                ch = (char)0xE8;
                break;
            case 0x0155: // ŕ
                ch = (char)0xEA;
                break;
            case 0x0170: // Ű
                ch = (char)0xEB;
                break;
            case 0x0163: // ţ
                ch = (char)0xEE;
                break;
            case 0x02DD: // ˝
                ch = (char)0xF1;
                break;
            case 0x02DB: // ˛
                ch = (char)0xF2;
                break;
            case 0x02C7: // ˇ
                ch = (char)0xF3;
                break;
            case 0x02D8: // ˘
                ch = (char)0xF4;
                break;
            case 0x02D9: // ˙
                ch = (char)0xFA;
                break;
            case 0x0171: // ű
                ch = (char)0xFB;
                break;
            case 0x0158: // Ř
                ch = (char)0xFC;
                break;
            case 0x0159: // ř
                ch = (char)0xFD;
                break;
            default:
                ch = IBM850Codec->fromUnicode(&in[i], 1).data()[0];
                break;
            }

            ret.append(ch);
        }
        return ret;
    }
}


//QByteArray QIBM852Codec::convertFromUnicode(const QChar *in, int length, ConverterState *state) const
//{
//    Q_UNUSED(state)
//    if (IBM850Codec == NULL) {
//        return "";
//    } else {
//        QByteArray ret;
//        for (int i = 0; i<length; i++) {
//            switch (in[i].unicode()) {
//            case 369: // ű
//                ret.append(150);
//                break;
//            case 337: // ő
//                ret.append(147);
//                break;
//            case 193: // Á
//                ret.append(143);
//                break;
//            case 205: // Í
//                ret.append(141);
//                break;
//            case 368: // Ű
//                ret.append(85);
//                break;
//            case 336: // Ő
//                ret.append(QChar(153));
//                break;
//            case 218: // Ú
//                ret.append(85);
//                break;
//            case 211: // Ó
//                ret.append(79);
//                break;
//            default:
//                ret.append(IBM850Codec->fromUnicode(&in[i], 1));
//                break;
//            }
//        }
//        return ret;
//    }
//}


QIBM852Codec::~QIBM852Codec()
{

}
