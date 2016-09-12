#include "archivemodel.h"
#include "archive.h"
#include "ArchiveTools/archiveinterface.h"
#include "jobs.h"
#include "iconprovider.h"

#include <QUrl>
#include <QDir>
#include <QFont>
#include <QLatin1String>
#include <QList>
#include <QMimeData>
#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QFileIconProvider>
#include <QDebug>

class ArchiveDirNode;

class ArchiveNode
{
public:
    ArchiveNode(ArchiveDirNode *parent, const ArchiveEntry & entry)
        : m_parent(parent)
    {
        setEntry(entry);
    }

    virtual ~ArchiveNode()
    {
    }

    const ArchiveEntry &entry() const
    {
        return m_entry;
    }

    void setEntry(const ArchiveEntry& entry)
    {
        m_entry = entry;

        const QStringList pieces = entry[FileName].toString().split(QLatin1Char( '/' ), QString::SkipEmptyParts);
        m_name = pieces.isEmpty() ? QString() : pieces.last();

        if (entry[IsDirectory].toBool()) {
            // Singleton IconProvider -> cached icons, icons of nonexisting files
            m_icon = IconProvider::Instance()->dirIcon().pixmap(16, 16);
        } else {
            m_icon = IconProvider::Instance()->fileIcon(m_entry[FileName].toString()).pixmap(16, 16);
        }
    }

    ArchiveDirNode *parent() const
    {
        return m_parent;
    }

    int row() const;

    virtual bool isDir() const
    {
        return false;
    }

    QPixmap icon() const
    {
        return m_icon;
    }

    QString name() const
    {
        return m_name;
    }

protected:
    void setIcon(const QPixmap &icon)
    {
        m_icon = icon;
    }

private:
    ArchiveEntry    m_entry;
    QPixmap         m_icon;
    QString         m_name;
    ArchiveDirNode *m_parent;
};


class ArchiveDirNode: public ArchiveNode
{
public:
    ArchiveDirNode(ArchiveDirNode *parent, const ArchiveEntry & entry)
        : ArchiveNode(parent, entry)
    {
    }

    ~ArchiveDirNode()
    {
        clear();
    }

    QList<ArchiveNode*> entries()
    {
        return m_entries;
    }

    void setEntryAt(int index, ArchiveNode* value)
    {
        m_entries[index] = value;
    }

    void appendEntry(ArchiveNode* entry)
    {
        m_entries.append(entry);
    }

    void removeEntryAt(int index)
    {
        delete m_entries.takeAt(index);
    }

    virtual bool isDir() const
    {
        return true;
    }

    ArchiveNode* find(const QString & name)
    {
        foreach(ArchiveNode *node, m_entries) {
            if (node && (node->name() == name)) {
                return node;
            }
        }
        return 0;
    }

    ArchiveNode* findByPath(const QStringList & pieces, int index = 0)
    {
        if (index == pieces.count()) {
            return 0;
        }

        ArchiveNode *next = find(pieces.at(index));

        if (index == pieces.count() - 1) {
            return next;
        }
        if (next && next->isDir()) {
            return static_cast<ArchiveDirNode*>(next)->findByPath(pieces, index + 1);
        }
        return 0;
    }

    void returnDirNodes(QList<ArchiveDirNode*> *store)
    {
        foreach(ArchiveNode *node, m_entries) {
            if (node->isDir()) {
                store->prepend(static_cast<ArchiveDirNode*>(node));
                static_cast<ArchiveDirNode*>(node)->returnDirNodes(store);
            }
        }
    }

    void clear()
    {
        qDeleteAll(m_entries);
        m_entries.clear();
    }

private:
    QList<ArchiveNode*> m_entries;
};

/*
 * Helper functor used by qStableSort.
 * It always sorts folders before files.
 * http://stackoverflow.com/questions/356950/c-functors-and-their-uses
 */
class ArchiveModelSorter
{
public:
    ArchiveModelSorter(int column, Qt::SortOrder order) :
        m_sortColumn(column),
        m_sortOrder(order)
    {
    }

    virtual ~ArchiveModelSorter()
    {
    }

    inline bool operator()(const QPair<ArchiveNode*, int> &left, const QPair<ArchiveNode*, int> &right) const
    {
        if (m_sortOrder == Qt::AscendingOrder) {
            return lessThan(left, right);
        } else {
            return !lessThan(left, right);
        }
    }

protected:
    bool lessThan(const QPair<ArchiveNode*, int> &left, const QPair<ArchiveNode*, int> &right) const
    {
        const ArchiveNode * const leftNode = left.first;
        const ArchiveNode * const rightNode = right.first;

        // Sort folders before files
        if ((leftNode->isDir()) && (!rightNode->isDir())) {
            return (m_sortOrder == Qt::AscendingOrder);
        } else if ((!leftNode->isDir()) && (rightNode->isDir())) {
            return !(m_sortOrder == Qt::AscendingOrder);
        }

        const QVariant &leftEntry = leftNode->entry()[m_sortColumn];
        const QVariant &rightEntry = rightNode->entry()[m_sortColumn];

        switch (m_sortColumn) {
        case FileName:
            return leftNode->name() < rightNode->name();
        case Size:
        case CompressedSize:
            return leftEntry.toInt() < rightEntry.toInt();
        default:
            return leftEntry.toString() < rightEntry.toString();
        }

        // Sem by se to nikdy nemělo dostat
        Q_ASSERT(false);
        return false;
    }

private:
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
};

int ArchiveNode::row() const
{
    if (parent()) {
        return parent()->entries().indexOf(const_cast<ArchiveNode*>(this));
    }
    return 0;
}

ArchiveModel::ArchiveModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootNode(new ArchiveDirNode(0, ArchiveEntry()))
{
    //used to speed up the loading of large archives
    m_previousMatch = NULL;
}

ArchiveModel::~ArchiveModel()
{
    delete m_rootNode;
    m_rootNode = 0;
}

QVariant ArchiveModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        ArchiveNode *node = static_cast<ArchiveNode*>(index.internalPointer());
        switch (role) {
        case Qt::DisplayRole: {
            int columnId = m_showColumns.at(index.column());
            switch (columnId) {
            case FileName:
                return node->name();
            case Size:
                if (node->isDir()) {
                    int dirs;
                    int files;
                    const int children = childCount(index, dirs, files);
                    return QSizeFormater::itemsSummaryString(children, files, dirs, 0, false);
                } else if (node->entry().contains(Link)) {
                    return QVariant();
                } else {
                    return QSizeFormater::convertSize(node->entry()[ Size ].toULongLong());
                }
            case CompressedSize:
                if (node->isDir() || node->entry().contains(Link)) {
                    return QVariant();
                } else {
                    qulonglong compressedSize = node->entry()[ CompressedSize ].toULongLong();
                    if (compressedSize != 0) {
                        return QSizeFormater::convertSize(compressedSize);
                    } else {
                        return QVariant();
                    }
                }
            case Ratio:
                // TODO: Použít node->entry()[Ratio] pokud dostupné
                if (node->isDir() || node->entry().contains(Link)) {
                    return QVariant();
                } else {
                    qulonglong compressedSize = node->entry()[ CompressedSize ].toULongLong();
                    qulonglong size = node->entry()[ Size ].toULongLong();
                    if (compressedSize == 0 || size == 0) {
                        return QVariant();
                    } else {
                        int ratio = int(100 * ((double)size - compressedSize) / size);
                        return QString(QString::number(ratio) + QLatin1String(" %"));
                    }
                }

            case Timestamp: {
                const QDateTime timeStamp = node->entry().value(Timestamp).toDateTime();
                return QLocale().toString(timeStamp, "d. MMMM yyyy hh:mm:ss");
            }

            default:
                return node->entry().value(columnId);
            }
            break;
        }
        case Qt::DecorationRole:
            if (index.column() == 0) {
                return node->icon();
            }
            return QVariant();
        case Qt::FontRole: {
            QFont f;
            f.setItalic(node->entry()[ IsPasswordProtected ].toBool());
            return f;
        }
        default:
            return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags ArchiveModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    if (index.isValid()) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | defaultFlags;
    }

    return 0;
}

QVariant ArchiveModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (section >= m_showColumns.size()) {
            //qDebug() << "showColumns.size = " << m_showColumns.size() << " and section = " << section;
            return QVariant();
        }

        int columnId = m_showColumns.at(section);

        switch (columnId) {
        case FileName:
            return tr("Name", "Name of a file inside an archive");
        case Size:
            return tr("Size", "Uncompressed size of a file inside an archive");
        case CompressedSize:
            return tr("Compressed", "Compressed size of a file inside an archive");
        case Ratio:
            return tr("Rate", "Compression rate of file");
        case Owner:
            return tr("Owner", "File's owner username");
        case Group:
            return tr("Group", "File's group");
        case Permissions:
            return tr("Mode", "File permissions");
        case CRC:
            return tr("CRC", "CRC hash code");
        case Method:
            return tr("Method", "Compression method");
        case Version:
            // Verze programu pro rozbalení
            return tr("Version", "Archive/Program version");
        case Timestamp:
            return tr("Date Modified", "Timestamp");
        case Comment:
            return tr("Comment", "File comment");
        default:
            return tr("??", "Unnamed column");
        }
    }
    return QVariant();
}

QModelIndex ArchiveModel::index(int row, int column, const QModelIndex &parent) const
{
    if (hasIndex(row, column, parent)) {
        ArchiveDirNode *parentNode = parent.isValid() ? static_cast<ArchiveDirNode*>(parent.internalPointer()) : m_rootNode;

        Q_ASSERT(parentNode->isDir());

        ArchiveNode *item = parentNode->entries().value(row, 0);
        if (item) {
            return createIndex(row, column, item);
        }
    }

    return QModelIndex();
}

QModelIndex ArchiveModel::parent(const QModelIndex &index) const
{
    if (index.isValid()) {
        ArchiveNode *item = static_cast<ArchiveNode*>(index.internalPointer());
        Q_ASSERT(item);
        if (item->parent() && (item->parent() != m_rootNode)) {
            return createIndex(item->parent()->row(), 0, item->parent());
        }
    }
    return QModelIndex();
}

ArchiveEntry ArchiveModel::entryForIndex(const QModelIndex &index)
{
    if (index.isValid()) {
        ArchiveNode *item = static_cast<ArchiveNode*>(index.internalPointer());
        Q_ASSERT(item);
        return item->entry();
    }
    return ArchiveEntry();
}

int ArchiveModel::childCount(const QModelIndex &index, int &dirs, int &files) const
{
    if (index.isValid()) {
        dirs = files = 0;
        ArchiveNode *item = static_cast<ArchiveNode*>(index.internalPointer());
        Q_ASSERT(item);
        if (item->isDir()) {
            const QList<ArchiveNode*> entries = static_cast<ArchiveDirNode*>(item)->entries();
            foreach(const ArchiveNode *node, entries) {
                if (node->isDir()) {
                    dirs++;
                } else {
                    files++;
                }
            }
            return entries.count();
        }
        return 0;
    }
    return -1;
}

int ArchiveModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() <= 0) {
        ArchiveNode *parentNode = parent.isValid() ? static_cast<ArchiveNode*>(parent.internalPointer()) : m_rootNode;

        if (parentNode && parentNode->isDir()) {
            return static_cast<ArchiveDirNode*>(parentNode)->entries().count();
        }
    }
    return 0;
}

int ArchiveModel::columnCount(const QModelIndex &parent) const
{
    return m_showColumns.size();
    if (parent.isValid()) {
        return static_cast<ArchiveNode*>(parent.internalPointer())->entry().size();
    }
}

void ArchiveModel::sort(int column, Qt::SortOrder order)
{
    if (m_showColumns.size() <= column) {
        return;
    }

    emit layoutAboutToBeChanged();

    QList<ArchiveDirNode*> dirNodes;
    m_rootNode->returnDirNodes(&dirNodes);
    dirNodes.append(m_rootNode);

    const ArchiveModelSorter modelSorter(m_showColumns.at(column), order);

    foreach(ArchiveDirNode* dir, dirNodes) {
        QVector < QPair<ArchiveNode*,int> > sorting(dir->entries().count());
        for (int i = 0; i < dir->entries().count(); ++i) {
            ArchiveNode *item = dir->entries().at(i);
            sorting[i].first = item;
            sorting[i].second = i;
        }

        qStableSort(sorting.begin(), sorting.end(), modelSorter);

        QModelIndexList fromIndexes;
        QModelIndexList toIndexes;
        for (int r = 0; r < sorting.count(); ++r) {
            ArchiveNode *item = sorting.at(r).first;
            toIndexes.append(createIndex(r, 0, item));
            fromIndexes.append(createIndex(sorting.at(r).second, 0, sorting.at(r).first));
            dir->setEntryAt(r, sorting.at(r).first);
        }

        changePersistentIndexList(fromIndexes, toIndexes);

        emit dataChanged(
            index(0, 0, indexForNode(dir)),
            index(dir->entries().size() - 1, 0, indexForNode(dir)));
    }

    emit layoutChanged();
}

QString ArchiveModel::cleanFileName(const QString& fileName)
{
    // "." is present in ISO files
    // "/" and "//" are in GNU Ar archives
    if ((fileName == "/") || (fileName == "//") || (fileName == ".")) {
        return QString();
    } else if (fileName.startsWith("./")) {
        return fileName.mid(2);
    }

    return fileName;
}

ArchiveDirNode* ArchiveModel::parentFor(const ArchiveEntry& entry)
{
    QStringList pieces = entry[ FileName ].toString().split(QLatin1Char( '/' ), QString::SkipEmptyParts);
    if (pieces.isEmpty()) {
        return NULL;
    }
    pieces.removeLast();

    if (m_previousMatch) {
        //the number of path elements must be the same for the shortcut to work
        if (m_previousPieces.count() == pieces.count()) {
            bool equal = true;

            //make sure all the pieces match up
            for (int i = 0; i < m_previousPieces.count(); ++i) {
                if (m_previousPieces.at(i) != pieces.at(i)) {
                    equal = false;
                    break;
                }
            }

            //if match return it
            if (equal) {
                return static_cast<ArchiveDirNode*>(m_previousMatch);
            }
        }
    }

    ArchiveDirNode *parent = m_rootNode;

    foreach(const QString &piece, pieces) {
        ArchiveNode *node = parent->find(piece);
        if (!node) {
            ArchiveEntry e;
            e[ FileName ] = (parent == m_rootNode) ?
                            piece : parent->entry()[ FileName ].toString() + QLatin1Char( '/' ) + piece;
            e[ IsDirectory ] = true;
            node = new ArchiveDirNode(parent, e);
            insertNode(node);
        }
        if (!node->isDir()) {
            ArchiveEntry e(node->entry());
            node = new ArchiveDirNode(parent, e);
            insertNode(node);
        }
        parent = static_cast<ArchiveDirNode*>(node);
    }

    m_previousMatch = parent;
    m_previousPieces = pieces;

    return parent;
}


QModelIndex ArchiveModel::indexForNode(ArchiveNode *node)
{
    Q_ASSERT(node);
    if (node != m_rootNode) {
        Q_ASSERT(node->parent());
        Q_ASSERT(node->parent()->isDir());
        return createIndex(node->row(), 0, node);
    }
    return QModelIndex();
}


void ArchiveModel::onEntryRemoved(const QString & path)
{
    qDebug() << "Removed node at path " << path;

    const QString entryFileName(cleanFileName(path));
    if (entryFileName.isEmpty()) {
        return;
    }

    ArchiveNode *entry = m_rootNode->findByPath(entryFileName.split(QLatin1Char( '/' ), QString::SkipEmptyParts));
    if (entry) {
        ArchiveDirNode *parent = entry->parent();
        QModelIndex index = indexForNode(entry);

        beginRemoveRows(indexForNode(parent), entry->row(), entry->row());

        parent->removeEntryAt(entry->row());

        endRemoveRows();
    } else {
        qDebug() << "Did not find the removed node";
    }

    if (m_rootNode->entries().count() == 1) {
        if (m_rootNode->entries().first()->isDir()) {
            m_archive.data()->setSingleFolderArchive(true);
            qDebug() << "Archive is SingleFolder.";
        }
    }
}

void ArchiveModel::onUserQuery(Query *query)
{
    // Execute query if GUI not redirect it
    qDebug("Warning: ArchiveModel execute querry");
    query->execute(NULL);
}

void ArchiveModel::addEntryToQueue(const ArchiveEntry& entry)
{
    //qDebug() << "ArchiveModel::addEntryToQueue()";

    // cache all entries
    m_newArchiveEntries.push_back(entry);
}

void ArchiveModel::onNewEntry(const ArchiveEntry& entry)
{
    addEntry(entry, NotifyViews);
}

void ArchiveModel::addEntry(const ArchiveEntry& receivedEntry, InsertBehaviour behaviour)
{
    //qDebug()<<"ArchiveModel::addEntry()";

    if (receivedEntry[FileName].toString().isEmpty()) {
        qDebug() << "Received empty entry (no filename) - skipping";
        return;
    }

    // Pokud seznam zobrazovaných sloupců prázdny, vytvořit na základě aktualní položky
    if (m_showColumns.isEmpty()) {
        // these are the columns we are interested in showing in the display
        static const QList<int> columnsForDisplay =
            QList<int>()
            << FileName
            << Size
            << CompressedSize
            << Permissions
            << Owner
            << Group
            << Ratio
            << CRC
            << Method
            << Version
            << Timestamp
            << Comment;

        QList<int> toInsert;

        foreach(int column, columnsForDisplay) {
            if (receivedEntry.contains(column)) {
                toInsert << column;
            }
        }
        beginInsertColumns(QModelIndex(), 0, toInsert.size() - 1);
        m_showColumns << toInsert;
        endInsertColumns();

        //qDebug() << "Show columns detected: " << m_showColumns;
    }

    // Make a copy
    ArchiveEntry entry = receivedEntry;

    // Filenames such as "./file" should be displayed as "file"
    // Entries called "/" should be ignored
    QString entryFileName = cleanFileName(entry[FileName].toString());
    if (entryFileName.isEmpty()) { // The entry contains only "." or "./"
        return;
    }
    entry[FileName] = entryFileName;

    // 1. Skip already created nodes
    if (m_rootNode) {
        ArchiveNode *existing = m_rootNode->findByPath(entry[ FileName ].toString().split(QLatin1Char( '/' )));
        if (existing) {
            qDebug() << "Refreshing entry for" << entry[FileName].toString();

            // Multi-volume files are repeated at least in RAR archives.
            // In that case, we need to sum the compressed size for each volume
            qulonglong currentCompressedSize = existing->entry()[CompressedSize].toULongLong();
            entry[CompressedSize] = currentCompressedSize + entry[CompressedSize].toULongLong();

            // Update entry
            existing->setEntry(entry);
            // Notify GUI
            QModelIndex iFrom = createIndex(existing->row(), 0, existing);
            QModelIndex iTo = createIndex(existing->row(), m_showColumns.size()-1, existing);
            emit dataChanged(iFrom, iTo);
            return;
        }
    }

    // 2. Find Parent Node, creating missing ArchiveDirNodes in the process
    ArchiveDirNode *parent = parentFor(entry);

    // 3. Create an ArchiveNode
    QString name = entry[ FileName ].toString().split(QLatin1Char( '/' ), QString::SkipEmptyParts).last();
    ArchiveNode *node = parent->find(name);
    if (node) {
        node->setEntry(entry);
    } else {
        if (entry[ FileName ].toString().endsWith(QLatin1Char( '/' )) || (entry.contains(IsDirectory) && entry[ IsDirectory ].toBool())) {
            node = new ArchiveDirNode(parent, entry);
        } else {
            node = new ArchiveNode(parent, entry);
        }
        insertNode(node, behaviour);
    }
}

void ArchiveModel::onLoadingFinished(QJob *job)
{
    //qDebug() << "ArchiveModel::onLoadingFinished()" << m_newArchiveEntries.count() << "New entrys";

    foreach(const ArchiveEntry &entry, m_newArchiveEntries)
    {
        addEntry(entry, DoNotNotifyViews);
    }
    reset();
    m_newArchiveEntries.clear();

    emit loadingFinished(job);
}

/* Insert the node into the model.*/
void ArchiveModel::insertNode(ArchiveNode *node, InsertBehaviour behaviour)
{
    Q_ASSERT(node);
    ArchiveDirNode *parent = node->parent();
    Q_ASSERT(parent);
    if (behaviour == NotifyViews) {
        beginInsertRows(indexForNode(parent), parent->entries().count(), parent->entries().count());
    }
    parent->appendEntry(node);
    if (behaviour == NotifyViews) {
        endInsertRows();
    }
}

Archive* ArchiveModel::archive() const
{
    return m_archive.data();
}

void ArchiveModel::listArchive()
{
    m_rootNode->clear();
    m_previousMatch = 0;
    m_previousPieces.clear();

    ListJob *job = NULL;

    m_newArchiveEntries.clear();
    if (m_archive && m_archive->exists()) {
        job = m_archive->list();

        connect(job, SIGNAL(newEntry(ArchiveEntry)), this, SLOT(addEntryToQueue(ArchiveEntry)));
        connect(job, SIGNAL(result(QJob*)), this, SLOT(onLoadingFinished(QJob*)));
        connect(job, SIGNAL(userQuery(Query*)), this, SLOT(onUserQuery(Query*)));

        connect(job, SIGNAL(error(QString,QString)), this, SIGNAL(error(QString,QString)));
        connect(job, SIGNAL(info(QString)), this, SIGNAL(info(QString)));

        emit loadingStarted(job);

        m_showColumns.clear();
    }
    reset();
    if (job) { job->start(); }
}

void ArchiveModel::setArchive(Archive *archive)
{
    beginResetModel();
    m_archive.reset(archive);
    emit archiveChanged();
    connect(archive, SIGNAL(codePageChanged(QString)), this, SLOT(onArchCodePageChanged(QString)));

    m_previousMatch = 0;
    m_previousPieces.clear();
    m_newArchiveEntries.clear();
    m_rootNode->clear();
    m_showColumns.clear();

    endResetModel();
}

bool ArchiveModel::isSingleFolderArchive()
{
    if (m_rootNode->entries().count() == 1) {
        if (m_rootNode->entries().first()->isDir()) {
            return true;
        }
    }
    return false;
}

bool ArchiveModel::isSingleFileArchive()
{
    if (m_rootNode->entries().count() == 1) {
        if (!m_rootNode->entries().first()->isDir()) {
            return true;
        }
    }
    return false;
}

QString ArchiveModel::subfolderName()
{
    // If single folder archive, return existing subfolder name,
    // else create subfolder name from archive name
    if (isSingleFolderArchive()) {
        return m_rootNode->entries().first()->name();
    }else {
        return archive()->createSubfolderName();
    }
}

OpenJob *ArchiveModel::openArchive()
{
    OpenJob *job = NULL;

    m_previousMatch = 0;
    m_previousPieces.clear();
    m_newArchiveEntries.clear();

    // removeColumnsAll
    beginRemoveColumns(indexForNode(m_rootNode), 0, m_showColumns.count() > 0 ? m_showColumns.count()-1 : 0);
    m_showColumns.clear();
    endRemoveColumns();
    // endRemoveColumnsAll

    beginResetModel();
    m_rootNode->clear();

    if (m_archive /*&& m_archive->exists()*/) {
        job = m_archive->open();
        connect(job, SIGNAL(newEntry(ArchiveEntry)), this, SLOT(onNewEntry(ArchiveEntry)));
        connect(job, SIGNAL(userQuery(Query*)), this, SLOT(onUserQuery(Query*)));
        connect(job, SIGNAL(error(QString,QString)), this, SIGNAL(error(QString,QString)));
        connect(job, SIGNAL(info(QString)), this, SIGNAL(info(QString)));
    }
    endResetModel();
    //reset();

    return job;


    Q_ASSERT(false);
}

bool ArchiveModel::isArchiveOpen() const
{
    return archive()->isOpen();
}

QString ArchiveModel::codePage() const
{
    return  archive()->codePage();
}

void ArchiveModel::setCodePage(const QString &codepage)
{
    archive()->setCodePage(codepage);
    listArchive();
}


ExtractJob* ArchiveModel::extractFile(const QVariant& fileName, const QString & destinationDir, const ExtractionOptions options) const
{
    QList<QVariant> files;
    files << fileName;
    return extractFiles(files, destinationDir, options);
}

ExtractJob* ArchiveModel::extractFiles(const QList<QVariant>& files, const QString & destinationDir, const ExtractionOptions options) const
{
    Q_ASSERT(m_archive);
    ExtractJob *newJob = m_archive->copyFiles(files, destinationDir, options);

    connect(newJob, SIGNAL(userQuery(Query*)), this, SLOT(onUserQuery(Query*)));

    return newJob;
}

AddJob* ArchiveModel::addFiles(const QStringList & filenames, const CompressionOptions& options)
{
    if (!m_archive) {
        return NULL;
    }

    if (!m_archive->isReadOnly()) {
        AddJob *job = m_archive->addFiles(filenames, options);

        connect(job, SIGNAL(newEntry(ArchiveEntry)), this, SLOT(onNewEntry(ArchiveEntry)));
        connect(job, SIGNAL(userQuery(Query*)), this, SLOT(onUserQuery(Query*)));

        return job;
    }
    return 0;
}

DeleteJob* ArchiveModel::deleteFiles(const QList<QVariant> & files)
{
    Q_ASSERT(m_archive);
    if (!m_archive->isReadOnly()) {
        DeleteJob *job = m_archive->deleteFiles(files);

        connect(job, SIGNAL(entryRemoved(QString)), this, SLOT(onEntryRemoved(QString)));
        connect(job, SIGNAL(finished(QJob*)), this, SLOT(cleanupEmptyDirs()));
        connect(job, SIGNAL(userQuery(Query*)), this, SLOT(onUserQuery(Query*)));

        return job;
    }
    return 0;
}

TestJob *ArchiveModel::testArchive()
{
    Q_ASSERT(m_archive);
    TestJob *newJob = m_archive->testArchive();

    connect(newJob, SIGNAL(userQuery(Query*)), this, SLOT(onUserQuery(Query*)));

    return newJob;
}

void ArchiveModel::cleanupEmptyDirs()
{
    qDebug();
    QList<QPersistentModelIndex> queue;
    QList<QPersistentModelIndex> nodesToDelete;

    // Add root nodes
    for (int i = 0; i < rowCount(); ++i) {
        queue.append(QPersistentModelIndex(index(i, 0)));
    }

    // Breadth-first search (Prohledávání do šířky)
    while (!queue.isEmpty()) {
        QPersistentModelIndex node = queue.takeFirst();
        ArchiveEntry entry = entryForIndex(node);
        //qDebug() << "Trying " << entry[FileName].toString();

        if (!hasChildren(node)) {
            if (!entry.contains(InternalID)) {
                nodesToDelete << node;
            }
        } else {
            for (int i = 0; i < rowCount(node); ++i) {
                queue.append(QPersistentModelIndex(index(i, 0, node)));
            }
        }
    }

    foreach(const QPersistentModelIndex& node, nodesToDelete) {
        ArchiveNode *rawNode = static_cast<ArchiveNode*>(node.internalPointer());
        qDebug() << "Delete with parent entries " << rawNode->parent()->entries() << " and row " << rawNode->row();
        beginRemoveRows(parent(node), rawNode->row(), rawNode->row());
        rawNode->parent()->removeEntryAt(rawNode->row());
        endRemoveRows();
        //qDebug() << "Removed entry " << entry[FileName].toString();
    }
}

void ArchiveModel::onArchCodePageChanged(const QString &codepage)
{
    emit codePageChanged(codepage);
}
