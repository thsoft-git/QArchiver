#ifndef QRECENTFILESMENU_H
#define QRECENTFILESMENU_H

#include <QMenu>

class QSettings;

class QRecentFilesMenu : public QMenu
{
    Q_OBJECT
    Q_PROPERTY(int maxItems READ maxItems WRITE setMaxItems)
public:
    //! Constructs a menu with a parent.
    explicit QRecentFilesMenu(QWidget *parent = 0);

    //! Constructs a menu with a title and a parent.
    QRecentFilesMenu(const QString &title, QWidget *parent = 0);

    //! Returns the maximum number of entries in the menu.
    int maxItems() const;

    //! Returns the format string for menu entries.
    QString format() const;

    /**
     * Sets the format string for menu entries.
     * For example: "%d %s [%p]"
     *  - %d - Entry number
     *  - %s - File name
     *  - %p - File path
     */
    void setFormat(const QString &fmtStr);

    /** Saves the state of the recent entries.
     * Typically this is used in conjunction with QSettings to remember entries
     * for a future session. A version number is stored as part of the data.
     * Here is an example:
     * QSettings settings;
     * settings.setValue("recentFiles", recentFilesMenu->saveState());
     */
    QByteArray saveState() const;
    void saveEntries(QSettings &config);

    /** Restores the recent entries to the state specified.
     * Typically this is used in conjunction with QSettings to restore entries from a past session.
     * Returns false if there are errors.
     * Here is an example:
     * QSettings settings;
     * recentFilesMenu->restoreState(settings.value("recentFiles").toByteArray());
     */
    bool restoreState(const QByteArray &state);
    void loadEntries(QSettings &config);

signals:
    //! This signal is emitted when a recent file in this menu is triggered.
    void recentFileTriggered(const QString & filename);
    
public slots:
    //! Add file path to menu
    void addFile(const QString &filePath);

    //! Remove file path from menu
    void removeFile(const QString &filePath);

    //! Removes all the menu's actions.
    void clearMenu();

    //! Sets the maximum number of entries int he menu.
    void setMaxItems(int count);

private slots:
    void fileActionTriggered();
    void menuItemTriggered(QAction* action);
    void updateRecentFileActions();

private:
    int m_maxItems;
    QString m_format;
    QStringList m_files;

};

#endif // QRECENTFILESMENU_H
