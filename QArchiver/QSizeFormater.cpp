#include "QSizeFormater.h"

#include <QLocale>

QString QSizeFormater::convertSize(quint64 bytes){
    // Format file size value to B, KiB, MiB, GiB, TiB
    const quint64 kib = 1024;
    const quint64 mib = 1024 * kib;
    const quint64 gib = 1024 * mib;
    const quint64 tib = 1024 * gib;

    if (bytes >= tib)
        return QObject::tr("%1 TiB").arg(QLocale().toString(qreal(bytes) / tib, 'f', 3));
    if (bytes >= gib)
        return QObject::tr("%1 GiB").arg(QLocale().toString(qreal(bytes) / gib, 'f', 2));
    if (bytes >= mib)
        return QObject::tr("%1 MiB").arg(QLocale().toString(qreal(bytes) / mib, 'f', 1));
    if (bytes >= kib)
        return QObject::tr("%1 KiB").arg(QLocale().toString(bytes / kib));
    //return QFileSystemModel::tr("%1 byte(s)").arg(QLocale().toString(bytes));
    return QObject::tr("%1 byte(s)").arg(QLocale().toString(bytes));
}


QString QSizeFormater::itemsSummaryString(uint items, uint files, uint dirs, quint64 size, bool showSize)
{
    if ( files == 0 && dirs == 0 && items == 0 ) {
        return QObject::tr( "%n Item(s)", 0, 0);
    }

    QString summary;
    const QString foldersText = QObject::tr( "%n Folder(s)", 0, dirs );
    const QString filesText = QObject::tr( "%n File(s)", 0, files );
    if ( files > 0 && dirs > 0 ) {
        summary = showSize ?
                    QObject::tr( "%1, %2 (%3)", "folders, files (size)").arg(foldersText, filesText, convertSize( size ) ) :
                    QObject::tr( "%1, %2", "folders, files").arg(foldersText, filesText);
    } else if ( files > 0 ) {
        summary = showSize ? QObject::tr( "%1 (%2)", "files (size)").arg(filesText, convertSize( size ) ) : filesText;
    } else if ( dirs > 0 ) {
        summary = foldersText;
    }

    if ( items > dirs + files ) {
        const QString itemsText = QObject::tr( "%n Item(s)", 0, items );
        summary = summary.isEmpty() ? itemsText : QObject::tr( "%1: %2", "items: folders, files (size)").arg(itemsText, summary);
    }

    return summary;
}
