#include "qrecentfilesmenu.h"

#include <QFileInfo>
#include <QSettings>

//static const QString RecentFilesDefauldFormat = "%d %s [%p]";
static const QString RecentFilesDefauldFormat = "%s [%p]";

QRecentFilesMenu::QRecentFilesMenu(QWidget *parent) :
    QMenu(parent)
{
    setMaxItems(5);
    setFormat(RecentFilesDefauldFormat);
}

QRecentFilesMenu::QRecentFilesMenu(const QString &title, QWidget *parent) :
    QMenu(title, parent)
{
    setMaxItems(5);
    setFormat(RecentFilesDefauldFormat);
}

int QRecentFilesMenu::maxItems() const
{
    return m_maxItems;
}

QString QRecentFilesMenu::format() const
{
    return m_format;
}

/**
 * Sets the format string for menu entries.
 * For example: "%d %s [%p]"
 *  - %d - Entry number
 *  - %s - File name
 *  - %p - File path
 */
void QRecentFilesMenu::setFormat(const QString &fmtStr)
{
    // pokud rozdilné -> update, jinak nic nedělat
    if (m_format != fmtStr) {
        m_format = fmtStr;
        updateRecentFileActions();
    }
}

QByteArray QRecentFilesMenu::saveState() const
{
    return QByteArray();
}

void QRecentFilesMenu::saveEntries(QSettings &config)
{
    QString key;
    QString value;

    config.remove("RecentFiles");
    config.beginGroup("RecentFiles");
    config.setValue("FilesCount", m_files.count());
    for (int i = 0; i < m_files.count(); i++)
    {
        key = QString("File%1").arg(i);
        value = m_files[i];

        config.setValue(key, value);
    }
    config.endGroup();
}


bool QRecentFilesMenu::restoreState(const QByteArray &state)
{
    Q_UNUSED(state);
    return false;
}

void QRecentFilesMenu::loadEntries(QSettings &config)
{
    QMenu::clear();
    m_files.clear();

    config.beginGroup("RecentFiles");
    int numRecentFiles = config.value("FilesCount", 0).toInt();
    if (numRecentFiles > 0) {
        QString key;
        QString value;

        for (int i = 0; i < numRecentFiles; ++i) {
            key = QString("File%1").arg(i);
            value = config.value(key, QString()).toString();
            m_files.append(value);
        }  
    }
    config.endGroup();
    updateRecentFileActions();
}


void QRecentFilesMenu::addFile(const QString &filePath)
{
    m_files.removeAll(filePath);
    m_files.prepend(filePath);

    while (m_files.size() > maxItems())
        m_files.removeLast();

    updateRecentFileActions();
}


void QRecentFilesMenu::removeFile(const QString &filePath)
{
    m_files.removeAll(filePath);
    updateRecentFileActions();
}


void QRecentFilesMenu::clearMenu()
{
    m_files.clear();
    updateRecentFileActions();
}


void QRecentFilesMenu::setMaxItems(int count)
{
    m_maxItems = count;
    updateRecentFileActions();
}


void QRecentFilesMenu::fileActionTriggered()
{
    // Jestli sender() je QAction: action == (QAction*)sender(), jinak NULL
    QAction *action = qobject_cast<QAction *>(sender());
    // Pokud prvni podminka == NULL dale se nevyhodnocuje, proto action->neco() je OK
    // Usetri jeden if()
    if ((action != NULL) && action->data().isValid()){
        emit recentFileTriggered(action->data().toString());
    }
}


void QRecentFilesMenu::menuItemTriggered(QAction *action)
{
    if (action->data().isValid())
        emit recentFileTriggered(action->data().toString());
}

void QRecentFilesMenu::updateRecentFileActions()
{
    int numRecentFiles = qMin(m_files.size(), maxItems());

    clear();

    for (int i = 0; i < numRecentFiles; ++i) {
        QString fileName = QFileInfo(m_files[i]).fileName();

        QString text = m_format;
        text.replace(QLatin1String("%d"), QString::number(i + 1));
        text.replace(QLatin1String("%s"), fileName);
        text.replace(QLatin1String("%p"), m_files[i]);

        QAction* recentFileAct = addAction(text, this, SLOT(fileActionTriggered()));
        recentFileAct->setData(m_files[i]);
    }
    // Clear menu action
    addSeparator();
    addAction(tr("Clear Menu"), this, SLOT(clearMenu()));

    setEnabled(numRecentFiles > 0);
}
