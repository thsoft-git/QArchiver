#include "infoframe.h"
#include "ui_infoframe.h"

#include "iconprovider.h"

#include <QtMimeTypes/QMimeDatabase>

InfoFrame::InfoFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::InfoFrame),
    m_model(NULL)
{
    ui->setupUi(this);

    // Make the file name font bigger than the rest
    QFont fnt = ui->fileNameLabel->font();
    if (fnt.pointSize() > -1) {
        fnt.setPointSize(fnt.pointSize() + 1);
    } else {
        fnt.setPixelSize(fnt.pixelSize() + 3);
    }
    ui->fileNameLabel->setFont(fnt);

    setDefaultInfo();
}

InfoFrame::~InfoFrame()
{
    delete ui;
}

void InfoFrame::setArchiveModel(ArchiveModel *model)
{
    m_model = model;
    if (m_model != NULL) {
         connect(m_model, SIGNAL(archiveChanged()), this, SLOT(setDefaultInfo()));
    }

    setDefaultInfo();
}

ArchiveModel *InfoFrame::archiveModel()
{
    return m_model;
}

void InfoFrame::setIndex(const QModelIndex &index)
{
    if (!index.isValid()) {
            setDefaultInfo();
        } else {
            const ArchiveEntry& entry = m_model->entryForIndex(index);
            QMimeDatabase db;
            QMimeType mimeType;

            if (entry[ IsDirectory ].toBool()) {
                mimeType = db.mimeTypeForName(QLatin1String( "inode/directory" ));
            } else {
                mimeType = db.mimeTypeForFile(entry[ FileName ].toString());
            }

            //ui->iconLabel->setPixmap(QIcon::fromTheme(mimeType.iconName()).pixmap(48));
            ui->iconLabel->setPixmap(IconProvider::fileIcon(entry[ FileName ].toString()).pixmap(48));
            if (entry[ IsDirectory ].toBool()) {
                int dirs;
                int files;
                const int children = m_model->childCount(index, dirs, files);
                ui->additionalInfoLabel->setText(QSizeFormater::itemsSummaryString(children, files, dirs, 0, false));
            } else if (entry.contains(Link)) {
                ui->additionalInfoLabel->setText(tr("Symbolic Link"));
            } else {
                if (entry.contains(Size)) {
                    ui->additionalInfoLabel->setText(QSizeFormater::convertSize(entry[ Size ].toULongLong()));
                } else {
                    ui->additionalInfoLabel->setText(tr("Unknown size"));

                }
            }

            const QStringList nameParts = entry[ FileName ].toString().split(QLatin1Char( '/' ), QString::SkipEmptyParts);
            const QString name = (nameParts.count() > 0) ? nameParts.last() : entry[ FileName ].toString();
            ui->fileNameLabel->setText(name);

            ui->metadataLabel->setText(metadataTextFor(index));
            showMetaData();
        }
}

void InfoFrame::setIndexes(const QModelIndexList &list)
{
    if (list.size() == 0) {
            setIndex(QModelIndex());
        } else if (list.size() == 1) {
            setIndex(list[ 0 ]);
        } else {
            ui->iconLabel->setPixmap(IconProvider::icon(QFileIconProvider::Computer).pixmap(48));
            ui->fileNameLabel->setText((list.size() > 1) ? tr("%1 files selected").arg(list.size()) : tr("One file selected"));
            quint64 totalSize = 0;
            foreach(const QModelIndex& index, list) {
                const ArchiveEntry& entry = m_model->entryForIndex(index);
                totalSize += entry[ Size ].toULongLong();
            }
            ui->additionalInfoLabel->setText(QSizeFormater::convertSize(totalSize));
            hideMetaData();
    }
}

void InfoFrame::setDefaultInfo()
{
    QString currentFileName;

    if (m_altFileName.isEmpty() && (m_model != NULL)) {
        if (m_model->archive()) {
            QFileInfo fileInfo(m_model->archive()->fileName());
            currentFileName = fileInfo.fileName();
        }
    }

    if (currentFileName.isEmpty()) {
        ui->iconLabel->setPixmap(IconProvider::icon(QFileIconProvider::Computer).pixmap(48));
        ui->fileNameLabel->setText(tr("No archive loaded"));
    } else {
        ui->iconLabel->setPixmap(IconProvider::fileIcon(m_model->archive()->fileName()).pixmap(48));
        ui->fileNameLabel->setText(currentFileName);
    }

    ui->additionalInfoLabel->setText(QString());
    hideMetaData();
    hideActions();
}


QString InfoFrame::metadataTextFor(const QModelIndex &index)
{
    const ArchiveEntry& entry = m_model->entryForIndex(index);
    QString text;


    QMimeDatabase db;
    QMimeType mimeType;

    if (entry[ IsDirectory ].toBool()) {

        mimeType = db.mimeTypeForName(QLatin1String( "inode/directory" ));
    } else {

        mimeType =db.mimeTypeForFile(entry[ FileName ].toString());
    }

    text += tr("<b>Type:</b> %1<br/>").arg(mimeType.comment());


    if (entry.contains(Size)) {
        text += tr("<b>Size:</b> %1<br/>").arg(QSizeFormater::convertSize(entry[ Size ].toLongLong()));
    }

    if (entry.contains(CompressedSize)) {
        text += tr("<b>Compressed Size:</b> %1<br/>").arg(QSizeFormater::convertSize(entry[ CompressedSize ].toLongLong()));
    }

    if (entry.contains(Owner)) {
        text += tr("<b>Owner:</b> %1<br/>").arg(entry[ Owner ].toString());
    }

    if (entry.contains(Group)) {
        text += tr("<b>Group:</b> %1<br/>").arg(entry[ Group ].toString());
    }

    if (entry.contains(Link)) {
        text += tr("<b>Target:</b> %1<br/>").arg(entry[ Link ].toString());
    }

    if (entry.contains(IsPasswordProtected) && entry[ IsPasswordProtected ].toBool()) {
        text += tr("<b>Password protected:</b> Yes<br/>");
    }

    return text;
}

void InfoFrame::showMetaData()
{
    ui->firstLine->show();
    ui->metadataLabel->show();
}

void InfoFrame::hideMetaData()
{
    ui->firstLine->hide();
    ui->metadataLabel->hide();
}

void InfoFrame::showActions()
{
    ui->secondLine->show();
    ui->actionsLabel->show();
}

void InfoFrame::hideActions()
{
    ui->secondLine->hide();
    ui->actionsLabel->hide();
}

QString InfoFrame::altFileName()
{
    return m_altFileName;
}

void InfoFrame::setAltFileName(const QString &fileName)
{
    m_altFileName = fileName;
}
