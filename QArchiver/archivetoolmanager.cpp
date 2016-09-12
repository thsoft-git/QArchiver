#include "archivetoolmanager.h"

#include "ArchiveTools/cli7zplugin.h"
#include "ArchiveTools/clirarplugin.h"
#include "ArchiveTools/clizipplugin.h"
#include "ArchiveTools/qlibarchive.h"
#include "ArchiveTools/singlefilecompression.h"

#include <QFile>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QDebug>
#include <QSettings>


// Static pointer used to ensure a single instance of the class.
ArchiveToolManager *ArchiveToolManager::self = NULL;

ArchiveToolManager *ArchiveToolManager::Instance()
{
    if(!self)   // Only allow one instance of class to be generated.
        self = new ArchiveToolManager();
    return self;
}

ArchiveToolManager::ArchiveToolManager(QObject *parent) :
    QObject(parent)
{
    QSettings config;
    readSettings(config);
}

ArchiveToolManager::~ArchiveToolManager()
{
}


Archive *ArchiveToolManager::create(const QString &filePath, QObject *parent)
{
    QMimeType mimeType = ArchiveToolManager::determineQMimeType(filePath);
    return createArchive(mimeType, filePath, parent);
}


Archive *ArchiveToolManager::createArchive(const QString &mimeTypeName, const QString &filePath, QObject *parent)
{
    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForName(mimeTypeName);

    return createArchive(mimeType, filePath, parent);
}


Archive *ArchiveToolManager::createArchive(const QMimeType &mimeType, const QString &filePath, QObject *parent)
{
    const QString absFilePath = QFileInfo(filePath).absoluteFilePath();
    ArchiveInterface*const interface = Instance()->createInterface(mimeType.name());

    qDebug() << "ArchiveToolManager::createArchive()-interface:" << interface;
    if (!interface) {
        qDebug() << "Could not create interface instance for" << absFilePath;
        return NULL;
    }

    Archive* archive = new Archive(mimeType, absFilePath, interface, parent);

    return archive;
}


QMimeType ArchiveToolManager::determineQMimeType(const QString& fileName)
{
    QElapsedTimer timer;
    timer.start();

    QMimeDatabase db;
    QMimeType mime;

    if (QFile::exists(fileName)) {
        /* MatchDefault    0x0  Both the file name and content are used to look for a match */
        mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchDefault);

        /* MatchContent    0x2 The file content is used to look for a match */
        //mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);
        //QFileInfo fileInfo(fileName);
        //mime = db.mimeTypeForFile(fileInfo, QMimeDatabase::MatchContent);
    }else {
        /* MatchExtension  0x1  Only the file name is used to look for a match */
        mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchExtension);
    }

    qDebug() << "The searching file MimeType took" << timer.elapsed() << "milliseconds";
    qDebug() << "MimeType:" << mime.name();
    qDebug() << "ParentMimeTypes:" << mime.parentMimeTypes();
    return mime;
}


QString ArchiveToolManager::determineMimeType(const QString& fileName)
{
    return determineQMimeType(fileName).name();
}


QStringList ArchiveToolManager::supportedMimeTypes()
{
    QStringList supported;
    QStringList mimeTypes;

    /* Rar */
    mimeTypes.append(CliRarPlugin::supportedMimetypes());

    /* Zip */
    mimeTypes.append(CliZipPlugin::supportedMimetypes());

    /* 7zip */
    mimeTypes.append(Cli7zPlugin::supportedMimetypes());

    /* QLibArchive */
    mimeTypes.append(QLibArchive::supportedMimetypes());

    /* SingleFileCompression */
    mimeTypes.append(SingleFileCompression::supportedMimetypes());

    // Clear duplicate
    foreach (const QString& mimeType, mimeTypes) {
        if (!supported.contains(mimeType)) {
            supported.append(mimeType);
        }
    }

    //qDebug() << "ArchiveToolManager::supportedMimeTypes()" << endl << supported;
    return supported;
}

QStringList ArchiveToolManager::supportedWriteMimeTypes()
{
    QStringList mimeTypes;

    /* Rar */
    mimeTypes.append(CliRarPlugin::supportedMimetypes(ReadWrite));

    /* Zip */
    mimeTypes.append(CliZipPlugin::supportedMimetypes(ReadWrite));

    /* 7zip */
    mimeTypes.append(Cli7zPlugin::supportedMimetypes(ReadWrite));

    /* QLibArchive */
    mimeTypes.append(QLibArchive::supportedMimetypes(ReadWrite));

    /* SingleFileCompression */
    mimeTypes.append(SingleFileCompression::supportedMimetypes(ReadWrite));

    QStringList supported;

    foreach (const QString& mimeType, mimeTypes) {
        if (!supported.contains(mimeType)) {
            supported.append(mimeType);
        }
    }

    qDebug() << "ArchiveToolManager::supportedWriteMimeTypes()" << endl << supported;
    return supported;
}


QString ArchiveToolManager::filterForSupported(ArchiveOpenMode mode)
{
    /* Create filter string */
    QMimeDatabase db;
    QList<QMimeType> mimeTypes;
    QStringList filters;
    QStringList suffixes;
    QStringList mimeNames;

    switch (mode) {
    case ReadOnly:
        mimeNames = supportedMimeTypes();
        break;
    case ReadWrite: /* or Create */
        mimeNames = supportedWriteMimeTypes();
        break;
    default:
        return QString();
    }

    foreach (const QString& mimeName, mimeNames) {
        QMimeType mimeType = db.mimeTypeForName(mimeName);
        if (mimeType.isValid()) {
            mimeTypes.append(mimeType);
            filters << mimeType.filterString();
            suffixes << mimeType.suffixes();

            qDebug() << mimeType.comment() << mimeType.name() << mimeType.suffixes() << mimeType.preferredSuffix();
        }else {
            qDebug() << "Invalid mimetype";
        }

    }

    filters.sort();

    QString all_suffixes = suffixes.join(" *.").prepend("*.");
    QString all_archives_filter = tr("All Archives (%1);;").arg(all_suffixes);

    QString filterString;
    filterString = filters.join(QLatin1String(";;"));

    if (mode == ReadOnly) {
        filterString.prepend(all_archives_filter);
        qDebug() << "AllArchivesFilter:" << all_archives_filter;
    }

    qDebug() << "FilterString:" << filterString;

    return filterString;
}


QString ArchiveToolManager::filterForFiles(ArchiveOpenMode mode)
{
    return ArchiveToolManager::filterForSupported(mode);
}
/*
QString ArchiveToolManager::filterForFiles(ArchiveOpenMode mode)
{
    QStringList filters;
    switch (mode) {
    case Read:
        filters << "Compressed files (*.rar *.r?? *.zip *7z *.tar.*)"
                << "RAR files (*.rar)"
                << "ZIP files (*.zip)"
                << "7Zip files (*.7z)"
                << "LHA files (*.lha)";
        break;
    case Create:
        filters << "RAR files (*.rar)"
                << "ZIP files (*.zip)"
                << "7Zip files (*.7z)"
                << "LHA files (*.lha)";
        break;
    }
    qDebug(filters.join(QLatin1String(";;")).toAscii().constData());
    return filters.join(QLatin1String(";;"));
}*/


ArchiveInterface *ArchiveToolManager::interfaceForFile(const QString &fileName)
{
    QString mimeType = determineQMimeType(fileName).name();
    QList<Interface> e_ifaceList;

    QMap<Interface, QStringList> availableInterfaces;
    availableInterfaces[Cli7z] = Cli7zPlugin::supportedMimetypes();
    availableInterfaces[CliRar] = CliRarPlugin::supportedMimetypes();
    availableInterfaces[CliZip] = CliZipPlugin::supportedMimetypes();

    QMap<Interface, QStringList>::const_iterator i = availableInterfaces.constBegin();
    while (i != availableInterfaces.constEnd()) {
        qDebug() << i.key() << ": " << i.value() << endl;

        QStringList mimeTypes = i.value();
        if (mimeTypes.contains(mimeType)) {
            e_ifaceList.append(i.key());
        }
        ++i;
    }

    // Vybrat interface s nejvyssi prioritou
    //qSort(e_ifaceList.begin(), e_ifaceList.end(), compareInterfaces);

    if (e_ifaceList.isEmpty()) {
        return NULL; // nic nenalezeno
    }

    Interface e_iface = e_ifaceList.first();

    QString args = QFileInfo(fileName).absoluteFilePath();

    ArchiveInterface * iface = 0;
    switch (e_iface) {
    case CliRar:
        iface = new CliRarPlugin(0);
        break;
    case CliZip:
        iface = new CliZipPlugin(0);
        break;
    case Cli7z:
        iface = new Cli7zPlugin(0);
        break;
    case LibArchive:
        iface = new QLibArchive(0);
        break;
    }

    return iface;
}


bool ArchiveToolManager::laZipEnabled()
{
    return m_LibarchiveZIP;
}

bool ArchiveToolManager::laRarEnabled()
{
    return m_LibarchiveRAR;
}

bool ArchiveToolManager::laLhaEnabled()
{
    return m_LibarchiveLHA;
}

void ArchiveToolManager::writeSettings(QSettings &config)
{
    config.beginGroup("LibArchive");
    config.setValue("LibarchiveZIP", this->m_LibarchiveZIP);
    config.setValue("LibarchiveRAR", this->m_LibarchiveRAR);
    config.setValue("LibarchiveLHA", this->m_LibarchiveLHA);
    config.endGroup();
}

void ArchiveToolManager::readSettings(QSettings &config)
{
    config.beginGroup("LibArchive");
    this->m_LibarchiveZIP = config.value("LibarchiveZIP", false).toBool();
    this->m_LibarchiveRAR = config.value("LibarchiveRAR", false).toBool();
#ifdef LIBARCHIVE_NO_RAR
    this->m_LibarchiveRAR = false;
#endif
    this->m_LibarchiveLHA = config.value("LibarchiveLHA", true).toBool();
    config.endGroup();
}

void ArchiveToolManager::setLaZipEnabled(bool value)
{
    m_LibarchiveZIP = value;
}

void ArchiveToolManager::setLaRarEnabled(bool value)
{
    m_LibarchiveRAR = value;
}

void ArchiveToolManager::setLaLhaEnabled(bool value)
{
    m_LibarchiveLHA = value;
}

ArchiveInterface *ArchiveToolManager::createInterface(const QString &mimeType)
{
    ArchiveInterface* iface = 0;

    if(mimeType.compare(QLatin1String("application/x-rar"))==0)
    {
        /* .rar .rXX*/

        if (m_LibarchiveRAR) {
            iface = new QLibArchive(mimeType);
        } else {
            iface = new CliRarPlugin(0);
        }
    }
    else if (mimeType.compare(QLatin1String("application/x-7z-compressed"))==0)
    {
        /* .7z */
        iface = new Cli7zPlugin(0);
    }
    else if (mimeType.compare(QLatin1String("application/zip"))==0)
    {
        /* .zip */
        if (m_LibarchiveZIP) {
            iface = new QLibArchive(mimeType);
        } else {
            iface = new CliZipPlugin(0);
            //iface = new Cli7zPlugin(0);
        }
    }
    else if (mimeType.compare(QLatin1String("application/x-java-archive"))==0)
    {
        /* .jav */
        if (m_LibarchiveZIP) {
            iface = new QLibArchive(mimeType);
        } else {
            iface = new CliZipPlugin(0);
            //iface = new Cli7zPlugin(0);
        }
    }
    else if (mimeType.compare("application/x-cpio") == 0
             || mimeType.compare("application/x-cpio-compressed") == 0
             || mimeType.compare("application/x-sv4cpio") == 0 )
    {
        /* .cpio */
        iface = new QLibArchive(mimeType);
    }
    else if (mimeType.compare("application/x-bcpio") == 0
             || mimeType.compare("application/x-sv4crc") == 0)
    {
         /* bcpio, sv4crc */
        iface = new QLibArchive(mimeType);
    }
    else if (mimeType.compare("application/x-archive") == 0
             || mimeType.compare("application/x-ar") == 0)
    {
        /*(Ar) .a */
        iface = new QLibArchive(mimeType);
    }
    else if (mimeType.compare(QLatin1String("application/x-tar"))==0)
    {
        /* .tar */
        iface = new QLibArchive(mimeType);
        //iface = new Cli7zPlugin(0);
    }
    else if (mimeType.compare(QLatin1String("application/x-tarz"))==0) {
        /* .tar.Z */
        iface = new QLibArchive(mimeType);
        //iface = new Cli7zPlugin(0);
    }
    else if (mimeType.compare(QLatin1String("application/x-compressed-tar"))==0) {
        /* .tar.gz */
        iface = new QLibArchive(mimeType);
        //iface = new Cli7zPlugin(0);
    }
    else if (mimeType.compare(QLatin1String("application/x-bzip-compressed-tar"))==0) {
        /* .tar.bz .tar.bz2 */
        iface = new QLibArchive(mimeType);
        //iface = new Cli7zPlugin(0);
    }
    else if (mimeType.compare(QLatin1String("application/x-xz-compressed-tar"))==0) {
        /* .tar.xz */
        iface = new QLibArchive(mimeType);
        //iface = new Cli7zPlugin(0);
    }
    else if (mimeType.compare(QLatin1String("application/x-lzma-compressed-tar"))==0) {
        /* .tar.lzma */
        iface = new QLibArchive(mimeType);
        //iface = new Cli7zPlugin(0);
    }
    else if (mimeType.compare(QLatin1String("application/x-rpm"))==0) {
        /* .rpm */
        iface = new QLibArchive(mimeType);
        //iface = new Cli7zPlugin(0);
    }
    else if (mimeType.compare(QLatin1String("application/x-deb"))==0) {
        /* .deb */
        iface = new QLibArchive(mimeType);
        //iface = new Cli7zPlugin(0);
    }
    else if (mimeType.compare(QLatin1String("application/x-lha"))==0) {
        /* .lha */
        iface = new QLibArchive(mimeType);
        //iface = new Cli7zPlugin(0);
    }
    else if (mimeType.compare(QLatin1String("application/x-arj")) == 0) {
        iface = new Cli7zPlugin(0);
    }
    else if (mimeType.compare(QLatin1String("application/x-compress")) == 0
             || mimeType.compare(QLatin1String("application/x-gzip")) == 0
             || mimeType.compare(QLatin1String("application/x-bzip")) == 0
             || mimeType.compare(QLatin1String("application/x-lzma")) == 0
             || mimeType.compare(QLatin1String("application/x-xz")) == 0)
    {
             iface = new SingleFileCompression(mimeType);
    }

    return iface;
}

