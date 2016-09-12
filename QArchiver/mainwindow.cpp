#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QInputDialog>
#include <QFileDialog>
#include <QDockWidget>
#include <QCheckBox>
#include <QMessageBox>
#include <QDebug>
#include <QTextCodec>
#include <QTimer>
#include <QVariant>
#include <QDesktopServices>

#include <QUrl>
#include <QDragEnterEvent>

#include "jobs.h"
#include "extractdialog.h"
#include "propertiesdialog.h"
#include "settingsdialog.h"
#include "progressdialog.h"

#include "QtExt/qduoled.h"
#include "QtExt/qclickablelabel.h"
#include "QtExt/qcbmessagebox.h"
#include "qstandarddirs.h"
#include "Codecs/textencoder.h"
#include "infoframe.h"
#include "filesystemmodel.h"

#define DEFAULT_CP_TEXT ""
//#define DEFAULT_CP_TEXT "Default"
//#define DEFAULT_CP_TEXT " -- "



void MainWindow::debugTest_QTreeView()
{
    // Test
    qDebug() << "ui->treeViewContent->selectionModel():" << ui->treeViewContent->selectionModel();
    qDebug() << "ui->treeViewContent->model():" << ui->treeViewContent->model();

    ui->treeViewContent->setModel(NULL);
    qDebug() << "ui->treeViewContent->selectionModel():" << ui->treeViewContent->selectionModel();
    qDebug() << "ui->treeViewContent->model():" << ui->treeViewContent->model();

    ui->treeViewContent->setModel(m_fileSystemModel);
    qDebug() << "ui->treeViewContent->selectionModel():" << ui->treeViewContent->selectionModel();
    qDebug() << "ui->treeViewContent->model():" << ui->treeViewContent->model();

    ui->treeViewContent->setModel(m_fileSystemModel);
    qDebug() << "ui->treeViewContent->selectionModel():" << ui->treeViewContent->selectionModel();
    qDebug() << "ui->treeViewContent->model():" << ui->treeViewContent->model();

    /* Ze začátku oba NULL */
    // ui->treeViewContent->selectionModel(): QObject(0x0)
    // ui->treeViewContent->model(): QObject(0x0)

    /* Vložen NULL view model -> nový selection model se vytvořil */
    // ui->treeViewContent->selectionModel(): QItemSelectionModel(0x2a78690)
    // ui->treeViewContent->model(): QObject(0x0)

    /* Vložen platný fileSystemModel -> vytvořil se nový selection model */
    // ui->treeViewContent->selectionModel(): QItemSelectionModel(0x2a78700)
    // ui->treeViewContent->model(): QFileSystemModel(0x1a0de68)

    /* Vložen znova ten samý model -> nic se nestalo */
    // ui->treeViewContent->selectionModel(): QItemSelectionModel(0x2a78700)
    // ui->treeViewContent->model(): QFileSystemModel(0x1a0de68)
}

void MainWindow::setupModelsAndViews()
{
    m_dirModel = new FileSystemModel(this);
    // Disable modifying file system
    m_dirModel->setReadOnly(true);
    m_dirModel->setRootPath("");
    //qDebug() << "Default:        " << m_dirModel->filter();
    m_dirModel->setFilter(QDir::Dirs | QDir::AllDirs | QDir::NoDotAndDotDot);
    //qDebug() << "DirModel:       " << m_dirModel->filter();

    m_fileSystemModel = new FileSystemModel(this);
    m_fileSystemModel->setReadOnly(true);
    m_fileSystemModel->setRootPath("");
    m_fileSystemModel->setFilter(QDir::AllEntries);
    //qDebug() << "FileSystemModel:" << m_fileSystemModel->filter();

    // Setup sort criteria (funguje jen na QDirModel)
    // directory first, ignore case, and sort by name
    // m_fileSystemModel->setSorting(QDir::DirsFirst | QDir::IgnoreCase | QDir::Name);

    // Direcrories
    ui->treeView->setModel(m_dirModel);
    // Set initial selection
    // Set current directory selection
    m_fsLastDirPath = QDir::currentPath();
    QModelIndex currentIndex = m_dirModel->index(m_fsLastDirPath);
    // Highlight the selected
    ui->treeView->setCurrentIndex(currentIndex);
    // for the selected directory as expanded
    ui->treeView->expand(currentIndex);
    // Resizing the column - first column
    ui->treeView->resizeColumnToContents(0);
    // Make it scroll to the selected
    ui->treeView->scrollTo(currentIndex,QAbstractItemView::PositionAtCenter);
    ui->treeView->hideColumn(1);// for hide Size Column
    ui->treeView->hideColumn(2);// for hide Type Column
    ui->treeView->hideColumn(3);// for hide Date Modified Column

    // Test
    //debugTest_QTreeView();

    // Directory content view
    ui->treeViewContent->setModel(m_fileSystemModel);
    //ui->treeViewContent->setExpandsOnDoubleClick(false);
    ui->treeViewContent->setItemsExpandable(false);
    ui->treeViewContent->setRootIsDecorated(false);
    ui->treeViewContent->setSortingEnabled(true);
    //ui->treeViewContent->setAllColumnsShowFocus(true);
    //ui->treeViewContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //ui->treeViewContent->setRootIndex(m_fileSystemModel->index(m_dirModel->filePath(currentIndex)));
    ui->treeViewContent->setRootIndex(m_fileSystemModel->index(m_fsLastDirPath));
    ui->treeViewContent->header()->resizeSection(0, 150);
    ui->treeViewContent->sortByColumn(0, Qt::AscendingOrder);
    //connect(ui->treeViewContent,SIGNAL(doubleClicked(QModelIndex)), ui->treeViewContent, SLOT(setRootIndex(QModelIndex))); //Autoconnect
    /* CustomContextMenu */
    //ui->treeViewContent->setContextMenuPolicy(Qt::CustomContextMenu);
    //ui->treeViewContent->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    //connect(ui->treeViewContent->header(),SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(on_treeViewContent_customContextMenuRequested(QPoint)));
    //qDebug() << "Actions" << ui->treeViewContent->header()->actions();
    // Save initial header state
    m_treeViewContentHeaderState = ui->treeViewContent->header()->saveState();
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_archiveManager(ArchiveToolManager::Instance()),
    m_busy(false)
{
    ui->setupUi(this);
    ui->bottomWidget->setVisible(false);
    ui->menuTools->menuAction()->setVisible(false);
    m_jobProgress = NULL;
    m_archiveModel = NULL;
    m_archiveMode = false;

    // Statusbar Codepage Label
    cpsLabel = new QClickableLabel();
    cpsLabel->setMinimumWidth(40);
    cpsLabel->setText(DEFAULT_CP_TEXT);
    cpsLabel->setToolTip(tr("Codepage"));
    cpsLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    connect(cpsLabel, SIGNAL(clicked()), this, SLOT(slotChangeCodepage()));
    statusBar()->addPermanentWidget(cpsLabel);

    // Statusbar LED
    busyLed = new QDuoLed(statusBar());
    busyLed->setColor1(QColor(16, 184, 8));
    busyLed->setColor2(QColor(184, 16, 8));
    busyLed->turnColor(1);
    busyLed->turnOn(true);
    busyLed->setToolTip(tr("Ready"));
    statusBar()->addPermanentWidget(busyLed);

    createMenuView();
    this->setWindowTitle(qApp->applicationName());
    //setWindowTitle(QString::null);
    //setWindowTitle(QString()); //pokud null, title bude windowFilePath + appName
    this->setWindowIcon(QIcon::fromTheme("qarchiver"));

    // setup actions
    setupActions();
    createMenuActions();
    // povoli/zakaze Actions
    updateActions();

    // Set application curent directory
    QDir::setCurrent(QDir::homePath());

    // Create and populate our model
    setupModelsAndViews();

    // Read app configuration
    readSettings();

    createInfoDockWindow();

    // Enable drop operation on window
    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    writeSettings();

    delete ui;

    for (int i = 0; i < m_tmpFiles.size(); ++i) {
        QFile *file = m_tmpFiles.at(i);
        if (!file->remove()) {
            qWarning("Warning: Soubor %s se nepodařilo smazat.", file->fileName().toLocal8Bit().constData());
        } else {
            qDebug("Info: Soubor %s úspěšně smazán.", file->fileName().toLocal8Bit().constData());
        }

        delete file;
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << event;

    if (this->isBusy()) {
        return;
    }

    if ((event->source() == NULL) && isSingleUrlDrag(event->mimeData())) {
        event->acceptProposedAction();
    }
    return;
}

void MainWindow::dropEvent(QDropEvent *event)
{
    qDebug() << event;

    if (this->isBusy()) {
        return;
    }

    if ((event->source() == NULL) && isSingleUrlDrag(event->mimeData())) {
        event->acceptProposedAction();
    }

    QUrl url = event->mimeData()->urls().at(0);

    if (!url.isEmpty()) {
        if (url.isLocalFile()) {
            // open local file
            openArchive(url.toLocalFile());
        }
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    qDebug() << "MainWindow::dragMoveEvent(" << event << ")";

    if (this->isBusy()) {
        return;
    }

    if ((event->source() == NULL) && isSingleUrlDrag(event->mimeData())) {
        event->acceptProposedAction();
    }
}

bool MainWindow::isSingleUrlDrag(const QMimeData *data)
{
    return ((data->hasUrls()) && (data->urls().count() == 1));
}

bool MainWindow::openArchive(const QString &path)
{
    const bool open_ok = openFile(path);
    if (open_ok) {
        this->setWindowTitle(qApp->applicationName().append(" - " + path));
        this->setWindowFilePath(path);
        m_recentFilesMenu->addFile(path);
    } else {
        // Otevření se nezdařilo
        m_recentFilesMenu->removeFile(path);
        if (m_archiveMode) {
            // Pokud "ArchiveMode" -> přepnout na "FileSystemMode"
            closeArchive();
        }
    }
    return open_ok;
}

void MainWindow::setShowExtractDialog(bool option)
{
    if (option) {
        m_openArgs.setOption(QLatin1String( "showExtractDialog" ), QLatin1String( "true" ));
    } else {
        m_openArgs.removeOption(QLatin1String( "showExtractDialog" ));
    }
}

void MainWindow::setupActions()
{
    /* "File" -> "New Archive" */
    ui->actionNew_archive->setShortcut(QKeySequence(QLatin1String( "Ctrl+N" )));
    //connect(ui->actionNew_archive, SIGNAL(triggered(bool)), this, SLOT(newArchive()));
    // autoconected to on_actionNew_archive_triggered();

    /* File -> "Open Archive" */
    ui->actionOpen_archive->setShortcut(QKeySequence(QLatin1String( "Ctrl+O" )));
    ui->actionOpen_archive->setIcon(QIcon::fromTheme("document-open", QIcon(":/actions/archive-open")));
    //connect(ui->actionOpen_archive, SIGNAL(triggered(bool)), this,  SLOT(openArchive()));
    // autoconected to on_actionOpen_archive_triggered();

    /* "File" -> "Properties" */
    ui->actionProperties->setEnabled(m_archiveMode);
    connect(ui->actionProperties,SIGNAL(triggered(bool)), this, SLOT(onActionProperties()));

    /* "File" -> "Close Archive" */
    ui->actionClose_archive->setShortcut(QKeySequence(QLatin1String( "Ctrl+W" )));
    ui->actionClose_archive->setEnabled(m_archiveMode);
    //connect(ui->actionClose_archive, SIGNAL(triggered(bool)), this, SLOT(closeArchive()));
    // autoconnected to on_actionClose_archive_triggered();

    /* "File" -> "Recent Files" */
    m_recentFilesMenu = new QRecentFilesMenu(tr("Recent Files"), this);
    m_recentFilesMenu->setFormat("%d: %s [%p]");
    connect(m_recentFilesMenu, SIGNAL(recentFileTriggered(const QString &)), this, SLOT(loadRecentFile(const QString &)));
    ui->actionRecent_files->setMenu(m_recentFilesMenu);
    ui->actionRecent_files->setIcon(QIcon::fromTheme("document-open-recent", QIcon(":/actions/open-recent")));

    /* "File" -> "Quit" */
    ui->actionQuit->setShortcut(QKeySequence(QKeySequence::Quit));
    ui->actionQuit->setIcon(QIcon::fromTheme("application-exit", QIcon(":/actions/application-exit")));
    ui->actionQuit->setToolTip(tr("Quit the application"));
    connect(ui->actionQuit, SIGNAL(triggered(bool)), this, SLOT(close()));

    /* "Actions" -> "Add-file" */
    //ui->actionAdd->setText(tr("Add &File..."));
    ui->actionAdd->setIcon(QIcon::fromTheme("archive-insert", QIcon(":/actions/archive-insert")));
    ui->actionAdd->setToolTip(tr("Add files to the archive"));
    ui->actionAdd->setStatusTip(tr("Click to add files to the archive")); //Zobrazuje se ve status panelu
    connect( ui->actionAdd, SIGNAL(triggered(bool)), this, SLOT(slotAddFiles()));

    /* "Actions" ->  "Add-dir" */
    m_addDirAction = new QAction("Add Folder", this);
    m_addDirAction->setIcon(QIcon::fromTheme( QLatin1String( "archive-insert-directory" ), QIcon(":/actions/archive-insert-dir")));
    m_addDirAction->setText(tr("Add Fo&lder..."));
    m_addDirAction->setToolTip(tr("Add a folder to the archive"));
    m_addDirAction->setStatusTip(tr("Click to add a folder to the archive"));
    connect(m_addDirAction, SIGNAL(triggered(bool)), this, SLOT(slotAddDir()));
    //ui->mainToolBar->addAction(m_addDirAction); // Vlozi na konec
    ui->mainToolBar->insertAction(ui->actionExtract, m_addDirAction); // Vlozi pred actionExtract

    /* "Actions" -> "Extract" */
    //ui->actionExtract->setText(tr("E&xtract"));
    ui->actionExtract->setIcon(QIcon::fromTheme("archive-extract", QIcon(":/actions/archive-extract")));
    ui->actionExtract->setShortcut(QKeySequence( QLatin1String( "Ctrl+E" ) ));
    ui->actionExtract->setToolTip(tr("Extract all files or just the selected ones"));
    ui->actionExtract->setStatusTip(tr("Click to extract either all files or just the selected ones"));
    connect(ui->actionExtract, SIGNAL(triggered(bool)), this, SLOT(slotExtractFiles()));

    /* "Actions" -> "Delete" */
    //ui->actionDelete->setText(tr("De&lete"));
    ui->actionDelete->setIcon(QIcon::fromTheme( "archive-remove", QIcon(":/actions/archive-remove")));
    ui->actionDelete->setShortcut(Qt::Key_Delete);
    ui->actionDelete->setToolTip(tr("Delete files"));
    ui->actionDelete->setStatusTip(tr("Click to delete the selected files"));
    connect(ui->actionDelete, SIGNAL(triggered(bool)), this, SLOT(slotDeleteFiles()));

    /* "Actions" -> "Preview" */
    m_previewAction = new QAction(tr("Preview"), this);
    m_previewAction->setText(tr("Pre&view", "to preview a file inside an archive"));
    m_previewAction->setIcon(QIcon::fromTheme("document-preview-archive",QIcon(":/actions/document-preview")));
    m_previewAction->setToolTip(tr("Preview file"));
    m_previewAction->setStatusTip(tr("Click to preview the selected file"));
    connect(m_previewAction, SIGNAL(triggered(bool)), this, SLOT(slotPreview()));
    ui->mainToolBar->addAction(m_previewAction); // Vloží na konec

    /* "Actions" -> "Codepage" */
    m_codepageAction = new QAction(tr("Codepage"), this);
    m_codepageAction->setText(tr("Codepage...", "filename encoding inside an archive"));
    m_codepageAction->setIcon(QIcon::fromTheme("preferences-desktop-locale", QIcon(":/actions/archive-codepage")));
    m_codepageAction->setToolTip(tr("Filename Codepage"));
    m_codepageAction->setStatusTip(tr("Click to change filename encoding."));
    connect(m_codepageAction, SIGNAL(triggered(bool)), this, SLOT(slotChangeCodepage()));
    ui->mainToolBar->addAction(m_codepageAction); // Vloží na konec

    /* "Help" -> "About" */
    m_aboutAction = new QAction("About...",this);
    m_aboutAction->setObjectName(QString::fromUtf8("aboutAction"));
    m_aboutAction->setIcon(QIcon::fromTheme("help-about", QIcon(":/actions/help-about")));
    connect(m_aboutAction, SIGNAL(triggered()), this, SLOT(onAbout()));
    ui->menuHelp->addAction(m_aboutAction);

    /* "Options" -> "Settings" */
    m_settingsDialog = new SettingsDialog(this);
    m_settingsAction = new QAction(tr("Settings..."), this);
    connect(m_settingsAction, SIGNAL(triggered()), m_settingsDialog, SLOT(exec()));
    ui->menuOptions->addAction(m_settingsAction);
}

void MainWindow::createMenuView()
{
    menuView = new QMenu(ui->menuBar);
    menuView->setTitle(tr("View"));
    menuBar()->insertMenu(ui->menuActions->menuAction(),menuView);

    // "View" -> "Toolbar"
    menuView->addAction(ui->mainToolBar->toggleViewAction());

    // "View" -> "Status Bar"
    QAction *actionShowStatusbar = new QAction(menuView);
    actionShowStatusbar->setText(tr("Status Bar"));
    actionShowStatusbar->setCheckable(true);
    actionShowStatusbar->setChecked(true);
    menuView->addAction(actionShowStatusbar);
    connect(actionShowStatusbar, SIGNAL(toggled(bool)),statusBar(), SLOT(setVisible(bool)));

    // Spodni panel - pro testovací účely
    QAction *actionShowBottPanel = new QAction(menuView);
    actionShowBottPanel->setVisible(false);
    actionShowBottPanel->setText(tr("Panel"));
    actionShowBottPanel->setCheckable(true);
    actionShowBottPanel->setChecked(ui->bottomWidget->isVisible());
    menuView->addAction(actionShowBottPanel);
    connect(actionShowBottPanel, SIGNAL(toggled(bool)),ui->bottomWidget, SLOT(setVisible(bool)));

    // Edit menu -> Select All
    QAction *actionSelectAll = new QAction(ui->menuEdit);
    actionSelectAll->setText(tr("Select All"));
    ui->menuEdit->addAction(actionSelectAll);
    connect(actionSelectAll, SIGNAL(triggered()), ui->treeViewContent, SLOT(selectAll()));

    // Edit menu -> Deselect All
    QAction *deselectAll = new QAction(ui->menuEdit);
    deselectAll->setText(tr("Deselect All"));
    ui->menuEdit->addAction(deselectAll);
    connect(deselectAll, SIGNAL(triggered()), ui->treeViewContent, SLOT(clearSelection()));
}

void MainWindow::createMenuActions()
{
    ui->menuActions->addAction(ui->actionTest);
    ui->menuActions->addAction(ui->actionAdd);
    ui->menuActions->addAction(m_addDirAction);
    ui->menuActions->addAction(ui->actionExtract);
    ui->menuActions->addAction(ui->actionDelete);
    ui->menuActions->addAction(m_previewAction);
    ui->menuActions->addAction(m_codepageAction);
}

void MainWindow::createInfoDockWindow()
{
    QDockWidget *dock = new QDockWidget(tr("Information"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->hide();

    m_infoFrame = new InfoFrame(dock);
    dock->setWidget(m_infoFrame);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    // "View" -> "Information"
    QAction *dockViewActtion = dock->toggleViewAction();
    dockViewActtion->setEnabled(true);
    menuView->addAction(dockViewActtion);
}

bool MainWindow::isSingleEntryArchive() const
{
    //return m_archiveModel->archive()->isSingleFolderArchive();
    return m_archiveModel->isSingleFolderArchive() || m_archiveModel->isSingleFileArchive();
}

QString MainWindow::detectSubfolder() const
{
    if (!m_archiveModel) {
        return QString();
    }

    return m_archiveModel->subfolderName();
}

QList<QVariant> MainWindow::selectedFiles()
{
    QStringList toSort;

    foreach(const QModelIndex & index, ui->treeViewContent->selectionModel()->selectedRows()) {
        const ArchiveEntry& entry = m_archiveModel->entryForIndex(index);
        toSort << entry[ InternalID ].toString();
    }

    toSort.sort();
    QVariantList ret;
    foreach(const QString &i, toSort) {
        ret << i;
    }
    return ret;
}

QList<QVariant> MainWindow::selectedFilesWithChildren()
{
    Q_ASSERT(m_archiveModel);

    QModelIndexList toIterate = ui->treeViewContent->selectionModel()->selectedRows();

    for (int i = 0; i < toIterate.size(); ++i) {
        QModelIndex index = toIterate.at(i);

        for (int j = 0; j < m_archiveModel->rowCount(index); ++j) {
            QModelIndex child = m_archiveModel->index(j, 0, index);
            if (!toIterate.contains(child)) {
                toIterate << child;
            }
        }
    }

    QVariantList ret;
    foreach(const QModelIndex & index, toIterate) {
        const ArchiveEntry& entry = m_archiveModel->entryForIndex(index);
        if (entry.contains(InternalID)) {
            ret << entry[ InternalID ];
        }
    }
    return ret;
}

QStringList MainWindow::suffixesFromFilterStr(const QString &filterStr)
{
    // Selected Filter: "Archiv 7-zip (*.7z)";
    int suffixesBegin = filterStr.indexOf("(*", 0);
    int suffixesEnd = filterStr.indexOf(")", suffixesBegin);
    QString suffixes;
    QStringList suffList;
    //qDebug() << "Begin" << suffixesBegin << "End" << suffixesEnd;
    if (suffixesBegin > -1 && suffixesEnd > -1) {
        // *.7z
        suffixes = filterStr.mid(suffixesBegin+1, suffixesEnd-suffixesBegin-1);
        //qDebug() << suffix;
        // .7z
        suffList = suffixes.split(QRegExp("\\s{0,1}\\*{1}"), QString::SkipEmptyParts);
        //qDebug() << suffix;
    }

    return suffList;
}

QString MainWindow::suffixFromFilterStr(const QString &filterStr)
{
    // Selected Filter: "Archiv 7-zip (*.7z)";
    int suffixesBegin = filterStr.indexOf("(*", 0);
    int suffixesEnd = filterStr.indexOf(")", suffixesBegin);
    QString suffix;
    //qDebug() << "Begin" << suffixesBegin << "End" << suffixesEnd;
    if (suffixesBegin > -1 && suffixesEnd > -1) {
        // *.7z
        suffix = filterStr.mid(suffixesBegin+1, suffixesEnd-suffixesBegin-1);
        //qDebug() << suffix;
        // .7z
        suffix = suffix.split(QRegExp("\\s{0,1}\\*{1}"), QString::SkipEmptyParts).first();
        //qDebug() << suffix;
    }

    return suffix;
}

bool MainWindow::hasSuffix(const QString &fileName, const QStringList &suffixes)
{
    for (int i = 0; i < suffixes.count(); ++i) {
        if (fileName.endsWith(suffixes[i])) {
            return true;
        }
    }
    return false;
}

QString MainWindow::getSaveFileName(QString *selectedFilter, QString directory)
{
    QFileDialog saveDialog(this,tr("Create Archive"), directory, ArchiveToolManager::filterForFiles(Create));
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setFileMode(QFileDialog::AnyFile);
    saveDialog.setOptions(QFileDialog::DontConfirmOverwrite);
    saveDialog.setConfirmOverwrite(false);
    saveDialog.setDirectory(directory);
    qDebug() <<endl << saveDialog.directory().absolutePath();
    //saveDialog.selectFile("");
    saveDialog.selectFile("archive.7z");

    saveDialog.exec();
    *selectedFilter = saveDialog.selectedNameFilter();

    QString fileName = saveDialog.selectedFiles().first();
    QDir dir = saveDialog.directory();

    return fileName;
}

void MainWindow::on_actionNew_archive_triggered()
{
    QString filterStr = ArchiveToolManager::filterForFiles(Create);
    QStringList filters = filterStr.split(QLatin1String(";;"));
    QString defaultSuffix = suffixFromFilterStr(filters.first());
    QString directory = QDir::currentPath();
    QString archiveName = "archive";
    QString selectedFilter;
    QString fileName;
    bool  ok = false;

    do {
        fileName = QFileDialog::getSaveFileName(this, tr("Create Archive"), directory + "/" + archiveName + defaultSuffix,
                                                filterStr, &selectedFilter,
                                                QFileDialog::DontConfirmOverwrite);

        if (!fileName.isEmpty())
        {
            //qDebug() << "FileName:" << fileName << "Selected Filter:" << selectedFilter;
            QStringList suffixes = suffixesFromFilterStr(selectedFilter);
            if (!hasSuffix(fileName, suffixes)) {
                // Přidej suffix
                fileName = fileName + suffixes.first();
            }

            const QFileInfo fileInfo(fileName);
            archiveName = fileInfo.baseName();
            QString parentDirPath = fileInfo.absolutePath();
            const QFileInfo dirInfo(parentDirPath);

            if (dirInfo.exists() && dirInfo.isDir()) {
                if (dirInfo.isExecutable() && dirInfo.isWritable()) {
                    // OK;
                    ok = true;
                } else {
                    // Could not create file, Dir is not writable
                    QMessageBox::critical(this, tr("Error", "@title:window"), tr("\nCould not create file: <i>%1</i>.\nPermission denied!").arg(fileName));
                }
            }else
            {
                // Could not create file, No such directory
                QMessageBox::warning(this, tr("No such directory", "@title:window"), tr("<qt><br/>Directory <i>%1</i> does not exist!</qt>").arg(parentDirPath));
            }
        }

    } while (!(ok || fileName.isEmpty()));

    if (fileName.isEmpty()) {
        return;
    }

    if (createFile(fileName)) {
        this->setWindowTitle(qApp->applicationName().append(" - " + fileName));
        this->setWindowFilePath(fileName);
        m_recentFilesMenu->addFile(fileName);
    } else {
        m_recentFilesMenu->removeFile(fileName);
    }

    m_openArgs.removeOption(QLatin1String("showExtractDialog"));

    return;
}


void MainWindow::on_actionOpen_archive_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Archive"),  QDir::currentPath(),
                                                    ArchiveToolManager::filterForFiles(ReadOnly), 0);
    if (fileName.isEmpty()) {
        // Canceled
        return;
    }

    openArchive(fileName);
}

void MainWindow::on_actionClose_archive_triggered()
{
    closeArchive();
}


void MainWindow::on_actionGo_Up_triggered()
{
    QModelIndex rootIndex = ui->treeViewContent->rootIndex();
    QModelIndex parent = rootIndex.parent();
    ui->treeViewContent->setRootIndex(parent);
    ui->treeViewContent->setCurrentIndex(parent);
}


void MainWindow::closeArchive()
{
    //qDebug() << "--- MainWindow::closeArchive() ---";

    // Nastavit treeView -> m_dirModel, treeViewContent -> fileSystemModel
    viewFileSystem();

    // Zavřít aktualní archiv
    // Smazat archive model
    delete m_archiveModel;
    m_archiveModel = NULL;
    m_infoFrame->setArchiveModel(m_archiveModel);

    m_archiveMode = false;
    ui->actionClose_archive->setEnabled(m_archiveMode);
    ui->actionProperties->setEnabled(m_archiveMode);
    updateActions();

    //cpsLabel->setText(" -- ");
    cpsLabel->setText(DEFAULT_CP_TEXT);
    this->setWindowTitle(qApp->applicationName());
    this->setWindowFilePath(QString::null);
}


bool MainWindow::openFile(const QString &filePath)
{
    qDebug() << "MainWindow::openFile(" << filePath << ")";

    if (filePath.isEmpty()) {
        qDebug("Warning: openFile(): Empty filePath!");
        return false;
    }

    const QFileInfo fileInfo(filePath);
    qDebug() << " Opening:" << fileInfo.absoluteFilePath();

    if (fileInfo.isDir()) {
        QMessageBox::critical(this, tr("Error", "@title:window"),
                              tr("<i>%1</i> is a directory.").arg(filePath));
        return false;
    }

    if (!fileInfo.exists()) {
        QMessageBox::critical(this, tr("Error", "@title:window"),
                              tr("The archive <i>%1</i> was not found.").arg(filePath));
        return false;
    }

    if (!m_archiveModel) {
        // If archiveModel not exist, create new
        m_archiveModel = new ArchiveModel(this);
        connect(m_archiveModel, SIGNAL(info(QString)), this, SLOT(showInfoMessage(QString)));
        connect(m_archiveModel, SIGNAL(codePageChanged(QString)), cpsLabel, SLOT(setText(QString)));
        connect(m_archiveModel, SIGNAL(archiveChanged()), cpsLabel, SLOT(clear()));
        viewArchive();
        connect(ui->treeViewContent->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
        connect(ui->treeViewContent->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged()));
    }

    QMimeType mimeType = ArchiveToolManager::determineQMimeType(filePath);

    QScopedPointer<Archive> archive(ArchiveToolManager::createArchive(mimeType, filePath, m_archiveModel));

    if (!archive) {
        QStringList mimeTypeList;
        QHash<QString, QString> mimeTypes;

        mimeTypeList = ArchiveToolManager::supportedMimeTypes();

        QMimeDatabase db;
        foreach(const QString& mime, mimeTypeList) {
            QMimeType mimeType = db.mimeTypeForName(mime);
            if (mimeType.isValid()) {
                // Key = "application/zip", Value = "Zip Archive"
                mimeTypes[mime] = mimeType.comment();
            }
        }

        QStringList mimeComments(mimeTypes.values());
        mimeComments.sort();

        bool ok;
        QString item = QInputDialog::getItem(this, tr("Unable to Determine Archive Type", "@title:window"),
                                         tr("QArchiver was unable to determine the archive type.\n\nPlease choose the correct archive type below.", "@info"),
                                         mimeComments, 0, false, &ok);

        if (!ok || item.isEmpty()) {
            return false;
        }

        // Vytvoří nový objekt třídy Archive pro nově zvolený MimeTyp, původní objekt bude automaticky dealokován
        archive.reset(ArchiveToolManager::createArchive(mimeTypes.key(item), filePath, m_archiveModel));
    }

    if (!archive) {
        QMessageBox::critical(this, tr("Error Opening Archive", "@title:window"),
                              tr("<qt>QArchiver was not able to open the archive <i>%1</i>. No plugin capable of handling the file was found.</qt>", "@info").arg(filePath));
        return false;
    }

    // Odebrání ukazatele na archiv z QScopedPointer -> přiřazení do archiveModel,
    // ukazatel uvnitř QScopedPointer resetovan na NULL
    m_archiveModel->setArchive(archive.take());

    OpenJob* job = m_archiveModel->openArchive();
    registerJob(job);
    connect(job, SIGNAL(result(QJob*)), this, SLOT(slotOpenDone(QJob*)));
    job->start();

    updateActions();

//    if (m_openArgs.getOption(QLatin1String( "showExtractDialog" )) == QLatin1String( "true" )) {
//        QTimer::singleShot(0, this, SLOT(slotExtractFiles()));
//    }

    return true;
}


bool MainWindow::createFile(const QString &filePath)
{
    const QFileInfo fileInfo(filePath);

    qDebug() << "MainWindow::createFile(" << filePath << ")";
    qDebug() << "  Creating archive:" << fileInfo.absoluteFilePath();

    if (fileInfo.exists()) {
        if (fileInfo.isDir()) {
            // Exsists and isDir
            QMessageBox::critical(this, tr("Error", "@title:window"), tr("<qt><br/><i>%1</i> is a directory.</qt>").arg(filePath));
            return false;
        }
        else {
            // Exsists and isFile
            int ret = QMessageBox::question(this, tr("File Exists", "@title:window"),
                                               tr("<qt><br/>The archive <i>%1</i> already exists. Would you like to open it instead?</qt>", "@info").arg(filePath),
                                               tr("Open File"), tr("Cancel"));
            // "Open File": ret = 0, "Cancel": ret = 1
            if (ret == 0) {
                return openFile(filePath);
            } else {
                return false;
            }
        }
    }

    if (!m_archiveModel) {
        // If archiveModel not exist, create new
        m_archiveModel = new ArchiveModel(this);
        connect(m_archiveModel, SIGNAL(info(QString)), this, SLOT(showInfoMessage(QString)));
        connect(m_archiveModel, SIGNAL(codePageChanged(QString)), cpsLabel, SLOT(setText(QString)));
        connect(m_archiveModel, SIGNAL(archiveChanged()), cpsLabel, SLOT(clear()));
        viewArchive();
        connect(ui->treeViewContent->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
        connect(ui->treeViewContent->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged()));
    }

    QMimeType mimeType = ArchiveToolManager::determineQMimeType(fileInfo.absoluteFilePath());
    QStringList suportedTypes = ArchiveToolManager::supportedWriteMimeTypes();

    if (!suportedTypes.contains(mimeType.name())) {
        QMessageBox::critical(this, tr("Error"),
                              tr("<qt>QArchiver was not able to create the archive <i>%1</i>."
                                 " <br/><br/>Unidentified archive format.</qt>", "@info").arg(fileInfo.fileName()));
        return false;
    }

    // Create Archive object for file
    Archive* archive = ArchiveToolManager::createArchive(mimeType, filePath, m_archiveModel);

    if (!archive) {
        QMessageBox::critical(this, tr("Error", "@title:window"),
                              tr("<qt>QArchiver was not able to create the archive <i>%1</i>."
                                 " No plugin capable of handling the file was found.</qt>", "@info").arg(filePath));
        return false;
    }

    m_archiveModel->setArchive(archive);

//    OpenJob* job = m_archiveModel->openArchive();
//    registerJob(job);
//    connect(job, SIGNAL(result(QJob*)), this, SLOT(slotCreateDone(QJob*)));
//    job->start();

    updateActions();

    return true;
}


void MainWindow::loadRecentFile(const QString &filePath)
{
    qDebug() << "--- MainWindow::loadRecentFile() ---";

    const bool open_ok = openFile(filePath);
    if (open_ok) {
        this->setWindowTitle(qApp->applicationName().append(" - " + filePath));
        this->setWindowFilePath(filePath);
        //m_recentFilesMenu->addFile(filePath);
    } else {
        m_recentFilesMenu->removeFile(filePath);
    }
}


void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    // zobrazi velikost okna v titulku - jen pro testovací ucely
    //this->setWindowTitle(QString("%1 :: %2x%3").arg(title).arg(size().width()).arg(size().height()));
}


void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    // Synchronize views
    if (m_archiveMode) {
        ui->treeViewContent->setRootIndex(m_archiveDirModel->mapToSource(index));
    } else {
        QString dirPath = m_dirModel->filePath(index);
        QFileInfo dirInfo(dirPath);
        if (dirInfo.isReadable()) {
            ui->treeViewContent->setRootIndex(m_fileSystemModel->index(m_dirModel->filePath(index)));
            QDir::setCurrent(m_dirModel->filePath(index));
        } else {
            QMessageBox::critical(this, tr("Error"), tr("\nThe specified folder does not exist or is not readable."));
        }
    }
}


void MainWindow::on_treeViewContent_doubleClicked(const QModelIndex &index)
{
    if(m_archiveMode)
    {
        // Kliknuto na položku v archivu
        //qDebug() << "Kliknuto na položku v archivu";

        ArchiveEntry a_entry = m_archiveModel->entryForIndex(index);
        if ( a_entry[IsDirectory].toBool() ) {
            ui->treeViewContent->setRootIndex(index);
        }else {
            // Rozbal vybrany soubor do TEMP, otevři soubor v přiřazené aplikaci
            QVariantList files = selectedFilesWithChildren();
            const QString tmpPath = QDesktopServices::storageLocation(QDesktopServices::TempLocation);

            ExtractionOptions options;
            options[QLatin1String("PreservePaths")] = true;
            options[QLatin1String("AutoOverwrite")] = 1; // 1 = Ovrwrite existing, 0 = skip

            const QString destinationDirectory = tmpPath;
            ExtractJob *job = m_archiveModel->extractFiles(files, destinationDirectory, options);
            registerJob(job);
            connect(job, SIGNAL(result(QJob*)), this, SLOT(slotTmpExtractionDone(QJob*)));
            job->start();
            QFile *file = new QFile(tmpPath + '/' + files.at(0).toString());
            qDebug() << "File extracted to TEMP: " << file->fileName();
            m_tmpFiles.append(file);
//            while(job->isRunning()) {
//                // čekej na dokončení, zpracuj události
//                qApp->processEvents();
//            }
//            QDesktopServices::openUrl(QUrl::fromLocalFile(file->fileName()));
        }
        return;
    }

    // Klik na položku v adresaři
    QString path = m_fileSystemModel->filePath(index);
    //qDebug() << "path: " << path;
    QFileInfo fileInfo = m_fileSystemModel->fileInfo(index);

    if(fileInfo.isDir())
    {
        // directory
        if (!fileInfo.isReadable()) {
            QMessageBox::critical(this, tr("Error"), tr("\nThe specified folder does not exist or was not readable."));
            return;
        }
        QModelIndex currentIndex(index);
        if(path.endsWith("/."))
        {
            currentIndex = index.parent();
            //qDebug() << m_fileSystemModel->filePath(currentIndex);
        }
        else if(path.endsWith("/.."))
        {
            currentIndex = index.parent().parent();
            //qDebug() << m_fileSystemModel->filePath(currentIndex);
        }
        else
        {
            currentIndex = index;
        }
        ui->treeViewContent->setRootIndex(currentIndex);
        QDir::setCurrent(m_fileSystemModel->filePath(currentIndex));

        // Synchronize selection ui->treeViewContent => ui->treeView
        // - (map fileSystemModel.index to dirModel.index)
        QModelIndex currentDirIndex = m_dirModel->index(m_fileSystemModel->filePath(currentIndex));
        ui->treeView->setCurrentIndex(currentDirIndex); // Zrovna i slectne
        //ui->treeView->selectionModel()->select(currentDirIndex, QItemSelectionModel::ClearAndSelect);
        ui->treeView->scrollTo(currentDirIndex, QAbstractItemView::EnsureVisible);
    }
    else
    {
        // file => Open File...
        QString fileExt = fileInfo.completeSuffix();
        QStringList suportedTypes = ArchiveToolManager::supportedMimeTypes();
        QMimeType mimeType = ArchiveToolManager::determineQMimeType(fileInfo.absoluteFilePath());

        if (!suportedTypes.contains(mimeType.name())) {
            // Nepodporovaný soubor -> konec
            QMessageBox::information(this, tr("Open Archive").append(fileExt),
                                     tr("<qt><br/>File <i>%1</i> is not supported.</qt>").arg(fileInfo.fileName()),
                                     QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        // Open file
        qDebug() << "Oteviram soubor: " << m_fileSystemModel->filePath(index) << " " << fileInfo.filePath()<< fileExt;
        if (!m_archiveModel) {
            // Create archiveModel if not exist
            m_archiveModel = new ArchiveModel(this);
            connect(m_archiveModel, SIGNAL(info(QString)), this, SLOT(showInfoMessage(QString)));
            connect(m_archiveModel, SIGNAL(codePageChanged(QString)), cpsLabel, SLOT(setText(QString)));
            connect(m_archiveModel, SIGNAL(archiveChanged()), cpsLabel, SLOT(clear()));
        }

        // Create Archive object for file
        Archive* archive = ArchiveToolManager::createArchive(mimeType, path, m_archiveModel);
        if (archive) {

            viewArchive();
            connect(ui->treeViewContent->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
            connect(ui->treeViewContent->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged()));

            m_archiveModel->setArchive(archive); // ToDo: nevolat před viewArchive();

            OpenJob* job = m_archiveModel->openArchive();
            registerJob(job);
            connect(job, SIGNAL(result(QJob*)), this, SLOT(slotOpenDone(QJob*)));
            job->start();

            this->setWindowTitle(qApp->applicationName().append(" - " + fileInfo.fileName()));
            this->setWindowFilePath(path); // or this->setWindowFilePath(fileInfo.absoluteFilePath());
            m_recentFilesMenu->addFile(path);
        }
    }
}

void MainWindow::on_treeViewContent_customContextMenuRequested(const QPoint &pos)
{
    QMenu *menu=new QMenu(this);

    menu->addAction(QString("Copy"), this, SLOT(on_copy()));
    menu->exec(ui->treeViewContent->viewport()->mapToGlobal(pos));
    delete menu;
}


void MainWindow::debugArchiveModel()
{
    qDebug() << "Header:"<< ui->treeViewContent->header()->sizeHint().height();
    qDebug() << "Header:"<< ui->treeViewContent->header()->height();
    qDebug() << "Header:"<< m_archiveModel->headerData(0,Qt::Horizontal,Qt::SizeHintRole).isValid();
    qDebug() << "Header:"<< m_archiveModel->headerData(0,Qt::Horizontal,Qt::DisplayRole).isValid();
    qDebug() << "Header:"<< m_archiveModel->headerData(0,Qt::Horizontal,Qt::TextAlignmentRole).isValid();
}


void MainWindow::debugFileSystemModel()
{
    //qDebug() << "Header:"<< ui->treeViewContent->header()->sizeHint().height();
    //qDebug() << "Header:"<< ui->treeViewContent->header()->height();
    //qDebug() << "Header:"<< fileSystemModel->headerData(0,Qt::Horizontal,Qt::SizeHintRole).toSize();
    QMetaObject::invokeMethod(ui->treeViewContent, "updateGeometries");
    qDebug() << "Header:"<< m_fileSystemModel->headerData(0,Qt::Horizontal,Qt::SizeHintRole).isValid();//false
    qDebug() << "Header:"<< m_fileSystemModel->headerData(0,Qt::Horizontal,Qt::DisplayRole).toString();//true
    qDebug() << "Header:"<< m_fileSystemModel->headerData(0,Qt::Horizontal,Qt::TextAlignmentRole).toInt();//true
    qDebug() << "Header:"<< m_fileSystemModel->headerData(0,Qt::Horizontal,Qt::FontRole).isValid();       //false
    qDebug() << "Header:"<< m_fileSystemModel->headerData(0,Qt::Horizontal,Qt::DecorationRole).typeName();//true//type QImage
    qDebug() << "Header:"<< m_fileSystemModel->headerData(0,Qt::Horizontal,Qt::ForegroundRole).isValid();//false
    qDebug() << "Header:"<< m_fileSystemModel->headerData(0,Qt::Horizontal,Qt::BackgroundRole).isValid();//false
    QImage image = m_fileSystemModel->headerData(0,Qt::Horizontal,Qt::DecorationRole).value<QImage>();
    image.save("image.png");
}


void MainWindow::viewArchive()
{
    // Save current directory path
    m_fsLastDirPath = m_dirModel->filePath(ui->treeView->currentIndex());

    QItemSelectionModel *sm_tvc = ui->treeViewContent->selectionModel();
    ui->treeViewContent->setModel(m_archiveModel);
    ui->treeViewContent->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->treeViewContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->treeViewContent->setAlternatingRowColors(true);
    ui->treeViewContent->setAnimated(true);
    ui->treeViewContent->setAllColumnsShowFocus(true);
    ui->treeViewContent->setSortingEnabled(true);

    m_infoFrame->setArchiveModel(m_archiveModel);

    delete sm_tvc;
    sm_tvc = NULL;

    m_archiveMode = true;
    ui->actionClose_archive->setEnabled(m_archiveMode);
    ui->actionProperties->setEnabled(m_archiveMode);

    m_archiveDirModel = new DirFilterProxyModel(this);
    m_archiveDirModel->setDynamicSortFilter(true);
    m_archiveDirModel->setSourceModel(m_archiveModel);

    QItemSelectionModel *sm_tv = ui->treeView->selectionModel();
    ui->treeView->setModel(m_archiveDirModel);
    delete sm_tv;
    sm_tv = NULL;

    connect(m_archiveModel, SIGNAL(loadingStarted(QJob*)), this, SLOT(slotLoadingStarted(QJob*))); //loadingStarted emited in setArchive() > nikdy se neprovede!! Connecty přesunout hned za vytvoření archiveModelu
    connect(m_archiveModel, SIGNAL(loadingFinished(QJob*)), this, SLOT(slotLoadingFinished(QJob*)));
    connect(m_archiveModel, SIGNAL(error(QString,QString)), this, SLOT(showErrorMessage(QString,QString)));
    //debugArchiveModel();

    //qDebug() << "ReSeting wiew";
    ui->treeViewContent->reset();
}


void MainWindow::viewFileSystem()
{
    //ui->treeViewContent->setSortingEnabled(false);
    // Není nutno odpojovat, zato je nutno mazat selection model
    //disconnect(ui->treeViewContent->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
    QItemSelectionModel *m_tvc = ui->treeViewContent->selectionModel();
    ui->treeViewContent->setModel(m_fileSystemModel);
    delete m_tvc;
    m_tvc = NULL;

    ui->treeViewContent->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->treeViewContent->setAlternatingRowColors(false);
    //QModelIndex dirModelIndex = ui->treeView->currentIndex();
    //ui->treeViewContent->setRootIndex(m_fileSystemModel->index(m_dirModel->filePath(dirModelIndex)));

    // Restore initial state
    ui->treeViewContent->header()->restoreState(m_treeViewContentHeaderState);
    ui->treeViewContent->setSortingEnabled(true);
    ui->treeViewContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QItemSelectionModel *m_tv = ui->treeView->selectionModel();
    ui->treeView->setModel(m_dirModel);
    delete m_tv;
    m_tv = NULL;

    // Select last saved directory path
    QDir::setCurrent(m_fsLastDirPath);
    QModelIndex currentIndex = m_dirModel->index(m_fsLastDirPath);
    ui->treeView->setCurrentIndex(currentIndex);
    ui->treeView->expand(currentIndex);
    ui->treeView->hideColumn(1);// for hide Size Column
    ui->treeView->hideColumn(2);// for hide Type Column
    ui->treeView->hideColumn(3);// for hide Date Modified Column

    //ui->treeViewContent->setRootIndex(m_fileSystemModel->index(m_dirModel->filePath(currentIndex)));
    ui->treeViewContent->setRootIndex(m_fileSystemModel->index(m_fsLastDirPath));
    //debugFileSystemModel();
}

void MainWindow::writeSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    settings.setValue("Size", size());
    settings.endGroup();

    if (m_recentFilesMenu) {
            m_recentFilesMenu->saveEntries(settings);
    }
    m_archiveManager->writeSettings(settings);
}

void MainWindow::readSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    resize(settings.value("Size", this->size()).toSize());
    settings.endGroup();

    m_recentFilesMenu->loadEntries(settings);
}

/*
 * Kliknutí na akci "Test"
 */
void MainWindow::on_actionTest_triggered()
{
    TestJob *job = m_archiveModel->testArchive();
    registerJob(job);
    connect(job, SIGNAL(result(QJob*)), this, SLOT(slotTestDone(QJob*)));
    job->start();
}

void MainWindow::onUserQuery(Query *query)
{
    query->execute(this);
}

void MainWindow::onBusyLedClick()
{
    if (isBusy() && (m_jobProgress != NULL)) {
        m_jobProgress->show();
    }
}

void MainWindow::onActionProperties()
{
    PropertiesDialog dialog(this);
    dialog.setArchive(m_archiveModel->archive());
    dialog.exec();
}

void MainWindow::onAbout()
{
#ifdef QT_ARCH_X86_64
    const char *arch = "64 bit";
#else
    const char *arch = "32 bit";
#endif

    QMessageBox::about(this, tr("About %1").arg(qApp->applicationName()),
                       tr("<big><b>QArchiver %1</b></big><br/><br/>"
                          "Uses Qt %2 (%3)<br/><br/>"
                          "Compiled %4 at %5<br/><br/>"
                          "Copyright (C) 2016 Tomáš Hotovec<br/>"
                          "Distributed under the GNU General Public License version 2.")
                       .arg(qApp->applicationVersion(), QT_VERSION_STR, arch, __DATE__, __TIME__));
}


void MainWindow::slotTestDone(QJob *job)
{
    if (job->error()) {
        qDebug() << job->errorText();
        QMessageBox::critical(this, tr("Error", "@title:window"),
                              tr("<qt>Testing the archive <i>%1</i> failed with the following error: <br/><br/>"
                                 "%2</qt>").arg(m_archiveModel->archive()->fileName(),  job->errorText()));
        return;
    }

    TestJob *testJob = qobject_cast<TestJob *>(job);
    if (testJob != NULL) {
        TestResultDialog* testResDialog = new TestResultDialog(this);
        testResDialog->setTestResultText(testJob->testResult());
        testResDialog->exec();
    }
}

void MainWindow::slotOpenDone(QJob *job)
{
    if (job->error()) {
        qDebug() << job->errorText();
        QMessageBox::critical(this, tr("Error", "@title:window"),
                              tr("<qt>Opening the archive <i>%1</i> failed with the following error: <br/><br/>"
                                 "%2</qt>").arg(m_archiveModel->archive()->fileName(),  job->errorText()));
        return;
    }
    OpenJob* openJob = qobject_cast<OpenJob *>(job);
    if(openJob->success()) {
        QTimer::singleShot(0, m_archiveModel, SLOT(listArchive()));
        /*ListJob* job = m_archiveModel->listArchive();*/
    }else {
        // Nemazat hned, Nechat eventLoop dokončit události
        QTimer::singleShot(0, this, SLOT(closeArchive()));
    }
}

void MainWindow::slotCreateDone(QJob *job)
{
    if (job->error()) {
        qDebug() << job->errorText();
        QMessageBox::critical(this, tr("Error", "@title:window"),
                              tr("<qt>Creating the archive <i>%1</i> failed with the following error:<br/><br/>"
                                 "%2</qt>").arg(m_archiveModel->archive()->fileName(),  job->errorText()));
    }
}


void MainWindow::registerJob(QJob *job)
{
    if (!m_jobProgress) {
        m_jobProgress = new ProgressDialog(this);
        connect(busyLed, SIGNAL(clicked()), this, SLOT(onBusyLedClick()));
    }
    m_jobProgress->registerJob(job);


    connect(job, SIGNAL(started(QJob*)), this, SLOT(setBusyGui()));
    connect(job, SIGNAL(result(QJob*)), this, SLOT(setReadyGui()));

    // Redirect Query
    disconnect(job, SIGNAL(userQuery(Query*)), m_archiveModel, SLOT(onUserQuery(Query*)));
    connect(job, SIGNAL(userQuery(Query*)), this, SLOT(onUserQuery(Query*)));
}


bool MainWindow::isBusy() const
{
    return m_busy;
}


bool MainWindow::isPreviewable(const QModelIndex &index) const
{
    // Only on files is preview action enabled
    return index.isValid() && (!m_archiveModel->entryForIndex(index)[ IsDirectory ].toBool());
}


void MainWindow::slotLoadingStarted(QJob *job)
{
    qDebug() << "MainWindow::slotLoadingStarted()";
    registerJob(job);
}


void MainWindow::slotLoadingFinished(QJob *job)
{
    qDebug() << "MainWindow::slotLoadingFinished()";

    if (job->error()) {
        QMessageBox::critical(this, tr("Error Opening Archive", "@title:window"),
        tr("<qt>Loading the archive <i>%1</i> failed with the following error:<br/><br/>"
           "%2</qt>").arg(m_archiveModel->archive()->fileName(),  job->errorText()));
    }

    ui->treeViewContent->sortByColumn(0, Qt::AscendingOrder);
    ui->treeViewContent->expandToDepth(0);

    // After loading all files, resize the columns to fit all fields
    ui->treeViewContent->header()->resizeSections(QHeaderView::ResizeToContents);

    //qDebug() << "Schovávám sloupce:"<< "Počet sloupců"<< m_archiveDirModel->columnCount();
    if (m_archiveDirModel->columnCount() > 1) {
        // Hide all coluns, except Name column
        for (int i = 1; i < m_archiveDirModel->columnCount(); ++i) {
            ui->treeView->hideColumn(i);
        }
    }

    updateActions();

    if (m_openArgs.getOption(QLatin1String( "showExtractDialog" )) == QLatin1String( "true" )) {
        QTimer::singleShot(0, this, SLOT(slotExtractFiles()));
    }
}


void MainWindow::showErrorMessage(const QString &errorMessage, const QString &details)
{
    m_openArgs.removeOption("showExtractDialog");

    if (details.isEmpty()) {
        QMessageBox::critical(this, tr("Error", "@title:window"), errorMessage);
    } else {
        QMessageBox::critical(this, tr("Error", "@title:window"), errorMessage + "<br/><br/>" + details);
    }
}


void MainWindow::showInfoMessage(const QString &message)
{
    //m_openArgs.removeOption("showExtractDialog");
    if ( m_openArgs.getOption("showExtractDialog") == "true") {
        return;
    }

    QSettings settings;
    if (settings.value("showEncodingInfo", true).toBool() == false) {
       return;
    }
    else {
        //QMessageBox::information(this, tr("Info", "@title:window"), message);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
        QMessageBox infoMsg(this);
#else
        // QMessageBox v Qt < 5.2 neumí zobrazit checkbox
        QCbMessageBox infoMsg(this);
#endif
        QCheckBox cb;
        cb.setText(tr("Do not &show again."));

        infoMsg.setCheckBox(&cb);
        infoMsg.setIcon(QMessageBox::Information);
        infoMsg.setWindowTitle(tr("Info", "@title:window"));
        infoMsg.setText(message);
        //infoMsg.setInformativeText("Informative text");
        infoMsg.setStandardButtons(QMessageBox::Ok);
        infoMsg.setDefaultButton(QMessageBox::Ok);
        infoMsg.exec();
        if (infoMsg.result() == QMessageBox::Ok) {
            settings.setValue("showEncodingInfo", !cb.isChecked());
        }
    }
}


void MainWindow::slotExtractFiles()
{
    if (!m_archiveModel) {
        return;
    }

    ExtractDialog* dialog = new ExtractDialog(this);

    // Extract: Selected files / All files
    if (ui->treeViewContent->selectionModel()->selectedRows().count() > 0) {
        dialog->setExtractSelectedFiles(true);
    } else {
        dialog->setExtractSelectedFiles(false);
    }

    dialog->setSingleFolderArchive(isSingleEntryArchive());
    dialog->setSubfolder(detectSubfolder());
    dialog->setCurrentDirectory(QFileInfo(m_archiveModel->archive()->fileName()).path());

    if (dialog->exec()) {
        // this is done to update the quick extract menu
        updateActions();

        QVariantList files;

        // if the user has chosen to extract only selected entries,
        // fetch these from the treeview
        if (!dialog->extractAllFiles()) {
            files = selectedFilesWithChildren();
        }

        qDebug() << "Selected " << files;

        ExtractionOptions options;

        if (dialog->preservePaths()) {
            options[QLatin1String("PreservePaths")] = true;
        }
        if (dialog->openDestDirAfterExtraction()) {
            options[QLatin1String("OpenDestFolder")] = true;
        }
        if(dialog->closeAfterExtraction()) {
            options[QLatin1String("CloseAfterExt")] = true;
        }
        if (dialog->overwriteExisting()) {
            options[QLatin1String("AutoOverwrite")] = 1; // 1 = Ovrwrite existing, 0 = skip
        }

        const QString destinationDirectory = dialog->destinationDirectory();
        ExtractJob *job = m_archiveModel->extractFiles(files, destinationDirectory, options);
        registerJob(job);

        connect(job, SIGNAL(result(QJob*)), this, SLOT(slotExtractionDone(QJob*)));

        job->start();
    }

    delete dialog;
    m_openArgs.removeOption("showExtractDialog" );
}


void MainWindow::slotExtractionDone(QJob* job)
{
    qDebug() << "slotExtractionDone()";
    if (job->error()) {
        QMessageBox::critical(this, tr("Error", "@title:window"), job->errorString());
    } else {
        ExtractJob *extractJob = qobject_cast<ExtractJob*>(job);
        Q_ASSERT(extractJob);

        if (extractJob->extractionOptions().value("OpenDestFolder", false).toBool()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(extractJob->destinationDirectory()));
        }

        if (extractJob->extractionOptions().value("CloseAfterExt", false).toBool()) {
            close();
        }
    }
}


void MainWindow::slotTmpExtractionDone(QJob *job)
{
    qDebug() << "MainWindow::slotTmpExtractionDone()";
    if (job->error()) {
        QMessageBox::critical(this, tr("Error", "@title:window"), job->errorString());
    } else {
        ExtractJob *extractJob = qobject_cast<ExtractJob*>(job);
        Q_ASSERT(extractJob);
        QString filePath = extractJob->destinationDirectory() + '/' + extractJob->filesToExtract().at(0).toString();
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    }
    //QTimer::singleShot(1, job, SLOT(deleteLater()));
    //delete job;
}


void MainWindow::slotAddFiles()
{
    qDebug() << "MainWindow::onActionAddFiles()";

    const QStringList filesToAdd = QFileDialog::getOpenFileNames(this, tr("Add Files", "@title:window"), QDir::currentPath());

    slotAddFiles(filesToAdd);
}


void MainWindow::slotAddFiles(const QStringList &files, const QString &path)
{

    if (files.isEmpty()) {
        return;
    }

    qDebug() << "Adding " << files << " to " << path;

    QStringList cleanFilesToAdd(files);
    for (int i = 0; i < cleanFilesToAdd.size(); ++i) {
        QString& file = cleanFilesToAdd[i];
        // Projdi přidávané položky, pokud je položka adresář přidej nakonec '/' pokud tam není
        if (QFileInfo(file).isDir()) {
            if (!file.endsWith(QLatin1Char( '/' ))) {
                file += QLatin1Char( '/' );
            }
        }
    }

    CompressionOptions options;

    QString firstPath = cleanFilesToAdd.first();
    // Pokud je první cesta adresařem, odeber '/' z konce jinak funkce dir()
    // nevrací parent directory adresáře, ale samotný adresář
    // ("adresar/" ==> filename prázdné, a parent directory je sám adresář)
    if (firstPath.right(1) == QLatin1String( "/" )) {
        firstPath.chop(1);
    }
    firstPath = QFileInfo(firstPath).dir().absolutePath();

    qDebug() << "Detected relative path to be " << firstPath;
    options[QLatin1String( "GlobalWorkDir" )] = firstPath;

    AddJob *job = m_archiveModel->addFiles(cleanFilesToAdd, options);
    if (!job) {
        return;
    }

    connect(job, SIGNAL(result(QJob*)), this, SLOT(slotAddFilesDone(QJob*)));
    registerJob(job);
    job->start();
}


void MainWindow::slotAddDir()
{
    qDebug() << "slotAddDir()";
    const QString dirToAdd = QFileDialog::getExistingDirectory(this, tr("Add Folder","@title:window"), QDir::currentPath());

    if (!dirToAdd.isEmpty()) {
        slotAddFiles(QStringList() << dirToAdd);
    }
}


void MainWindow::slotAddFilesDone(QJob *job)
{
    qDebug() << "MainWindow::slotAddFilesDone()";
    if (job->error()) {
        QMessageBox::critical(this, tr("Error", "@title:window"), job->errorText());
    }

    ui->treeViewContent->sortByColumn(0, Qt::AscendingOrder);
    ui->treeViewContent->expandToDepth(0);
}


void MainWindow::slotDeleteFiles()
{
    qDebug() << "MainWindow::slotDeleteFiles()";

    const int reallyDelete =
            QMessageBox::question(this, tr("Delete files", "@title:window"),
                                  tr("Deleting these files is not undoable. Are you sure you want to do this?"),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (reallyDelete == QMessageBox::No) {
        return;
    }

    DeleteJob *job = m_archiveModel->deleteFiles(selectedFilesWithChildren());
    connect(job, SIGNAL(result(QJob*)), this, SLOT(slotDeleteFilesDone(QJob*)));
    registerJob(job);
    job->start();
}


void MainWindow::slotDeleteFilesDone(QJob *job)
{
    qDebug() << "MainWindow::slotDeleteFilesDone()";
    if (job->error()) {
        QMessageBox::critical(this, tr("Error", "@title:window"), job->errorString());
    }
}


void MainWindow::updateActions()
{
    //qDebug() << "MainWindow::updateActions()";
    bool isWritable = m_archiveModel == NULL ? false : m_archiveModel->archive() && !(m_archiveModel->archive()->isReadOnly());

    ui->actionTest->setEnabled((m_archiveModel != NULL) && !isBusy());
    ui->actionAdd->setEnabled(!isBusy() && isWritable);
    m_addDirAction->setEnabled(!isBusy() && isWritable);
    ui->actionExtract->setEnabled(m_archiveModel == NULL ? false : !isBusy() && (m_archiveModel->rowCount() > 0));
    ui->actionDelete->setEnabled(m_archiveModel == NULL ? false : !isBusy() && isWritable
                                                          && (ui->treeViewContent->selectionModel()->selectedRows().count() > 0));
    m_previewAction->setEnabled(m_archiveModel == NULL ? false : !isBusy() && (ui->treeViewContent->selectionModel()->selectedRows().count() == 1)
                                                         && isPreviewable(ui->treeViewContent->selectionModel()->currentIndex()));
    m_codepageAction->setEnabled(m_archiveModel == NULL ? false : !isBusy()); //m_codepageAction

    ui->actionGo_Up->setEnabled(!isBusy());

    ui->actionSet_Password->setEnabled(m_archiveModel == NULL ? false : !isBusy()); //actionSet_Password
}

void MainWindow::updateFileMenuActions()
{
    ui->actionNew_archive->setEnabled(!this->isBusy());
    ui->actionOpen_archive->setEnabled(!this->isBusy());
    ui->actionClose_archive->setEnabled(!this->isBusy());
    //ui->actionRecent_files->setEnabled(!this->isBusy());
    m_recentFilesMenu->setEnabled(!this->isBusy());
}

void MainWindow::selectionChanged()
{
    m_infoFrame->setIndexes(ui->treeViewContent->selectionModel()->selectedRows());
}

void MainWindow::setBusyGui()
{
    //qDebug() << "MainWindow::setBusyGui()";
    m_busy = true;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    //busyLed->setColor(QColor(255,0,0)); // QLed
    busyLed->turnColor(2);
    busyLed->setToolTip(tr("Busy"));
    cpsLabel->setEnabled(!m_busy);
    ui->treeViewContent->setEnabled(!m_busy);
    ui->treeView->setEnabled(!m_busy);

    updateActions();
    updateFileMenuActions();
    //emit busy();
}

void MainWindow::setReadyGui()
{
    //qDebug() << "MainWindow::setReadyGui()";
    //busyLed->setColor(QColor(0,255,0)); // QLed
    QApplication::restoreOverrideCursor();
    busyLed->turnColor(1);
    busyLed->setToolTip(tr("Ready"));
    m_busy = false;
    cpsLabel->setEnabled(!m_busy);
    ui->treeViewContent->setEnabled(!m_busy);
    ui->treeView->setEnabled(!m_busy);
    updateActions();
    updateFileMenuActions();
    //emit ready();
}

void MainWindow::slotPreview()
{
    const QModelIndex index = ui->treeViewContent->selectionModel()->currentIndex();

    if (isPreviewable(index)) {
        slotPreview(index);
    }
}


void MainWindow::slotPreview(const QModelIndex &index)
{
    const ArchiveEntry& entry =  m_archiveModel->entryForIndex(index);

    if (!entry.isEmpty()) {
        ExtractionOptions options;
        options[QLatin1String( "PreservePaths" )] = true;
        options[QLatin1String( "AutoOverwrite" )] = 1; // 1 = Ovrwrite existing, 0 = skip
        const QString tmpPath = QDesktopServices::storageLocation(QDesktopServices::TempLocation);

        ExtractJob *job = m_archiveModel->extractFile(entry[ InternalID ], tmpPath, options);
        registerJob(job);
        connect(job, SIGNAL(result(QJob*)), this, SLOT(slotPreviewExtracted(QJob*)));
        job->start();
    }
}


void MainWindow::slotPreviewExtracted(QJob *job)
{
    if (job->error()) {
        QMessageBox::critical(this, tr("Error"), job->errorString());
    } else {
        const ArchiveEntry& entry = m_archiveModel->entryForIndex(ui->treeViewContent->selectionModel()->currentIndex());
        const QString tmpPath = QDesktopServices::storageLocation(QDesktopServices::TempLocation);

        QFile *file = new QFile(tmpPath + QLatin1Char('/') + entry[FileName].toString());
        qDebug() << "File extracted to TEMP: " << file->fileName();
        m_tmpFiles.append(file);

        QDesktopServices::openUrl(QUrl::fromLocalFile(file->fileName()));
    }
    //setReadyGui(); //ready se volá automaticky na základě job::result() signálu, zde již nebude potřeba
}


void MainWindow::slotChangeCodepage()
{
    if (!m_archiveMode) {
        return;
    }

    QString cp = m_archiveModel->codePage();

    int current = 0;
    QStringList items;
    for (int i = 0; i < cp_count; ++i) {
        const char *cp0 = code_pages[i][0];
        if (cp == cp0) {
            current = i;
        }

        const char *cp1 = code_pages[i][1];
        items << QString::fromAscii(cp1);
    }

    bool ok;
    QString item = QInputDialog::getItem(this, tr("Filename Encoding", "@title:window"),
                                         tr("Encoding:"), items, current, false, &ok);
    if (ok && !item.isEmpty()) {
        QString cp;

        for (int i = 0; i < cp_count; ++i) {
            const char *cp1 = code_pages[i][1];
            if (item == QString::fromAscii(cp1)) {
                cp = code_pages[i][0];
            }
        }
        m_archiveModel->setCodePage(cp);
    }
}


void MainWindow::smazan(QObject * obj)
{
    qDebug() << "QObject:" << obj << "dealokován";
}


void MainWindow::on_pushButton_Make_Dir_clicked()
{
    //Make dir
    // Make directory
    QModelIndex index = ui->treeView->currentIndex();
    if(!index.isValid()) return;

    QString name  = QInputDialog::getText(this, "Name", "Enter a name");

    if(name.isEmpty()) return;

    m_dirModel->mkdir(index, name);
}


void MainWindow::on_pushButton_Delete_Dir_clicked()
{
    //Delete dir
    // Get the current selection
    QModelIndex index = ui->treeView->currentIndex();
    if(!index.isValid()) return;
    int ret = QMessageBox::warning(this, tr("Smazat soubor"),
                                   tr("Opravdu chcete smazat soubor:\n").append(m_dirModel->fileName(index)),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Cancel);
    if(ret==QMessageBox::Ok){
        if(m_dirModel->fileInfo(index).isDir())
        {
            // directory
            m_dirModel->rmdir(index);
        }
        else
        {
            // file
            m_dirModel->remove(index);
        }
    }
}


void MainWindow::on_pushButton_TestProgressDialog_clicked()
{
    ProgressDialog* pDialog = new ProgressDialog(this);
    //pDialog->setWindowTitle("Test ProgressDialogu");
    pDialog->exec();

    delete pDialog;
}


void MainWindow::on_pushButton_testEnv_clicked()
{
    ;
}

void MainWindow::on_actionSet_Password_triggered()
{
    bool ok;
    QString password  = QInputDialog::getText(this, tr("Password"), tr("Please enter the password:"), QLineEdit::Password, QString(), &ok);
    if (ok) {
        m_archiveModel->archive()->setPassword(password);
    }
}
