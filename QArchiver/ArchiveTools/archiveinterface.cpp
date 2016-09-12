#include "archiveinterface.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

ArchiveInterface::ArchiveInterface(QObject *parent) :
    QObject(parent), m_waitForFinishedSignal(false)
{
    qDebug("Constructing %s object.", metaObject()->className());
    m_archive = NULL;
}

ArchiveInterface::~ArchiveInterface()
{
}

QString ArchiveInterface::filename() const
{
    return m_archive->fileName();
}

QString ArchiveInterface::password() const
{
    return m_archive->password();
}

void ArchiveInterface::setPassword(const QString &password)
{
    m_archive->setPassword(password);
}

// archive() konflikt s libarchive "archive" => getArchive()
Archive *ArchiveInterface::getArchive() const
{
    return m_archive;
}

void ArchiveInterface::setArchive(Archive *arch)
{
    m_archive = arch;
}

bool ArchiveInterface::isReadOnly() const
{
    // default implementation
    return true;
}

bool ArchiveInterface::waitForFinishedSignal() const
{
    return m_waitForFinishedSignal;
}

bool ArchiveInterface::doKill()
{
    // default implementation
    return false;
}

void ArchiveInterface::setWaitForFinishedSignal(bool value)
{
    m_waitForFinishedSignal = value;
}

