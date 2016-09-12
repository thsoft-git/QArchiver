#include "iconprovider.h"

#ifdef Q_OS_WIN
#include <Windows.h> //DestroyIcon(), Library User32.lib, DLL User32.dll
#include <ObjBase.h> //CoInitialize(), Library Ole32.lib, DLL Ole32.dll
#include <Shellapi.h> //SHGetFileInfo(), Library Shell32.lib, DLL Shell32.dll (version 4.0 or later)
#endif
#ifdef Q_OS_LINUX
#include <QtMimeTypes/QMimeDatabase>
#endif

// Global static pointer used to ensure a single instance of the class.
IconProvider *IconProvider::self = 0;

IconProvider *IconProvider::Instance()
{
    if(!self)   // Only allow one instance of class to be generated.
        self = new IconProvider();
    return self;
}

QIcon IconProvider::fileIcon(const QString &filename)
{
    QFileInfo fileInfo(filename);
    QPixmap pixmap;

#ifdef Q_OS_WIN32

    if (fileInfo.suffix().isEmpty() || fileInfo.suffix() == "exe" && fileInfo.exists())
    {
        return Instance()->iconProvider.icon(fileInfo);
    }

    if (!Instance()->iconCache.find(fileInfo.suffix(), &pixmap))
    {
        // Support for nonexistent file type icons, will reimplement it as custom icon provider later
        /* We don't use the variable, but by storing it statically, we
         * ensure CoInitialize is only called once. */
        static HRESULT comInit = CoInitialize(NULL);
        Q_UNUSED(comInit);

        SHFILEINFO shFileInfo;
        unsigned long val = 0;

        val = SHGetFileInfo((const wchar_t *)("foo." + fileInfo.suffix()).utf16(), 0, &shFileInfo,
                            sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES);
//        val = SHGetFileInfo((const wchar_t *)("foo." + fileInfo.suffix()).utf16(), 0, &shFileInfo,
//                            sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES); // vetsi ikona

        // Even if GetFileInfo returns a valid result, hIcon can be empty in some cases
        if (val && shFileInfo.hIcon)
        {
            pixmap = QPixmap::fromWinHICON(shFileInfo.hIcon);
            if (!pixmap.isNull())
            {
                Instance()->iconCache.insert(fileInfo.suffix(), pixmap);
            }
            DestroyIcon(shFileInfo.hIcon);
        }
        else
        {
            // TODO: Return default icon if nothing else found
        }
    }

#else
    if (fileInfo.exists())
    {
        // Default icon for Linux and Mac OS X for now
        return Instance()->iconProvider.icon(fileInfo);
    }

    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForFile(fileInfo);
    QString in = mt.iconName();
    QString icoGeneric = mt.genericIconName();
    QIcon icon = QIcon::fromTheme(in, QIcon::fromTheme(icoGeneric));
    return icon;
    //return Instance()->iconProvider.icon(fileInfo);
#endif

    return QIcon(pixmap);
}

QIcon IconProvider::dirIcon()
{
    return Instance()->iconProvider.icon(QFileIconProvider::Folder);
}

QIcon IconProvider::icon(QFileIconProvider::IconType type)
{
    return Instance()->iconProvider.icon(type);
}

/***/

