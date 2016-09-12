#ifndef ARCHIVETOOLMANAGER_H
#define ARCHIVETOOLMANAGER_H

#include <QObject>
#include <QSettings>
#include <QtMimeTypes/QMimeDatabase>

#include "ArchiveTools/archiveinterface.h"

class ArchiveToolManager : public QObject
{
    Q_OBJECT
public:
    static ArchiveToolManager * Instance();
    ~ArchiveToolManager();
    static QMimeType determineQMimeType(const QString& fileName);
    static QString determineMimeType(const QString& fileName);

    static Archive* create(const QString &filePath, QObject *parent = 0);
    static Archive* createArchive(const QString &mimeTypeName, const QString &filePath, QObject *parent = 0);
    static Archive* createArchive(const QMimeType &mimeType, const QString &filePath, QObject *parent = 0);

    static QStringList supportedMimeTypes();
    static QStringList supportedWriteMimeTypes();
    static QString filterForSupported(ArchiveOpenMode mode = ReadOnly);
    static QString filterForFiles(ArchiveOpenMode mode = ReadOnly);
    ArchiveInterface* createInterface(const QString &mimeType);

    bool laZipEnabled();
    bool laRarEnabled();
    bool laLhaEnabled();
    void writeSettings(QSettings &config);
    void readSettings(QSettings &config);

    enum Interface {
        Cli7z = 0,
        CliRar,
        CliZip,
        LibArchive
    };
    ArchiveInterface* interfaceForFile(const QString& fileName);

signals:
    
public slots:
    void setLaZipEnabled(bool value);
    void setLaRarEnabled(bool value);
    void setLaLhaEnabled(bool value);

private:
    explicit ArchiveToolManager(QObject *parent = 0);          // Private so that it can  not be called
    ArchiveToolManager(ArchiveToolManager const&);             // copy constructor is private
    ArchiveToolManager& operator=(ArchiveToolManager const&);  // assignment operator is private

    static ArchiveToolManager *self;
    bool m_LibarchiveZIP;
    bool m_LibarchiveRAR;
    bool m_LibarchiveLHA;
};

#endif // ARCHIVETOOLMANAGER_H
