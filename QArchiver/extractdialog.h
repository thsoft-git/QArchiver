#ifndef EXTRACTDIALOG_H
#define EXTRACTDIALOG_H

#include <QDialog>
#include <QFileSystemModel>
#include <QDirModel>

namespace Ui {
class ExtractDialog;
}

class ExtractDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ExtractDialog(QWidget *parent = 0);
    ~ExtractDialog();

    void setExtractSelectedFiles(bool value);
    void setSingleFolderArchive(bool value);
    void setCurrentDirectory(const QString& path);
    bool extractAllFiles() const;
    bool extractToSubfolder() const;
    bool preservePaths() const;
    void setPreservePaths(bool value);
    bool openDestDirAfterExtraction() const;
    void setOpenDestFolderAfterExtraction(bool value);
    bool closeAfterExtraction() const;
    bool overwriteExisting() const;
    QString destinationDirectory() const;
    QString subfolder() const;

public slots:
    virtual void accept();
    void setSubfolder(const QString& subfolder);

private slots:
    void scrollPath();
    void scrollPath(const QString& path);
    void scrollPathExpand();
    void scrollPathExpand(const QString path);
    void writeSettings();

    void onDirectoryLoaded(const QString& path);
    void onPathCurrentIndexChanged(const QString& path);
    void on_applyPathPushButton_clicked();
    void on_treeView_clicked(const QModelIndex &index);
    void on_newDirPushButton_clicked();

private:
    Ui::ExtractDialog *ui;
    QDirModel *fileSystemModel;
};

#endif // EXTRACTDIALOG_H
