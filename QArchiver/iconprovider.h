/* C++ Singleton
 *
 * http://www.yolinux.com/TUTORIALS/C++Singleton.html
 * http://sourcemaking.com/design_patterns/Singleton/cpp/1
 *
 */

#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H

#include <QIcon>
#include <QPixmapCache>
#include <QFileIconProvider>

class IconProvider
{
public:
    static IconProvider * Instance();
    static QIcon fileIcon(const QString &filename);
    static QIcon dirIcon();
    static QIcon icon(QFileIconProvider::IconType type);
private:
    IconProvider(){}    // Private so that it can  not be called
    IconProvider(IconProvider const&);             // copy constructor is private
    IconProvider& operator=(IconProvider const&);  // assignment operator is private

private:
    static IconProvider *self;
    QPixmapCache iconCache;
    QFileIconProvider iconProvider;
};

#endif // ICONPROVIDER_H
