#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QFileSystemModel>
#include "QtExt/qrecentfilesmenu.h"

#include "archivemodel.h"
#include "dirfilterproxymodel.h"
#include "openarguments.h"
#include "testresultdialog.h"
#include "archivetoolmanager.h"

namespace Ui {
class MainWindow;
}

class QDuoLed;
class QClickableLabel;
class InfoFrame;
class PropertiesDialog;
class SettingsDialog;
class ProgressDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool openArchive(const QString& path);
    void setShowExtractDialog(bool option);

protected:
    void viewArchive();
    void viewFileSystem();
    void writeSettings();
    void readSettings();

    QStringList suffixesFromFilterStr(const QString& filterStr);
    QString suffixFromFilterStr(const QString& filterStr);
    bool hasSuffix(const QString& fileName, const QStringList& suffixes);

    bool openFile(const QString &filePath);
    bool createFile(const QString & filePath);

    //reimplemented from QWidget
    virtual void resizeEvent(QResizeEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
    static bool isSingleUrlDrag(const QMimeData *data);
    QString getSaveFileName(QString *selectedFilter, QString directory); // debug

private:
    void setupActions();
    void createMenuView();
    void createMenuActions();
    void createInfoDockWindow();
    void setupModelsAndViews();
    bool isSingleEntryArchive() const;
    QString detectSubfolder() const;
    QList<QVariant> selectedFiles();
    QList<QVariant> selectedFilesWithChildren();
    void registerJob(QJob *job);
    bool isBusy() const;
    bool isPreviewable(const QModelIndex &index) const;
    // debug:
    void debugFileSystemModel();
    void debugArchiveModel();
    void debugTest_QTreeView();

private slots:
    void on_actionNew_archive_triggered();
    void on_actionOpen_archive_triggered();
    void on_actionClose_archive_triggered(); //Autoconnect version of closeArchive()
    void closeArchive();
    void on_actionGo_Up_triggered();
    void on_actionSet_Password_triggered();

    void loadRecentFile(const QString &filePath);

    void on_treeView_clicked(const QModelIndex &index);
    void on_treeViewContent_doubleClicked(const QModelIndex &index);
    void on_treeViewContent_customContextMenuRequested(const QPoint &pos);
    void on_actionTest_triggered();
    void onUserQuery(Query *query);
    void onBusyLedClick();
    void onActionProperties();
    void onAbout();
    void slotTestDone(QJob* job);
    void slotOpenDone(QJob* job);
    void slotCreateDone(QJob *job);
    void slotLoadingStarted(QJob *job);
    void slotLoadingFinished(QJob *job);
    void showErrorMessage(const QString& errorMessage, const QString& details);
    void showInfoMessage(const QString &message);
    void slotExtractFiles();
    void slotExtractionDone(QJob *job);
    void slotTmpExtractionDone(QJob* job);
    void slotAddFiles();
    void slotAddFiles(const QStringList& files, const QString& path = QString());
    void slotAddDir();
    void slotAddFilesDone(QJob *job);
    void slotDeleteFiles();
    void slotDeleteFilesDone(QJob *job);
    void updateActions();
    void updateFileMenuActions();
    void selectionChanged();
    void setBusyGui();
    void setReadyGui();
    void slotPreview();
    void slotPreview(const QModelIndex & index);
    void slotPreviewExtracted(QJob *job);
    void slotChangeCodepage();

// debug:
    void smazan(QObject *obj);  // debug
    void on_pushButton_Make_Dir_clicked();  // debug
    void on_pushButton_Delete_Dir_clicked();  // debug
    void on_pushButton_TestProgressDialog_clicked();  // debug
    void on_pushButton_testEnv_clicked();  // debug

signals:

private:
    Ui::MainWindow *ui;

    ArchiveToolManager *m_archiveManager;
    QString m_fsLastDirPath;
    QFileSystemModel *m_dirModel;
    QFileSystemModel *m_fileSystemModel;
    ArchiveModel *m_archiveModel;
    DirFilterProxyModel *m_archiveDirModel;
    bool m_archiveMode;
    bool m_busy;

    QRecentFilesMenu *m_recentFilesMenu;
    InfoFrame *m_infoFrame;

    QByteArray m_treeViewContentHeaderState;
    OpenArguments m_openArgs;

    // temp extracted files
    QList<QFile *> m_tmpFiles;
    //QDir m_previewDir;
    QAction *m_addDirAction;
    QAction *m_previewAction;
    QAction *m_codepageAction;
    QAction *m_settingsAction;
    QAction *m_aboutAction;
    QMenu *menuView;
    QDuoLed *busyLed;
    QClickableLabel *cpsLabel;
    SettingsDialog *m_settingsDialog;
    ProgressDialog *m_jobProgress;
};

#endif // MAINWINDOW_H
