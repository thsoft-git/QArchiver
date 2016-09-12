#ifndef QSIZEFORMATER_H
#define QSIZEFORMATER_H

#include <QString>

namespace QSizeFormater {

QString convertSize(quint64 bytes);
QString itemsSummaryString(uint items, uint files, uint dirs, quint64 size, bool showSize);

}

#endif // QSIZEFORMATER_H
