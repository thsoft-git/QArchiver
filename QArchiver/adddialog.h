#ifndef ADDDIALOG_H
#define ADDDIALOG_H

#include <QtGui/QDialog>
#include <QtCore/QHash>
#include <QtMimeTypes/QMimeDatabase>
#include <QtMimeTypes/QMimeType>

class QHBoxLayout;
class QGridLayout;
class QRadioButton;

namespace Ui {
class AddDialog;
}

class AddDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AddDialog(QWidget *parent = 0);
    AddDialog(const QStringList& itemsToAdd, const QString& startDir, QWidget* parent = 0);
    ~AddDialog();

    QString selectedFilePath() const;
    void setSelectedFilePath(const QString & filePath);
    QMimeType currentMimeType();
    QString currentMimeTypeSuffix();
    QString currentMimeTypeName();
public slots:
    virtual void done(int r);

private slots:
    void onArchiveFormatChanged();
    void on_pushButtonBrowse_clicked();

private:
    void setupArchiveFormatGroup();
    void setupIconList(const QStringList& itemsToAdd);
    QString getFileName(QWidget *parent, const QString &title, const QString &path,
                        const QString &filter, QString *selectedFilter = 0);
    QString getFileName2(QWidget *parent, const QString &title, const QString &path,
                        const QStringList &filters, QString *selectedFilter = 0);
    QString suffixFromFilterStr(const QString &filterStr);
    
private:
    Ui::AddDialog *ui;

    // Archive format groupbox
    QGridLayout *layout_format;
    QHash<QRadioButton* , QMimeType> mimeMap;
    QRadioButton *rb_format_zip;
    QRadioButton *rb_format_7z;
    QRadioButton *rb_format_ar;
    QRadioButton *rb_format_cpio;
    QRadioButton *rb_format_cpio_gz;
    QRadioButton *rb_format_tar;
    QRadioButton *rb_format_tar_Z;
    QRadioButton *rb_format_tar_gz;
    QRadioButton *rb_format_tar_bz2;
    QRadioButton *rb_format_tar_lzma;
    QRadioButton *rb_format_tar_xz;
    QRadioButton *rb_format_rar;

    QMimeType m_currentMimeType;
};

#endif // ADDDIALOG_H
