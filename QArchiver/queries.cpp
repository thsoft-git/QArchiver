#include "queries.h"

#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QWeakPointer>

enum RenameDialog_Result {R_RESUME = 6, R_RESUME_ALL = 7, R_OVERWRITE = 4, R_OVERWRITE_ALL = 5, R_SKIP = 2, R_AUTO_SKIP = 3, R_RENAME = 1, R_AUTO_RENAME = 8, R_CANCEL = 0};

Query::Query()
{
    m_responseMutex.lock();
}

QVariant Query::response()
{
    return m_data.value(QLatin1String( "response" ));
}

void Query::waitForResponse()
{
    qDebug();

    //if there is no response set yet, wait
    if (!m_data.contains(QLatin1String("response"))) {
        m_responseCondition.wait(&m_responseMutex);
    }
    m_responseMutex.unlock();
}

void Query::setResponse(QVariant response)
{
    qDebug();

    m_data[QLatin1String( "response" )] = response;
    m_responseCondition.wakeAll();
}

OverwriteQuery::OverwriteQuery(const QString &filename) :
        m_noRenameMode(false),
        m_multiMode(true)
{
    m_data[QLatin1String( "filename" )] = filename;
}

void OverwriteQuery::execute(QWidget *parent)
{
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    QString sourcePath = m_data.value(QLatin1String( "filename" )).toString();
    //QString destPath = m_data.value(QLatin1String( "filename" )).toString();
    int response = QMessageBox::question(parent,
                                       QObject::tr("File already exists"),
                                       /*QObject::tr("Confirm file Overwrite?\n\nReplace file: %1\n\nWith file: %2").arg(sourcePath, destPath),*/
                                       QObject::tr("Confirm file Overwrite?\n\nReplace file: %1").arg(sourcePath),
                                       QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel|QMessageBox::YesToAll,
                                       QMessageBox::NoButton);
    setResponse(response);

    QApplication::restoreOverrideCursor();
}

bool OverwriteQuery::responseCancelled()
{
    return m_data.value(QLatin1String( "response" )).toInt() == QMessageBox::Cancel;
}
bool OverwriteQuery::responseOverwriteAll()
{
    return m_data.value(QLatin1String( "response" )).toInt() == QMessageBox::YesToAll;
}
bool OverwriteQuery::responseOverwrite()
{
    return m_data.value(QLatin1String( "response" )).toInt() == QMessageBox::Yes;
}

bool OverwriteQuery::responseRename()
{
    return false;
    //return m_data.value(QLatin1String( "response" )).toInt() == 0
}

bool OverwriteQuery::responseSkip()
{
    return m_data.value(QLatin1String( "response" )).toInt() == QMessageBox::No;
}

bool OverwriteQuery::responseAutoSkip()
{
    return m_data.value(QLatin1String( "response" )).toInt() == QMessageBox::NoToAll;
}

QString OverwriteQuery::newFilename()
{
    return m_data.value(QLatin1String( "newFilename" )).toString();
}

void OverwriteQuery::setNoRenameMode(bool enableNoRenameMode)
{
    m_noRenameMode = enableNoRenameMode;
}

bool OverwriteQuery::noRenameMode()
{
    return m_noRenameMode;
}

void OverwriteQuery::setMultiMode(bool enableMultiMode)
{
    m_multiMode = enableMultiMode;
}

bool OverwriteQuery::multiMode()
{
    return m_multiMode;
}

PasswordNeededQuery::PasswordNeededQuery(const QString& archiveFilename, bool incorrectTryAgain)
{
    m_data[QLatin1String( "archiveFilename" )] = archiveFilename;
    m_data[QLatin1String( "incorrectTryAgain" )] = incorrectTryAgain;
}

void PasswordNeededQuery::execute(QWidget *parent)
{
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

    QString prompt = QObject::tr("The archive <i>%1</i> is password protected.<br/> Please enter the password to extract the file.");
    bool ok;
    QString password  = QInputDialog::getText(parent, QObject::tr("Password"),
                                              prompt.arg(m_data.value(QLatin1String( "archiveFilename" )).toString()),
                                              QLineEdit::Password, QString(), &ok);

    m_data[QLatin1String("password")] = password;
    setResponse(ok && !password.isEmpty());

    QApplication::restoreOverrideCursor();
}

QString PasswordNeededQuery::password()
{
    return m_data.value(QLatin1String("password")).toString();
}

bool PasswordNeededQuery::responseCancelled()
{
    return !m_data.value(QLatin1String("response")).toBool();
}


EncodingWarnQuery::EncodingWarnQuery(const QString &codepage)
{
    m_data["codepage"] = codepage;
}


void EncodingWarnQuery::execute(QWidget *parent)
{
    // The archive contains the names of files encoded in CP852.
    // It is not recommended to modify a archive with this encoding by \"zip\" program.
    // Do you really want to continue?
    QMessageBox warnMsg(parent);
    warnMsg.setIcon(QMessageBox::Warning);
    warnMsg.setText(QObject::tr("<qt>The archive contains the names of files encoded in %1. It is not recommended to modify a archive with this encoding by \"zip\" program.</qt>").arg(m_data.value("codepage").toString()));

    warnMsg.setInformativeText(QObject::tr("Do you really want to continue?"));
    warnMsg.addButton(QMessageBox::Yes);
    warnMsg.addButton(QMessageBox::No);
    warnMsg.setDefaultButton(QMessageBox::No);
    setResponse(warnMsg.exec() == QMessageBox::Yes);

}

bool EncodingWarnQuery::canContinue()
{
    return response().toBool();
}
