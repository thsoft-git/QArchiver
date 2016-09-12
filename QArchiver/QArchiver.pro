#-------------------------------------------------
#
# Project created by QtCreator 2015-02-10T18:06:14
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qarchiver
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ArchiveTools/archiveinterface.cpp \
    ArchiveTools/cliinterface.cpp \
    ArchiveTools/clirarplugin.cpp \
    ArchiveTools/cli7zplugin.cpp \
    ArchiveTools/clizipplugin.cpp \
    queries.cpp \
    archivemodel.cpp \
    iconprovider.cpp \
    Codecs/textencoder.cpp \
    Codecs/qibm852codec.cpp \
    jobs.cpp \
    overwritedialog.cpp \
    extractdialog.cpp \
    openarguments.cpp \
    progressdialog.cpp \
    qstandarddirs.cpp \
    QtExt/qled.cpp \
    testresultdialog.cpp \
    archivetoolmanager.cpp \
    addtoarchive.cpp \
    jobinterface.cpp \
    adddialog.cpp \
    qarchive.cpp \
    QtExt/qrecentfilesmenu.cpp \
    infoframe.cpp \
    QSizeFormater.cpp \
    filesystemmodel.cpp \
    dirfilterproxymodel.cpp \
    QtExt/qduoled.cpp \
    ArchiveTools/qlibarchive.cpp \
    settingsdialog.cpp \
    ArchiveTools/libachivesettingswidget.cpp \
    Codecs/qenca.cpp \
    QtExt/qclickablelabel.cpp \
    Codecs/unzipreverseencoder.cpp \
    QtExt/qbargraf.cpp \
    propertiesdialog.cpp \
    ArchiveTools/singlefilecompression.cpp \
    QtExt/qcbmessagebox.cpp

HEADERS  += mainwindow.h \
    ArchiveTools/archiveinterface.h \
    ArchiveTools/cliinterface.h \
    ArchiveTools/clirarplugin.h \
    ArchiveTools/cli7zplugin.h \
    ArchiveTools/clizipplugin.h \
    queries.h \
    archivemodel.h \
    QSizeFormater.h \
    iconprovider.h \
    Codecs/textencoder.h \
    Codecs/qibm852codec.h \
    jobs.h \
    overwritedialog.h \
    extractdialog.h \
    openarguments.h \
    progressdialog.h \
    qstandarddirs.h \
    QtExt/qled.h \
    testresultdialog.h \
    archivetoolmanager.h \
    addtoarchive.h \
    jobinterface.h \
    adddialog.h \
    qarchive.h \
    QtExt/qrecentfilesmenu.h \
    infoframe.h \
    filesystemmodel.h \
    dirfilterproxymodel.h \
    QtExt/qduoled.h \
    ArchiveTools/qlibarchive.h \
    settingsdialog.h \
    ArchiveTools/libachivesettingswidget.h \
    Codecs/qenca.h \
    QtExt/qclickablelabel.h \
    Codecs/unzipreverseencoder.h \
    QtExt/qbargraf.h \
    propertiesdialog.h \
    ArchiveTools/singlefilecompression.h \
    QtExt/qcbmessagebox.h

FORMS    += mainwindow.ui \
    overwritedialog.ui \
    extractdialog.ui \
    progressdialog.ui \
    testresultdialog.ui \
    adddialog.ui \
    infoframe.ui \
    settingsdialog.ui \
    propertiesdialog.ui

RESOURCES += \
    icons.qrc

OTHER_FILES += \
    QArchiver.desktop \
    qarchiver_addto.desktop \
    qarchiver_extract.desktop \
    ArchiveTools/LibArchiveErrorMessages.txt \
    app-icons/hi16-apps-qarchiver.png \
    app-icons/hi16a-apps-qarchiver.png \
    app-icons/hi22-apps-qarchiver.png \
    app-icons/hi32-apps-qarchiver.png \
    app-icons/hi48-apps-qarchiver.png \
    app-icons/hi64-apps-qarchiver.png \
    app-icons/hi128-apps-qarchiver.png \
    app-icons/hisc-apps-qarchiver.svgz \
    app-icons/COPYING.icons.txt \
    icons/settings.png \
    icons/help-about.png \
    icons/go-up.png \
    icons/document-preview-archive.png \
    icons/document-open.png \
    icons/document-open-recent.png \
    icons/document-new.png \
    icons/dialog-ok.png \
    icons/dialog-ok-apply.png \
    icons/dialog-close.png \
    icons/dialog-cancel.png \
    icons/archive-test.png \
    icons/archive-test-alt.png \
    icons/archive-remove.png \
    icons/archive-new.png \
    icons/archive-insert.png \
    icons/archive-insert-directory.png \
    icons/archive-extract.png \
    icons/archive-codepage.png \
    icons/application-exit.png \
    icons/COPYING.icons.txt \
    ChangeLog.txt

win32: LIBS += -lshell32 -lOle32 -lUser32

unix: LIBS += -larchive -lenca -lQtMimeTypes

unix {

## Spustitelny soubor aplikace
target.path = /usr/local/bin

## Desktop soubor
desktop.path = /usr/share/applications
desktop.files += QArchiver.desktop

## Context menu KDE
context.path = /usr/share/kde4/services/ServiceMenus
context.files += qarchiver_addto.desktop qarchiver_extract.desktop

INSTALLS += target desktop context

## Ikony aplikace
isEmpty(ICO_PREFIX) {
    ICO_PREFIX = /usr
}

hicolor16.path = $${ICO_PREFIX}/share/icons/hicolor/16x16/apps
hicolor16.extra = -$(INSTALL_FILE) $$PWD/app-icons/hi16-apps-qarchiver.png $(INSTALL_ROOT)$${hicolor16.path}/qarchiver.png
hicolor16.uninstall = -$(DEL_FILE) $(INSTALL_ROOT)$${hicolor16.path}/qarchiver.png

hicolor22.path = $${ICO_PREFIX}/share/icons/hicolor/22x22/apps
hicolor22.extra = -$(INSTALL_FILE) $$PWD/app-icons/hi22-apps-qarchiver.png $(INSTALL_ROOT)$${hicolor22.path}/qarchiver.png
hicolor22.uninstall = -$(DEL_FILE) $(INSTALL_ROOT)$${hicolor22.path}/qarchiver.png

## Dle Qt dokumentace ikona min v $prefix/share/icons/hicolor/32x32/apps
hicolor32.path = $${ICO_PREFIX}/share/icons/hicolor/32x32/apps
hicolor32.extra = -$(INSTALL_FILE) $$PWD/app-icons/hi32-apps-qarchiver.png $(INSTALL_ROOT)$${hicolor32.path}/qarchiver.png
hicolor32.uninstall = -$(DEL_FILE) $(INSTALL_ROOT)$${hicolor32.path}/qarchiver.png

## Dle freedesktop.org ikona min v $prefix/share/icons/hicolor/48x48/apps
hicolor48.path = $${ICO_PREFIX}/share/icons/hicolor/48x48/apps
hicolor48.extra = -$(INSTALL_FILE) $$PWD/app-icons/hi48-apps-qarchiver.png $(INSTALL_ROOT)$${hicolor48.path}/qarchiver.png
hicolor48.uninstall = -$(DEL_FILE) $(INSTALL_ROOT)$${hicolor48.path}/qarchiver.png

hicolor64.path = $${ICO_PREFIX}/share/icons/hicolor/64x64/apps
hicolor64.extra = -$(INSTALL_FILE) $$PWD/app-icons/hi64-apps-qarchiver.png $(INSTALL_ROOT)$${hicolor64.path}/qarchiver.png
hicolor64.uninstall = -$(DEL_FILE) $(INSTALL_ROOT)$${hicolor64.path}/qarchiver.png

hicolor128.path = $${ICO_PREFIX}/share/icons/hicolor/128x128/apps
hicolor128.extra = -$(INSTALL_FILE) $$PWD/app-icons/hi128-apps-qarchiver.png $(INSTALL_ROOT)$${hicolor128.path}/qarchiver.png
hicolor128.uninstall = -$(DEL_FILE) $(INSTALL_ROOT)$${hicolor128.path}/qarchiver.png

## Svg icon: $ICO_PREFIX/share/icons/hicolor/scalable/apps
hicolorSC.path = $${ICO_PREFIX}/share/icons/hicolor/scalable/apps
hicolorSC.extra = -$(INSTALL_FILE) $$PWD/app-icons/hisc-apps-qarchiver.svgz $(INSTALL_ROOT)$${hicolorSC.path}/qarchiver.svgz
hicolorSC.uninstall = -$(DEL_FILE) $(INSTALL_ROOT)$${hicolorSC.path}/qarchiver.svgz

INSTALLS += hicolor16 hicolor22 hicolor32 hicolor48 hicolor64 hicolor128 hicolorSC
}
