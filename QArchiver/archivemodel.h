#ifndef ARCHIVEMODEL_H
#define ARCHIVEMODEL_H

#include <QAbstractItemModel>
#include <QScopedPointer>

#include "qarchive.h"
#include "QSizeFormater.h"

class Query;
class ArchiveNode;
class ArchiveDirNode;

class ArchiveModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ArchiveModel(QObject *parent = 0);
    virtual ~ArchiveModel();

    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    Archive *archive() const;
    void setArchive(Archive *archive);
    bool isSingleFolderArchive();
    bool isSingleFileArchive();
    QString subfolderName();

    OpenJob* openArchive();
    bool isArchiveOpen() const;

    QString codePage() const;
    void setCodePage(const QString &codepage);

    ArchiveEntry entryForIndex(const QModelIndex &index);
    int childCount(const QModelIndex &index, int &dirs, int &files) const;

    ExtractJob* extractFile(const QVariant& fileName, const QString & destinationDir, const ExtractionOptions options = ExtractionOptions()) const;
    ExtractJob* extractFiles(const QList<QVariant>& files, const QString & destinationDir, const ExtractionOptions options = ExtractionOptions()) const;

    AddJob* addFiles(const QStringList & paths, const CompressionOptions& options = CompressionOptions());
    DeleteJob* deleteFiles(const QList<QVariant> & files);
    TestJob* testArchive();

public slots:
    void listArchive();

signals:
    void loadingStarted(QJob *);
    void loadingFinished(QJob *);
    void extractionFinished(bool success);
    void error(const QString& error, const QString& details);
    void info(const QString &plain);
    void codePageChanged(const QString &codepage);
    void archiveChanged();

private slots:
    void addEntryToQueue(const ArchiveEntry& entry);
    void onNewEntry(const ArchiveEntry& entry);
    void onEntryRemoved(const QString & path);
    void onLoadingFinished(QJob *job);
    void onUserQuery(Query *query);
    void cleanupEmptyDirs();
    void onArchCodePageChanged(const QString &codepage);

private:
    QString cleanFileName(const QString& fileName);

    ArchiveDirNode* parentFor(const ArchiveEntry& entry);
    QModelIndex indexForNode(ArchiveNode *node);
    static bool compareAscending(const QModelIndex& a, const QModelIndex& b);
    static bool compareDescending(const QModelIndex& a, const QModelIndex& b);

    enum InsertBehaviour { NotifyViews, DoNotNotifyViews };
    void insertNode(ArchiveNode *node, InsertBehaviour behaviour = NotifyViews);
    void addEntry(const ArchiveEntry& entry, InsertBehaviour behaviour);

    QList<ArchiveEntry> m_newArchiveEntries; // holds entries from opening
    QList<int> m_showColumns;
    QScopedPointer<Archive> m_archive;
    ArchiveDirNode *m_rootNode;
    ArchiveNode* m_previousMatch;
    QStringList m_previousPieces;
};

#endif // ARCHIVEMODEL_H
