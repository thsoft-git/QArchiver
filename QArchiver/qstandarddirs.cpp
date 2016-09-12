#include "qstandarddirs.h"
#include <QDir>
#include <QApplication>
#include <QDebug>

QStandardDirs::QStandardDirs(QObject *parent) :
    QObject(parent)
{
}

QString QStandardDirs::findExe(const QString &appname)
{

#ifdef Q_OS_WIN
    QStringList executable_extensions;// = executableExtensions();
    executable_extensions << QLatin1String(".exe");
    if (!executable_extensions.contains(appname.section(QLatin1Char('.'), -1, -1, QString::SectionIncludeLeadingSep), Qt::CaseInsensitive)) {
        QString found_exe;
        foreach (const QString& extension, executable_extensions) {
            found_exe = findExe(appname + extension/*, pstr, options*/);
            if (!found_exe.isEmpty()) {
                return found_exe;
            }
        }
        return QString();
    }
#endif
    // Environment path variable
    QString env_path = QString::fromLocal8Bit(qgetenv("PATH"));

#ifdef Q_OS_LINUX
    QStringList path_list = env_path.split(':');
#endif
#ifdef Q_OS_WIN
    QStringList path_list = env_path.split(';');
#endif
    // local app tools path
    //QString local_app_path = qApp->applicationDirPath() +"/bin";
    //path_list.insert(0, local_app_path); // Vlozit na začátek - nejvyšší priorita

    // local app tools path;
    //path_list.insert(0, "/opt/InfoZip/bin"); // Vlozit na začátek

    // Instalace v: "/usr/local/bin" upřednostnit možnost nahradit standardní systémové verze
    path_list.insert(0, "/usr/local/bin");

#ifdef Q_OS_WIN
    // WinRar standard path
    QString winrar_path = "C:\\Program Files\\WinRAR\\";
    path_list.insert(1, winrar_path);
#endif

    //qDebug() << path_list;
    foreach (const QString& path, path_list) {
        qDebug() << path;
        QDir dir = QDir(path);
        if (dir.exists(appname)) {
            return dir.absoluteFilePath(appname);
        }
    }

    return QString();
}

QString QStandardDirs::testPath()
{
    QStringList path;
    //APPDATA
    QString env_appdata = QString::fromLocal8Bit(qgetenv("APPDATA")); // fromLocal8Bit() i cz znaky
    // Environment path variable
    QString env_path = qgetenv("PATH");
#ifdef Q_OS_WIN
    path = env_path.split(';');
#endif
#ifdef Q_OS_LINUX
    path = env_path.split(':');
    //path << env_path;
#endif
    // local app tools path
    QString local_app_path = qApp->applicationDirPath() +"/app/bin";
    // WinRar path
    QString winrar_path = "C:\\Program Files\\WinRAR\\";

    path.insert(0, local_app_path);
    path.insert(1, winrar_path);
    path.append(env_appdata);

    return path.join("\n");
}
